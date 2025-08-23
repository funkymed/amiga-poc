#include "amiga_base.h"
#include <proto/dos.h>
#include <proto/exec.h>

BOOL load_image(const char *filename, AmigaImage *image)
{
    BPTR file;
    LONG file_size;
    UBYTE header[54];
    LONG bytes_read;
    UWORD i, j;
    UBYTE *temp_buffer;

    file = Open((STRPTR)filename, MODE_OLDFILE);
    if (!file) {
        return FALSE;
    }

    bytes_read = Read(file, header, 54);
    if (bytes_read < 54 || header[0] != 'B' || header[1] != 'M') {
        Close(file);
        return FALSE;
    }

    image->width = *(ULONG*)(header + 18);
    image->height = *(ULONG*)(header + 22);
    image->depth = 8;

    if (image->width > SCREEN_WIDTH || image->height > SCREEN_HEIGHT) {
        Close(file);
        return FALSE;
    }

    file_size = image->width * image->height;
    image->data = AllocMem(file_size, MEMF_CHIP | MEMF_CLEAR);
    if (!image->data) {
        Close(file);
        return FALSE;
    }

    temp_buffer = AllocMem(file_size * 3, MEMF_PUBLIC);
    if (!temp_buffer) {
        FreeMem(image->data, file_size);
        Close(file);
        return FALSE;
    }

    Seek(file, 54, OFFSET_BEGINNING);
    bytes_read = Read(file, temp_buffer, file_size * 3);
    Close(file);

    for (i = 0; i < image->height; i++) {
        for (j = 0; j < image->width; j++) {
            ULONG pixel_offset = (i * image->width + j) * 3;
            UBYTE r = temp_buffer[pixel_offset + 2];
            UBYTE g = temp_buffer[pixel_offset + 1];
            UBYTE b = temp_buffer[pixel_offset];
            
            UBYTE color_index = ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6);
            image->data[(image->height - 1 - i) * image->width + j] = color_index;
        }
    }

    FreeMem(temp_buffer, file_size * 3);
    return TRUE;
}

void display_image(AmigaImage *image, WORD x, WORD y)
{
    UWORD i, j;
    
    if (!image || !image->data) return;

    for (i = 0; i < image->height; i++) {
        for (j = 0; j < image->width; j++) {
            if (x + j >= 0 && x + j < SCREEN_WIDTH && 
                y + i >= 0 && y + i < SCREEN_HEIGHT) {
                UBYTE color = image->data[i * image->width + j];
                if (color != 0) {
                    set_pixel(x + j, y + i, color);
                }
            }
        }
    }
}

void free_image(AmigaImage *image)
{
    if (image && image->data) {
        FreeMem(image->data, image->width * image->height);
        image->data = NULL;
    }
}