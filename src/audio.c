#include "amiga_base.h"
#include <devices/audio.h>
#include <proto/exec.h>
#include <proto/dos.h>

AudioSystem audio_system;

BOOL init_audio(void)
{
    UBYTE allocation_map[4] = {1, 2, 4, 8};
    
    audio_system.audio_port = CreateMsgPort();
    if (!audio_system.audio_port) {
        return FALSE;
    }

    audio_system.audio_io = (struct IOAudio *)CreateIORequest(audio_system.audio_port, sizeof(struct IOAudio));
    if (!audio_system.audio_io) {
        DeleteMsgPort(audio_system.audio_port);
        return FALSE;
    }

    audio_system.audio_io->ioa_Request.io_Message.mn_Node.ln_Pri = 0;
    audio_system.audio_io->ioa_Data = allocation_map;
    audio_system.audio_io->ioa_Length = sizeof(allocation_map);

    if (OpenDevice("audio.device", 0, (struct IORequest *)audio_system.audio_io, 0) != 0) {
        DeleteIORequest((struct IORequest *)audio_system.audio_io);
        DeleteMsgPort(audio_system.audio_port);
        return FALSE;
    }

    audio_system.audio_device_open = TRUE;
    audio_system.current_mod = NULL;
    audio_system.playing = FALSE;

    return TRUE;
}

void cleanup_audio(void)
{
    if (audio_system.playing) {
        stop_mod();
    }

    if (audio_system.audio_device_open) {
        CloseDevice((struct IORequest *)audio_system.audio_io);
        audio_system.audio_device_open = FALSE;
    }

    if (audio_system.audio_io) {
        DeleteIORequest((struct IORequest *)audio_system.audio_io);
        audio_system.audio_io = NULL;
    }

    if (audio_system.audio_port) {
        DeleteMsgPort(audio_system.audio_port);
        audio_system.audio_port = NULL;
    }
}

BOOL load_mod_file(const char *filename, ModFile *mod)
{
    BPTR file;
    UBYTE header[1084];
    LONG bytes_read;
    UWORD i, j;
    ULONG pattern_size;

    file = Open((STRPTR)filename, MODE_OLDFILE);
    if (!file) {
        return FALSE;
    }

    bytes_read = Read(file, header, 1084);
    if (bytes_read < 1084) {
        Close(file);
        return FALSE;
    }

    CopyMem(header, mod->name, 20);
    mod->name[20] = '\0';

    for (i = 0; i < 31; i++) {
        UWORD sample_offset = 20 + i * 30;
        
        mod->samples[i].sample_length = (header[sample_offset + 22] << 8) | header[sample_offset + 23];
        mod->samples[i].sample_length *= 2;
        
        mod->samples[i].sample_rate = 8287;
        mod->samples[i].volume = header[sample_offset + 25];
        
        if (mod->samples[i].sample_length > 2) {
            mod->samples[i].sample_data = AllocMem(mod->samples[i].sample_length, MEMF_CHIP);
            if (!mod->samples[i].sample_data) {
                Close(file);
                return FALSE;
            }
        } else {
            mod->samples[i].sample_data = NULL;
            mod->samples[i].sample_length = 0;
        }
    }

    mod->song_length = header[950];
    mod->num_patterns = 0;

    CopyMem(header + 952, mod->pattern_table, 128);

    for (i = 0; i < 128; i++) {
        if (mod->pattern_table[i] > mod->num_patterns) {
            mod->num_patterns = mod->pattern_table[i];
        }
    }
    mod->num_patterns++;

    pattern_size = mod->num_patterns * 1024;
    mod->pattern_data = AllocMem(pattern_size, MEMF_PUBLIC);
    if (!mod->pattern_data) {
        Close(file);
        return FALSE;
    }

    bytes_read = Read(file, mod->pattern_data, pattern_size);
    if (bytes_read < pattern_size) {
        Close(file);
        return FALSE;
    }

    for (i = 0; i < 31; i++) {
        if (mod->samples[i].sample_data && mod->samples[i].sample_length > 0) {
            bytes_read = Read(file, mod->samples[i].sample_data, mod->samples[i].sample_length);
            if (bytes_read < mod->samples[i].sample_length) {
                Close(file);
                return FALSE;
            }
        }
    }

    Close(file);
    return TRUE;
}

BOOL play_mod(ModFile *mod)
{
    if (!mod || audio_system.playing) {
        return FALSE;
    }

    audio_system.current_mod = mod;
    audio_system.playing = TRUE;

    return TRUE;
}

void stop_mod(void)
{
    if (audio_system.playing && audio_system.audio_io) {
        audio_system.audio_io->ioa_Request.io_Command = CMD_FLUSH;
        DoIO((struct IORequest *)audio_system.audio_io);
        audio_system.playing = FALSE;
    }
}

void set_mod_volume(UBYTE volume)
{
    if (audio_system.audio_io && volume <= 64) {
        audio_system.audio_io->ioa_Volume = volume;
    }
}

void free_mod(ModFile *mod)
{
    UWORD i;
    
    if (!mod) return;

    if (mod->pattern_data) {
        FreeMem(mod->pattern_data, mod->num_patterns * 1024);
        mod->pattern_data = NULL;
    }

    for (i = 0; i < 31; i++) {
        if (mod->samples[i].sample_data) {
            FreeMem(mod->samples[i].sample_data, mod->samples[i].sample_length);
            mod->samples[i].sample_data = NULL;
        }
    }
}