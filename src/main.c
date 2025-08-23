#include "amiga_base.h"
#include <proto/intuition.h>
#include <proto/exec.h>

extern void free_image(AmigaImage *image);
extern void free_mod(ModFile *mod);

int main(void)
{
    AmigaImage test_image;
    ModFile test_mod;
    ULONG signals;
    BOOL running = TRUE;
    struct IntuiMessage *msg;
    WORD x = 160, y = 128;

    if (!init_amiga_system()) {
        Printf("Erreur: Impossible d'initialiser le système\n");
        return 20;
    }

    Printf("Système Amiga initialisé avec succès!\n");

    clear_screen(0);

    for (int i = 0; i < 32; i++) {
        SetRGB4(&display.screen->ViewPort, i, 
                (i & 16) ? 15 : 0,
                (i & 8) ? 15 : 0, 
                (i & 4) ? 15 : 0);
    }

    Printf("Test de manipulation de pixels...\n");
    for (int i = 0; i < 100; i++) {
        set_pixel(100 + i, 100, 31);
        set_pixel(100, 100 + i, 31);
    }

    Printf("Test de tracé de lignes...\n");
    draw_line(50, 50, 150, 100, 15);
    draw_line(50, 100, 150, 50, 31);

    Printf("Tentative de chargement d'une image...\n");
    if (load_image("test.bmp", &test_image)) {
        Printf("Image chargée: %dx%d\n", test_image.width, test_image.height);
        display_image(&test_image, 200, 50);
    } else {
        Printf("Impossible de charger test.bmp\n");
    }

    Printf("Tentative de chargement d'un module MOD...\n");
    if (load_mod_file("test.mod", &test_mod)) {
        Printf("Module chargé: %s\n", test_mod.name);
        Printf("Longueur du morceau: %d\n", test_mod.song_length);
        Printf("Nombre de patterns: %d\n", test_mod.num_patterns);
        
        if (play_mod(&test_mod)) {
            Printf("Lecture du module démarrée\n");
        }
    } else {
        Printf("Impossible de charger test.mod\n");
    }

    Printf("Appuyez sur une touche ou cliquez pour quitter...\n");

    while (running) {
        signals = Wait(1L << display.window->UserPort->mp_SigBit | SIGBREAKF_CTRL_C);

        if (signals & SIGBREAKF_CTRL_C) {
            running = FALSE;
        }

        while ((msg = (struct IntuiMessage *)GetMsg(display.window->UserPort))) {
            switch (msg->Class) {
                case IDCMP_MOUSEBUTTONS:
                    if (msg->Code == SELECTDOWN) {
                        x = msg->MouseX;
                        y = msg->MouseY;
                        set_pixel(x, y, 31);
                        Printf("Pixel défini à (%d,%d)\n", x, y);
                    }
                    running = FALSE;
                    break;

                case IDCMP_RAWKEY:
                    running = FALSE;
                    break;

                case IDCMP_CLOSEWINDOW:
                    running = FALSE;
                    break;
            }
            ReplyMsg((struct Message *)msg);
        }

        Delay(1);
    }

    Printf("Nettoyage...\n");
    
    if (test_mod.pattern_data) {
        stop_mod();
        free_mod(&test_mod);
    }
    
    if (test_image.data) {
        free_image(&test_image);
    }

    cleanup_amiga_system();
    
    Printf("Programme terminé.\n");
    return 0;
}