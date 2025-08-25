/**
 * example_01.c - Simple example using the ptplayer library
 * to play a Protracker 2 MOD file.
 */
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/blit.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/gels.h>
#include <graphics/copper.h>
#include <graphics/videocontrol.h>
#include <devices/input.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/alib_protos.h>
#include <hardware/blit.h>

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

// Simple globals with Copper support
static int animation_frame = 0;
static UWORD *copper_list = NULL;
static ULONG copper_list_size = 0;

// Optimized input handling
static struct MsgPort *input_mp;
static struct IOStdReq *input_io;
static struct Interrupt handler_info;
static volatile int should_exit;
static volatile ULONG last_input_time = 0;


// Optimized input handler with reduced CPU overhead
static struct InputEvent *my_input_handler(__reg("a0") struct InputEvent *event,
                                           __reg("a1") APTR handler_data)
{
    register struct InputEvent *current = event;
    ULONG current_time;
    
    // Quick exit if no events
    if (!current) return event;
    
    // Get current time for debouncing
    current_time = current->ie_TimeStamp.tv_secs;
    
    // Process events in chain efficiently
    while (current) {
        switch (current->ie_Class) {
            case IECLASS_RAWMOUSE:
                if (current->ie_Code == IECODE_LBUTTON) {
                    // Debounce input (prevent multiple rapid clicks)
                    if (current_time != last_input_time) {
                        should_exit = 1;
                        last_input_time = current_time;
                    }
                }
                // Remove mouse events to prevent Intuition processing
                return NULL;
                
            case IECLASS_RAWKEY:
                // Handle ESC key for alternative exit
                if (current->ie_Code == 0x45) { // ESC key
                    should_exit = 1;
                    return NULL;
                }
                break;
        }
        current = current->ie_NextEvent;
    }
    
    return event;
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

// Optimized input handler setup
static int setup_input_handler(void)
{
    input_mp = CreatePort(0, 0);
    if (!input_mp) return 0;
    
    input_io = (struct IOStdReq *) CreateExtIO(input_mp, sizeof(struct IOStdReq));
    if (!input_io) {
        DeletePort(input_mp);
        return 0;
    }
    
    BYTE error = OpenDevice("input.device", 0L, (struct IORequest *) input_io, 0);
    if (error != 0) {
        DeleteExtIO((struct IORequest *) input_io);
        DeletePort(input_mp);
        return 0;
    }

    handler_info.is_Code = (void (*)(void)) my_input_handler;
    handler_info.is_Data = NULL;
    handler_info.is_Node.ln_Pri = 51; // Lower priority for less CPU impact
    handler_info.is_Node.ln_Name = "OptimizedDemo";
    
    input_io->io_Command = IND_ADDHANDLER;
    input_io->io_Data = (APTR) &handler_info;
    
    if (DoIO((struct IORequest *) input_io) != 0) {
        CloseDevice((struct IORequest *) input_io);
        DeleteExtIO((struct IORequest *) input_io);
        DeletePort(input_mp);
        return 0;
    }
    
    return 1;
}

#define MOD_SIZE (214004)
#define MOD_NAME "yeah.mod"
// Allocate MOD data dynamically to save memory when not needed
static UBYTE *mod_data = NULL;

// Bitmap font constants
#define FONT_WIDTH 32        // Each character is 32 pixels wide (320/10)
#define FONT_HEIGHT 25       // Each character is 25 pixels high (200/8)
#define FONT_CHARS_H 10      // 10 characters per row
#define FONT_CHARS_V 8       // 8 rows of characters
#define FONT_IMAGE_WIDTH 320
#define FONT_IMAGE_HEIGHT 200

// Bitmap font structure
typedef struct {
    UBYTE *font_data;           // Raw bitmap data
    int char_width;
    int char_height;
    int chars_per_row;
    int total_chars;
} BitmapFont;

static BitmapFont bitmap_font;

// Character mapping table for bitmap font
// Layout: ABCDEFGHIJ / KLMNOPQRST / UVWXYZ!?:; / 0123456789 / *()=?.'°_" / ,-
static char char_map[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',  // Row 0
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',  // Row 1  
    'U', 'V', 'W', 'X', 'Y', 'Z', '!', '?', ':', ';',  // Row 2
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',  // Row 3
    '*', '(', ')', '=', '?', '.', '\'', '#', '_', '"', // Row 4
    ',', '-', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '   // Row 5 (with spaces for padding)
};

// Find character index in bitmap font
int get_char_index(char c) {
    // Convert to uppercase for matching
    char upper_c = (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
    
    for (int i = 0; i < sizeof(char_map); i++) {
        if (char_map[i] == upper_c) {
            return i;
        }
    }
    
    // Return space index if character not found
    return 52; // Position of first space in row 5
}

// Optimized bitmap font loading with smart memory allocation
int load_bitmap_font(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;
    
    // Calculate memory requirements
    ULONG font_size = FONT_IMAGE_WIDTH * FONT_IMAGE_HEIGHT;
    
    // Try FAST memory first (if available) for better performance
    bitmap_font.font_data = AllocMem(font_size, MEMF_FAST | MEMF_CLEAR);
    if (!bitmap_font.font_data) {
        // Fallback to CHIP memory if FAST memory not available
        bitmap_font.font_data = AllocMem(font_size, MEMF_CHIP | MEMF_CLEAR);
        if (!bitmap_font.font_data) {
            fclose(fp);
            return 0;
        }
    }
    
    // Read raw bitmap data in larger chunks for better performance
    size_t bytes_read = 0;
    size_t chunk_size = 8192; // 8KB chunks
    UBYTE *buffer_ptr = bitmap_font.font_data;
    
    while (bytes_read < font_size) {
        size_t to_read = (font_size - bytes_read < chunk_size) ? (font_size - bytes_read) : chunk_size;
        size_t read_bytes = fread(buffer_ptr, 1, to_read, fp);
        if (read_bytes == 0) break;
        
        bytes_read += read_bytes;
        buffer_ptr += read_bytes;
    }
    
    fclose(fp);
    
    if (bytes_read != font_size) {
        FreeMem(bitmap_font.font_data, font_size);
        bitmap_font.font_data = NULL;
        return 0;
    }
    
    bitmap_font.char_width = FONT_WIDTH;
    bitmap_font.char_height = FONT_HEIGHT;
    bitmap_font.chars_per_row = FONT_CHARS_H;
    bitmap_font.total_chars = FONT_CHARS_H * FONT_CHARS_V;
    
    return 1;
}

// Simple setup - disable Copper for now
void setup_copper_rainbow(struct ViewPort *vp) {
    // Mark as disabled
    copper_list = NULL;
}



// Simple background rendering - back to working version
void draw_rainbow_background(struct RastPort *rp) {
    int stripe_height = 256 / 12;
    
    for (int i = 0; i < 12; i++) {
        SetAPen(rp, 2 + i);
        SetDrMd(rp, JAM1);
        RectFill(rp, 0, i * stripe_height, 639, (i + 1) * stripe_height - 1);
    }
}

// Update colors the simple way
void update_colors(int offset, struct ViewPort *vp) {
    static struct { UBYTE r, g, b; } rainbow_colors[12] = {
        {15, 0, 0}, {15, 8, 0}, {15, 15, 0}, {8, 15, 0},
        {0, 15, 0}, {0, 15, 8}, {0, 15, 15}, {0, 8, 15},
        {0, 0, 15}, {8, 0, 15}, {15, 0, 15}, {15, 0, 8}
    };
    
    for (int i = 0; i < 12; i++) {
        int color_index = (i + offset) % 12;
        SetRGB4(vp, 2 + i, 
                rainbow_colors[color_index].r,
                rainbow_colors[color_index].g,
                rainbow_colors[color_index].b);
    }
}

// Hardware-optimized Blitter-based character drawing
void draw_bitmap_char_blitter(struct RastPort *rp, char c, int x, int y) {
    if (!bitmap_font.font_data) return;
    
    // Get character index from mapping table
    int char_index = get_char_index(c);
    
    int char_row = char_index / FONT_CHARS_H;
    int char_col = char_index % FONT_CHARS_H;
    
    int src_x = char_col * FONT_WIDTH;
    int src_y = char_row * FONT_HEIGHT;
    
    // Wait for Blitter to be free
    WaitBlit();
    
    // Set up source bitmap structure for font data
    struct BitMap src_bitmap;
    InitBitMap(&src_bitmap, 8, FONT_IMAGE_WIDTH, FONT_IMAGE_HEIGHT);
    src_bitmap.Planes[0] = bitmap_font.font_data;
    for (int i = 1; i < 8; i++) {
        src_bitmap.Planes[i] = NULL; // Only using plane 0 for 8-bit chunky data
    }
    
    struct Rectangle src_rect = {src_x, src_y, src_x + FONT_WIDTH - 1, src_y + FONT_HEIGHT - 1};
    
    // Use BltBitMapRastPort for hardware-accelerated copy with transparency
    BltBitMapRastPort(&src_bitmap, src_x, src_y, rp, x, y, FONT_WIDTH, FONT_HEIGHT, 0xC0);
}

// Simple bitmap character drawing - basic and reliable
void draw_bitmap_char_software(struct RastPort *rp, char c, int x, int y) {
    if (!bitmap_font.font_data) return;
    
    // Get character index from mapping table
    int char_index = get_char_index(c);
    
    int char_row = char_index / FONT_CHARS_H;
    int char_col = char_index % FONT_CHARS_H;
    
    int src_x = char_col * FONT_WIDTH;
    int src_y = char_row * FONT_HEIGHT;
    
    // Simple pixel-by-pixel rendering with palette colors
    SetDrMd(rp, JAM1); // Only draw foreground pixels
    
    for (int dy = 0; dy < FONT_HEIGHT; dy++) {
        for (int dx = 0; dx < FONT_WIDTH; dx++) {
            int src_offset = (src_y + dy) * FONT_IMAGE_WIDTH + (src_x + dx);
            if (src_offset < FONT_IMAGE_WIDTH * FONT_IMAGE_HEIGHT) {
                UBYTE pixel = bitmap_font.font_data[src_offset];
                
                // Only draw non-transparent pixels (0 = transparent)
                if (pixel != 0) {
                    SetAPen(rp, pixel);  // Use the actual color index from bitmap
                    WritePixel(rp, x + dx, y + dy);
                }
            }
        }
    }
}

// Simple character drawing - software only for compatibility
void draw_bitmap_char(struct RastPort *rp, char c, int x, int y) {
    // Always use software rendering to avoid conflicts
    draw_bitmap_char_software(rp, c, x, y);
}

// Draw text using bitmap font
void draw_bitmap_text(struct RastPort *rp, const char *text, int x, int y) {
    int current_x = x;
    int len = strlen(text);
    
    for (int i = 0; i < len; i++) {
        draw_bitmap_char(rp, text[i], current_x, y);
        current_x += FONT_WIDTH;
    }
}

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
    
    // Load bitmap font
    load_bitmap_font("assets/bitmap.raw");
    
    // Initialize rainbow palette FIRST
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
    
    // Initialize Copper rainbow effect
    setup_copper_rainbow(&screen->ViewPort);
    
    // Draw simple rainbow background
    struct RastPort *rp = window->RPort;
    
    // Draw rainbow background with RectFill (working version)
    draw_rainbow_background(rp);
    
    // Display centered text "yeah by med/mandarine" using bitmap font
    char *text = "yeah by med/mandarine";
    int text_len = strlen(text);
    int text_width = text_len * FONT_WIDTH;
    int text_x = (640 - text_width) / 2;  // Center horizontally
    int text_y = (256 - FONT_HEIGHT) / 2; // Center vertically
    
    // Test: Draw simple white pixels first to verify rendering works
    SetAPen(rp, 1); // White
    SetDrMd(rp, JAM1);
    
    // Draw a test pattern - small white rectangle
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 10; j++) {
            WritePixel(rp, text_x + i, text_y + j);
        }
    }
    
    // Draw text with bitmap font
    if (bitmap_font.font_data) {
        draw_bitmap_text(rp, text, text_x + 30, text_y);
    } else {
        // Fallback to system font if bitmap font failed to load
        SetAPen(rp, 1); // White pen
        SetBPen(rp, 0); // Transparent background
        SetDrMd(rp, JAM1); // Only foreground color
        Move(rp, text_x, text_y + 20);
        Text(rp, text, text_len);
    }

    if (!setup_input_handler()) {
        CloseWindow(window);
        CloseScreen(screen);
        OpenWorkBench();
        // Error: Could not initialize input handler
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    // Allocate CHIP memory for MOD data (required by Paula audio hardware)
    mod_data = (UBYTE *)AllocMem(MOD_SIZE, MEMF_CHIP | MEMF_CLEAR);
    if (!mod_data) {
        cleanup_input_handler();
        CloseWindow(window);
        CloseScreen(screen);
        OpenWorkBench();
        // Error: Could not allocate memory for MOD data
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    
    FILE *fp = fopen(MOD_NAME, "rb");
    if (!fp) {
        FreeMem(mod_data, MOD_SIZE);
        cleanup_input_handler();
        CloseWindow(window);
        CloseScreen(screen);
        OpenWorkBench();
        // Error: Could not open MOD file
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    
    int bytes_read = fread(mod_data, sizeof(UBYTE), MOD_SIZE, fp);
    fclose(fp);
    
    if (bytes_read != MOD_SIZE) {
        FreeMem(mod_data, MOD_SIZE);
        cleanup_input_handler();
        CloseWindow(window);
        CloseScreen(screen);
        OpenWorkBench();
        // Error: Could not read complete MOD file
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 1;
    }
    BOOL is_pal = (((struct GfxBase *) GfxBase)->DisplayFlags & PAL) == PAL;

    // initialize ptplayer
    void *p_samples = NULL;
    UBYTE start_pos = 0;
    mt_install_cia(&custom, NULL, is_pal);
    mt_init(&custom, mod_data, p_samples, start_pos);
    mt_Enable = 1;

    // Animation variables for hardware color cycling
    int color_offset = 0;
    int frame_counter = 0;
    ULONG vblank_count = 0;

    while (!should_exit) {
        // Precise VBlank synchronization
        WaitTOF();
        vblank_count++;
        
        // Hardware-accelerated color cycling every 3 frames (50Hz/3 ≈ 16.7Hz)
        if (++frame_counter >= 3) {
            frame_counter = 0;
            color_offset = (color_offset + 1) % 12;
            
            // Use simple color cycling
            update_colors(color_offset, &screen->ViewPort);
        }
        
        // No double buffering - keep it simple
    }

    // Cleanup
    mt_Enable = 0;
    mt_end(&custom);
    mt_remove_cia(&custom);
    cleanup_input_handler();
    
    // Cleanup Copper list
    if (copper_list) {
        WaitTOF();
        LoadView(GfxBase->ActiView);  // Restore system view
        FreeMem(copper_list, copper_list_size);
        copper_list = NULL;
    }
    
    // Free MOD data memory
    if (mod_data) {
        FreeMem(mod_data, MOD_SIZE);
        mod_data = NULL;
    }
    
    // Free bitmap font memory
    if (bitmap_font.font_data) {
        FreeMem(bitmap_font.font_data, FONT_IMAGE_WIDTH * FONT_IMAGE_HEIGHT);
        bitmap_font.font_data = NULL;
    }
    
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