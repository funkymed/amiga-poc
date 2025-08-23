/* Hello World écran natif complet pour Amiga */
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/exec.h>

struct IntuitionBase *IntuitionBase;
struct GfxBase *GfxBase;

int main() {
    struct Screen *screen;
    struct Window *window;
    struct RastPort *rp;
    struct IntuiMessage *msg;
    BOOL running = TRUE;
    
    /* Ouvrir les librairies */
    IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
    if (!IntuitionBase) return 20;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase) {
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    /* Fermer le Workbench pour avoir l'écran complet */
    CloseWorkBench();
    
    /* Créer un écran natif 320x256 */
    screen = OpenScreenTags(NULL,
        SA_Width, 320,
        SA_Height, 256,
        SA_Depth, 4,
        SA_DisplayID, LORES_KEY,
        SA_Type, CUSTOMSCREEN,
        SA_ShowTitle, FALSE,        /* Pas de barre de titre */
        SA_Draggable, FALSE,        /* Non déplaçable */
        SA_Quiet, TRUE,             /* Pas de bip système */
        TAG_END);
        
    if (!screen) {
        OpenWorkBench();
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    /* Créer une fenêtre qui occupe tout l'écran */
    window = OpenWindowTags(NULL,
        WA_CustomScreen, screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, 320,
        WA_Height, 256,
        WA_Backdrop, TRUE,          /* Fenêtre de fond */
        WA_Borderless, TRUE,        /* Sans bordures */
        WA_RMBTrap, TRUE,           /* Capturer clic droit */
        WA_Activate, TRUE,
        WA_IDCMP, IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS,
        TAG_END);
        
    if (!window) {
        CloseScreen(screen);
        OpenWorkBench();
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    rp = window->RPort;
    
    /* Définir une palette personnalisée */
    SetRGB4(&screen->ViewPort, 0, 0, 0, 0);      /* Noir */
    SetRGB4(&screen->ViewPort, 1, 0, 0, 15);     /* Rouge vif */
    SetRGB4(&screen->ViewPort, 2, 0, 15, 0);     /* Vert vif */
    SetRGB4(&screen->ViewPort, 3, 15, 15, 15);   /* Blanc */
    
    /* Effacer l'écran en noir */
    SetRast(rp, 0);
    
    /* Définir couleur rouge pour le texte principal */
    SetAPen(rp, 1);
    
    /* Afficher "Hello World!" centré */
    Move(rp, 110, 120);
    Text(rp, "Hello World!", 12);
    
    /* Définir couleur verte pour les instructions */
    SetAPen(rp, 2);
    Move(rp, 80, 140);
    Text(rp, "Native Amiga Screen", 19);
    
    /* Définir couleur blanche pour les instructions */
    SetAPen(rp, 3);
    Move(rp, 70, 170);
    Text(rp, "Press ESC or click to exit", 26);
    
    /* Boucle d'événements */
    while (running) {
        WaitPort(window->UserPort);
        
        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) {
            switch (msg->Class) {
                case IDCMP_RAWKEY:
                    if (msg->Code == 0x45) { /* ESC */
                        running = FALSE;
                    }
                    break;
                case IDCMP_MOUSEBUTTONS:
                    running = FALSE;
                    break;
            }
            ReplyMsg((struct Message *)msg);
        }
    }
    
    /* Nettoyage */
    CloseWindow(window);
    CloseScreen(screen);
    
    /* Rouvrir le Workbench */
    OpenWorkBench();
    
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    
    return 0;
}