#ifndef AMIGA_BASE_H
#define AMIGA_BASE_H

#include <exec/types.h>
#include <intuition/intuition.h>
#include <graphics/gfx.h>
#include <graphics/view.h>
#include <graphics/rastport.h>
#include <graphics/gfxmacros.h>
#include <libraries/dos.h>
#include <devices/audio.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/dos.h>

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define SCREEN_DEPTH 5

typedef struct {
    struct Screen *screen;
    struct Window *window;
    struct RastPort *rastport;
    struct BitMap *bitmap;
} AmigaDisplay;

typedef struct {
    UBYTE *data;
    UWORD width;
    UWORD height;
    UBYTE depth;
} AmigaImage;

typedef struct {
    UBYTE *sample_data;
    ULONG sample_length;
    UWORD sample_rate;
    UBYTE volume;
} ModSample;

typedef struct {
    char name[21];
    UBYTE num_patterns;
    UBYTE song_length;
    UBYTE pattern_table[128];
    ModSample samples[31];
    UBYTE *pattern_data;
} ModFile;

typedef struct {
    struct IOAudio *audio_io;
    struct MsgPort *audio_port;
    UBYTE audio_device_open;
    ModFile *current_mod;
    UBYTE playing;
} AudioSystem;

extern AmigaDisplay display;
extern AudioSystem audio_system;

BOOL init_amiga_system(void);
void cleanup_amiga_system(void);

BOOL load_image(const char *filename, AmigaImage *image);
void display_image(AmigaImage *image, WORD x, WORD y);
void set_pixel(WORD x, WORD y, UBYTE color);
UBYTE get_pixel(WORD x, WORD y);
void draw_line(WORD x1, WORD y1, WORD x2, WORD y2, UBYTE color);
void clear_screen(UBYTE color);

BOOL load_mod_file(const char *filename, ModFile *mod);
BOOL play_mod(ModFile *mod);
void stop_mod(void);
void set_mod_volume(UBYTE volume);

#endif