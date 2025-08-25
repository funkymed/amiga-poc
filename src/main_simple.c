/**
 * example_01.c - Simple example using the ptplayer library
 * to play a Protracker 2 MOD file.
 */
#include <hardware/custom.h>
#include <graphics/gfxbase.h>
#include <devices/input.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <string.h>

// Download from Aminet
// http://aminet.net/package/dev/c/SDI_headers
// http://aminet.net/package/mus/play/ptplayer
#include <SDI/SDI_compiler.h>
#include "ptplayer/ptplayer.h"

extern struct GfxBase *GfxBase;
extern struct IntuitionBase *IntuitionBase;
extern struct Custom custom;

struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;

// To handle input
static struct MsgPort *input_mp;
static struct IOStdReq *input_io;
static struct Interrupt handler_info;
static int should_exit;


static struct InputEvent *my_input_handler(__reg("a0") struct InputEvent *event,
                                           __reg("a1") APTR handler_data)
{
    struct InputEvent *result = event, *prev = NULL;

    Forbid();
    // Intercept all raw mouse events before they reach Intuition, ignore
    // everything else
    if (result->ie_Class == IECLASS_RAWMOUSE) {
        if (result->ie_Code == IECODE_LBUTTON) {
            should_exit = 1;
        }
        return NULL;
    }
    Permit();
    return result;
}

static void cleanup_input_handler(void)
{
    if (input_io) {
        // remove our input handler from the chain
        input_io->io_Command = IND_REMHANDLER;
        input_io->io_Data = (APTR) &handler_info;
        DoIO((struct IORequest *) input_io);

        if (!(CheckIO((struct IORequest *) input_io))) AbortIO((struct IORequest *) input_io);
        WaitIO((struct IORequest *) input_io);
        CloseDevice((struct IORequest *) input_io);
        DeleteExtIO((struct IORequest *) input_io);
    }
    if (input_mp) DeletePort(input_mp);
}

static int setup_input_handler(void)
{
    input_mp = CreatePort(0, 0);
    input_io = (struct IOStdReq *) CreateExtIO(input_mp, sizeof(struct IOStdReq));
    BYTE error = OpenDevice("input.device", 0L, (struct IORequest *) input_io, 0);

    handler_info.is_Code = (void (*)(void)) my_input_handler;
    handler_info.is_Data = NULL;
    handler_info.is_Node.ln_Pri = 100;
    handler_info.is_Node.ln_Name = "ex03";
    input_io->io_Command = IND_ADDHANDLER;
    input_io->io_Data = (APTR) &handler_info;
    DoIO((struct IORequest *) input_io);
    return 1;
}

#define MOD_SIZE (214004)
#define MOD_NAME "yeah.mod"
static UBYTE __chip mod_data[MOD_SIZE];

int main(int argc, char **argv)
{
    // Open libraries
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase) return 20;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    // Close Workbench
    CloseWorkBench();
    
    // Create custom screen 640x256 in 16 colors
    struct NewScreen ns = {
        0, 0, 640, 256, 4,  // left, top, width, height, depth (4-bit = 16 colors)
        0, 1,               // detail pen, block pen
        HIRES_KEY,          // view modes (640 pixels wide = HIRES)
        CUSTOMSCREEN | SCREENQUIET, // screen type with no title bar
        NULL,               // font
        NULL,               // title
        NULL,               // gadgets
        NULL                // custom bitmap
    };
    
    struct Screen *screen = OpenScreen(&ns);
    if (!screen) {
        OpenWorkBench();
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    
    // Create window on custom screen
    struct NewWindow nw = {
        0, 0, 640, 256,     // left, top, width, height
        0, 1,               // detail pen, block pen
        MOUSEBUTTONS,       // IDCMP flags
        BORDERLESS | ACTIVATE, // flags
        NULL,               // first gadget
        NULL,               // checkmark
        "",                 // title
        screen,             // screen
        NULL,               // bitmap
        0, 0, 0, 0,         // min/max width/height
        CUSTOMSCREEN        // type
    };
    
    struct Window *window = OpenWindow(&nw);
    if (!window) {
        CloseScreen(screen);
        OpenWorkBench();
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    
    // Hide mouse pointer with blank sprite
    static UWORD __chip blank_pointer[] = {
        0x0000, 0x0000, // Row 1
        0x0000, 0x0000, // Row 2
        0x0000, 0x0000  // End marker
    };
    SetPointer(window, blank_pointer, 1, 1, 0, 0);
    
    // Initialize rainbow palette
    struct ViewPort *vp = &screen->ViewPort;
    
    // Set up initial rainbow colors (colors 2-15 for rainbow, 0=black, 1=white for text)
    SetRGB4(vp, 0, 0, 0, 0);    // Black
    SetRGB4(vp, 1, 15, 15, 15); // White for text
    
    // Initialize rainbow colors
    SetRGB4(vp, 2, 15, 0, 0);   // Red
    SetRGB4(vp, 3, 15, 8, 0);   // Orange
    SetRGB4(vp, 4, 15, 15, 0);  // Yellow
    SetRGB4(vp, 5, 8, 15, 0);   // Yellow-green
    SetRGB4(vp, 6, 0, 15, 0);   // Green
    SetRGB4(vp, 7, 0, 15, 8);   // Green-cyan
    SetRGB4(vp, 8, 0, 15, 15);  // Cyan
    SetRGB4(vp, 9, 0, 8, 15);   // Cyan-blue
    SetRGB4(vp, 10, 0, 0, 15);  // Blue
    SetRGB4(vp, 11, 8, 0, 15);  // Blue-violet
    SetRGB4(vp, 12, 15, 0, 15); // Violet
    SetRGB4(vp, 13, 15, 0, 8);  // Violet-red
    SetRGB4(vp, 14, 8, 8, 8);   // Gray
    SetRGB4(vp, 15, 4, 4, 4);   // Dark gray
    
    // Draw rainbow background
    struct RastPort *rp = window->RPort;
    int stripe_height = 256 / 12; // 12 color stripes
    
    for (int i = 0; i < 12; i++) {
        SetAPen(rp, 2 + i); // Use colors 2-13
        RectFill(rp, 0, i * stripe_height, 639, (i + 1) * stripe_height - 1);
    }
    
    // Display centered text "yeah by med/mandarine" in white
    SetAPen(rp, 1); // White pen
    SetBPen(rp, 0); // Transparent background
    SetDrMd(rp, JAM1); // Only foreground color
    
    char *text = "yeah by med/mandarine";
    int text_len = strlen(text);
    int char_width = 8; // Assume 8 pixels per character
    int text_width = text_len * char_width;
    int text_x = (640 - text_width) / 2;  // Center horizontally
    int text_y = 256 / 2;                 // Center vertically
    
    Move(rp, text_x, text_y);
    Text(rp, text, text_len);

    if (!setup_input_handler()) {
        CloseWindow(window);
        CloseScreen(screen);
        OpenWorkBench();
        puts("Could not initialize input handler");
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    FILE *fp = fopen(MOD_NAME, "rb");
    int bytes_read = fread(mod_data, sizeof(UBYTE), MOD_SIZE, fp);
    fclose(fp);
    BOOL is_pal = (((struct GfxBase *) GfxBase)->DisplayFlags & PAL) == PAL;

    // initialize ptplayer
    void *p_samples = NULL;
    UBYTE start_pos = 0;
    mt_install_cia(&custom, NULL, is_pal);
    mt_init(&custom, mod_data, p_samples, start_pos);
    mt_Enable = 1;

    // Animation variables for color cycling
    int color_offset = 0;
    int frame_counter = 0;

    while (!should_exit) {
        WaitTOF();
        
        // Animate colors every 4 frames for smoother animation
        if (++frame_counter >= 4) {
            frame_counter = 0;
            color_offset = (color_offset + 1) % 12;
            
            // Cycle rainbow colors through the palette
            for (int i = 0; i < 12; i++) {
                int color_index = (i + color_offset) % 12;
                
                // Rainbow color table
                static int rainbow_colors[12][3] = {
                    {15, 0, 0},   // Red
                    {15, 8, 0},   // Orange
                    {15, 15, 0},  // Yellow
                    {8, 15, 0},   // Yellow-green
                    {0, 15, 0},   // Green
                    {0, 15, 8},   // Green-cyan
                    {0, 15, 15},  // Cyan
                    {0, 8, 15},   // Cyan-blue
                    {0, 0, 15},   // Blue
                    {8, 0, 15},   // Blue-violet
                    {15, 0, 15},  // Violet
                    {15, 0, 8}    // Violet-red
                };
                
                SetRGB4(vp, 2 + i, 
                       rainbow_colors[color_index][0],
                       rainbow_colors[color_index][1], 
                       rainbow_colors[color_index][2]);
            }
        }
    }

    // Cleanup
    mt_Enable = 0;
    mt_end(&custom);
    mt_remove_cia(&custom);
    cleanup_input_handler();
    
    // Close window and screen
    CloseWindow(window);
    CloseScreen(screen);
    
    // Reopen Workbench
    OpenWorkBench();
    
    // Close libraries
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    return 0;
}