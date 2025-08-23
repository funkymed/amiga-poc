#include "amiga_base.h"
#include <proto/graphics.h>

AmigaDisplay display;

BOOL init_display(void)
{
    display.screen = OpenScreenTags(NULL,
        SA_Width, SCREEN_WIDTH,
        SA_Height, SCREEN_HEIGHT,
        SA_Depth, SCREEN_DEPTH,
        SA_DisplayID, LORES_KEY,
        SA_Title, "Amiga Demo",
        TAG_END);

    if (!display.screen) {
        return FALSE;
    }

    display.window = OpenWindowTags(NULL,
        WA_CustomScreen, display.screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, SCREEN_WIDTH,
        WA_Height, SCREEN_HEIGHT,
        WA_Backdrop, TRUE,
        WA_Borderless, TRUE,
        WA_Activate, TRUE,
        WA_RMBTrap, TRUE,
        TAG_END);

    if (!display.window) {
        CloseScreen(display.screen);
        return FALSE;
    }

    display.rastport = display.window->RPort;
    display.bitmap = display.rastport->BitMap;

    SetRast(display.rastport, 0);
    
    return TRUE;
}

void cleanup_display(void)
{
    if (display.window) {
        CloseWindow(display.window);
        display.window = NULL;
    }
    if (display.screen) {
        CloseScreen(display.screen);
        display.screen = NULL;
    }
}

void set_pixel(WORD x, WORD y, UBYTE color)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        SetAPen(display.rastport, color);
        WritePixel(display.rastport, x, y);
    }
}

UBYTE get_pixel(WORD x, WORD y)
{
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        return ReadPixel(display.rastport, x, y);
    }
    return 0;
}

void draw_line(WORD x1, WORD y1, WORD x2, WORD y2, UBYTE color)
{
    SetAPen(display.rastport, color);
    Move(display.rastport, x1, y1);
    Draw(display.rastport, x2, y2);
}

void clear_screen(UBYTE color)
{
    SetRast(display.rastport, color);
}