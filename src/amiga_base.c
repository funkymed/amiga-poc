#include "amiga_base.h"

extern BOOL init_display(void);
extern void cleanup_display(void);
extern BOOL init_audio(void);
extern void cleanup_audio(void);

BOOL init_amiga_system(void)
{
    if (!init_display()) {
        return FALSE;
    }

    if (!init_audio()) {
        cleanup_display();
        return FALSE;
    }

    return TRUE;
}

void cleanup_amiga_system(void)
{
    cleanup_audio();
    cleanup_display();
}