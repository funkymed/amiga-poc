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
#include <exec/memory.h>

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
#define MOD_NAME "assets/yeah.mod"
// Allocate MOD data dynamically to save memory when not needed
static UBYTE *mod_data = NULL;

// Bitmap font constants - font1.png
#define FONT_WIDTH 32        // Each character is 32 pixels wide
#define FONT_HEIGHT 32       // Each character is 32 pixels high (192/6)
#define FONT_CHARS_H 10      // 10 characters per row
#define FONT_CHARS_V 6       // 6 rows of characters
#define FONT_IMAGE_WIDTH 320
#define FONT_IMAGE_HEIGHT 200

// Character position cache for faster lookup
typedef struct {
    int src_x, src_y;          // Source coordinates in font bitmap
} CharPos;

// Bitmap font structure for Blitter operations
typedef struct {
    struct BitMap *bitmap;      // BitMap structure for Blitter
    struct BitMap *mask_bitmap; // Mask bitmap for transparency
    UBYTE *font_data;          // Raw bitmap data (backup)
    PLANEPTR planes[4];        // Direct pointers to BitMap planes
    PLANEPTR mask_planes[1];   // Mask plane for transparency
    CharPos char_positions[256]; // Pre-calculated character positions
    int char_width;
    int char_height;
    int chars_per_row;
    int total_chars;
    BOOL blitter_ready;        // Flag if Blitter setup is ready
} BitmapFont;

static BitmapFont bitmap_font = {0};

// Forward declarations
void draw_bitmap_text(struct RastPort *rp, const char *text, int x, int y);

// Character mapping table for font1.png
// Layout: !"# %B°()* / +,-./01234 / 56789::<=? / @ABCDEFGHI / JKLMNOPQRS / TUVWXYZ(
static char char_map[] = {
    ' ', '!', '"', '#', ' ', '%', 'B', 'o', '(', ')',  // Row 0 (space for missing chars, o for degree)
    '*', '+', ',', '-', '.', '/', '0', '1', '2', '3',  // Row 1
    '4', '5', '6', '7', '8', '9', ':', ':', '<', '=',  // Row 2
    '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',  // Row 3
    'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q',  // Row 4
    'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '('   // Row 5
};

// Pre-calculate character positions for fast lookup
void init_char_positions(void) {
    // Initialize all positions to space character (0,0)
    for (int i = 0; i < 256; i++) {
        bitmap_font.char_positions[i].src_x = 0;
        bitmap_font.char_positions[i].src_y = 0;
    }
    
    // Map each character in char_map to its bitmap position
    for (int i = 0; i < 60; i++) { // 60 chars total (10x6)
        char c = char_map[i];
        int char_row = i / FONT_CHARS_H;
        int char_col = i % FONT_CHARS_H;
        
        bitmap_font.char_positions[(unsigned char)c].src_x = char_col * FONT_WIDTH;
        bitmap_font.char_positions[(unsigned char)c].src_y = char_row * FONT_HEIGHT;
        
        // Also map lowercase to uppercase positions
        if (c >= 'A' && c <= 'Z') {
            char lowercase = c - 'A' + 'a';
            bitmap_font.char_positions[(unsigned char)lowercase].src_x = char_col * FONT_WIDTH;
            bitmap_font.char_positions[(unsigned char)lowercase].src_y = char_row * FONT_HEIGHT;
        }
    }
}

// Find character index in bitmap font (DEPRECATED - use direct lookup now)
int get_char_index(char c) {
    // Convert to uppercase for matching
    char upper_c = (c >= 'a' && c <= 'z') ? (c - 'a' + 'A') : c;
    
    for (int i = 0; i < 60; i++) { // 60 chars total (10x6)
        if (char_map[i] == upper_c) {
            return i;
        }
    }
    
    // Return space index if character not found (position 0 in row 0)
    return 0;
}

// Convert chunky pixels to planar BitMap format (4-bit depth)
void chunky_to_planar(UBYTE *chunky_data, PLANEPTR planes[4], PLANEPTR mask_plane, 
                      int width, int height, int bytes_per_row) {
    int total_pixels = width * height;
    int words_per_row = bytes_per_row / 2;
    
    // Clear all planes first
    for (int plane = 0; plane < 4; plane++) {
        memset(planes[plane], 0, bytes_per_row * height);
    }
    memset(mask_plane, 0, bytes_per_row * height);
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            UBYTE pixel = chunky_data[y * width + x];
            
            // Calculate bit position in plane
            int byte_offset = y * bytes_per_row + (x >> 3);
            int bit_pos = 7 - (x & 7);
            UBYTE bit_mask = 1 << bit_pos;
            
            // Set mask bit for non-transparent pixels (pixel != 0)
            if (pixel != 0) {
                ((UBYTE*)mask_plane)[byte_offset] |= bit_mask;
                
                // Convert 4-bit color to 4 bitplanes
                if (pixel & 1) ((UBYTE*)planes[0])[byte_offset] |= bit_mask;
                if (pixel & 2) ((UBYTE*)planes[1])[byte_offset] |= bit_mask;
                if (pixel & 4) ((UBYTE*)planes[2])[byte_offset] |= bit_mask;
                if (pixel & 8) ((UBYTE*)planes[3])[byte_offset] |= bit_mask;
            }
        }
    }
}

// Load bitmap font and setup for Blitter operations
int load_bitmap_font(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;
    
    // Calculate memory requirements
    ULONG font_size = FONT_IMAGE_WIDTH * FONT_IMAGE_HEIGHT;
    
    // Allocate CHIP memory for font data (required for Blitter)
    bitmap_font.font_data = AllocMem(font_size, MEMF_CHIP | MEMF_CLEAR);
    if (!bitmap_font.font_data) {
        fclose(fp);
        return 0;
    }
    
    // Read raw bitmap data
    size_t bytes_read = fread(bitmap_font.font_data, 1, font_size, fp);
    fclose(fp);
    
    if (bytes_read != font_size) {
        FreeMem(bitmap_font.font_data, font_size);
        bitmap_font.font_data = NULL;
        return 0;
    }
    
    // Set up font parameters
    bitmap_font.char_width = FONT_WIDTH;
    bitmap_font.char_height = FONT_HEIGHT;
    bitmap_font.chars_per_row = FONT_CHARS_H;
    bitmap_font.total_chars = FONT_CHARS_H * FONT_CHARS_V;
    
    // Calculate BitMap parameters
    int bytes_per_row = ((FONT_IMAGE_WIDTH + 15) >> 4) << 1;  // Round up to word boundary
    ULONG plane_size = bytes_per_row * FONT_IMAGE_HEIGHT;
    
    // Allocate main BitMap structure
    bitmap_font.bitmap = AllocBitMap(FONT_IMAGE_WIDTH, FONT_IMAGE_HEIGHT, 4, BMF_CLEAR, NULL);
    if (!bitmap_font.bitmap) {
        FreeMem(bitmap_font.font_data, font_size);
        bitmap_font.font_data = NULL;
        return 0;
    }
    
    // Allocate mask BitMap for transparency
    bitmap_font.mask_bitmap = AllocBitMap(FONT_IMAGE_WIDTH, FONT_IMAGE_HEIGHT, 1, BMF_CLEAR, NULL);
    if (!bitmap_font.mask_bitmap) {
        FreeBitMap(bitmap_font.bitmap);
        FreeMem(bitmap_font.font_data, font_size);
        bitmap_font.font_data = NULL;
        bitmap_font.bitmap = NULL;
        return 0;
    }
    
    // Get plane pointers
    for (int i = 0; i < 4; i++) {
        bitmap_font.planes[i] = bitmap_font.bitmap->Planes[i];
    }
    bitmap_font.mask_planes[0] = bitmap_font.mask_bitmap->Planes[0];
    
    // Convert chunky data to planar format
    chunky_to_planar(bitmap_font.font_data, bitmap_font.planes, bitmap_font.mask_planes[0],
                     FONT_IMAGE_WIDTH, FONT_IMAGE_HEIGHT, bytes_per_row);
    
    // Pre-calculate character positions for fast lookup
    init_char_positions();
    
    bitmap_font.blitter_ready = TRUE;
    
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

// Hardware-optimized Blitter-based character drawing with transparency
void draw_bitmap_char_blitter(struct RastPort *rp, char c, int x, int y) {
    if (!bitmap_font.blitter_ready || !bitmap_font.bitmap || !bitmap_font.mask_bitmap) return;
    
    // Get character position using direct lookup (much faster!)
    CharPos *pos = &bitmap_font.char_positions[(unsigned char)c];
    
    // Wait for Blitter to be free
    WaitBlit();
    
    // Use BltMaskBitMapRastPort for hardware-accelerated copy with transparency
    // This function uses the mask bitmap to preserve background pixels where mask = 0
    BltMaskBitMapRastPort(bitmap_font.bitmap, pos->src_x, pos->src_y, rp, x, y, 
                          FONT_WIDTH, FONT_HEIGHT, 
                          (ABC|ABNC|ANBC), bitmap_font.mask_bitmap->Planes[0]);
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

// Character drawing - use hardware acceleration when available
void draw_bitmap_char(struct RastPort *rp, char c, int x, int y) {
    // Use hardware-accelerated Blitter rendering when available
    if (bitmap_font.blitter_ready && bitmap_font.bitmap && bitmap_font.mask_bitmap) {
        draw_bitmap_char_blitter(rp, c, x, y);
    } else {
        // Fallback to software rendering
        draw_bitmap_char_software(rp, c, x, y);
    }
}

// Optimized text rendering with batch Blitter operations
void draw_bitmap_text_optimized(struct RastPort *rp, const char *text, int x, int y) {
    if (!bitmap_font.blitter_ready || !bitmap_font.bitmap || !bitmap_font.mask_bitmap) {
        // Fallback to software rendering
        draw_bitmap_text(rp, text, x, y);
        return;
    }
    
    int current_x = x;
    int len = strlen(text);
    
    // Wait for Blitter once at the start
    WaitBlit();
    
    // Render all characters using hardware acceleration
    for (int i = 0; i < len; i++) {
        CharPos *pos = &bitmap_font.char_positions[(unsigned char)text[i]];
        
        // Use BltMaskBitMapRastPort for each character
        BltMaskBitMapRastPort(bitmap_font.bitmap, pos->src_x, pos->src_y, rp, current_x, y, 
                              FONT_WIDTH, FONT_HEIGHT, 
                              (ABC|ABNC|ANBC), bitmap_font.mask_bitmap->Planes[0]);
        
        current_x += FONT_WIDTH;
    }
}

// Draw text using bitmap font (software fallback)
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
    load_bitmap_font("assets/font1.raw");
    
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
    
    // Draw text with bitmap font
    if (bitmap_font.font_data) {
        // TEST: Draw a simple character first to debug
        // Try drawing 'Y' character at fixed position
        draw_bitmap_char(rp, 'Y', 300, 100);
        draw_bitmap_char(rp, 'E', 330, 100);
        draw_bitmap_char(rp, 'A', 360, 100);
        draw_bitmap_char(rp, 'H', 390, 100);
        
        // Draw the full text using optimized Blitter rendering
        draw_bitmap_text_optimized(rp, text, text_x, text_y);
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
    if (bitmap_font.bitmap || bitmap_font.mask_bitmap) {
        WaitBlit(); // Ensure no pending Blitter operations
        
        if (bitmap_font.bitmap) {
            FreeBitMap(bitmap_font.bitmap);
            bitmap_font.bitmap = NULL;
        }
        if (bitmap_font.mask_bitmap) {
            FreeBitMap(bitmap_font.mask_bitmap);
            bitmap_font.mask_bitmap = NULL;
        }
    }
    if (bitmap_font.font_data) {
        FreeMem(bitmap_font.font_data, FONT_IMAGE_WIDTH * FONT_IMAGE_HEIGHT);
        bitmap_font.font_data = NULL;
    }
    bitmap_font.blitter_ready = FALSE;
    
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