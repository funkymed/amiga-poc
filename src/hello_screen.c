/* Hello World avec écran réel pour Amiga */
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
    
    /* Créer l'écran 320x256 */
    screen = OpenScreenTags(NULL,
        SA_Width, 320,
        SA_Height, 256, 
        SA_Depth, 4,
        SA_DisplayID, LORES_KEY,
        SA_Title, "Hello World Screen",
        TAG_END);
        
    if (!screen) {
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    /* Créer la fenêtre */
    window = OpenWindowTags(NULL,
        WA_CustomScreen, screen,
        WA_Left, 0,
        WA_Top, 0,
        WA_Width, 320,
        WA_Height, 256,
        WA_Backdrop, TRUE,
        WA_Borderless, TRUE,
        WA_Activate, TRUE,
        WA_IDCMP, IDCMP_RAWKEY | IDCMP_MOUSEBUTTONS,
        TAG_END);
        
    if (!window) {
        CloseScreen(screen);
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary((struct Library *)IntuitionBase);
        return 20;
    }
    
    rp = window->RPort;
    
    /* Effacer l'écran en bleu */
    SetRast(rp, 1);
    
    /* Définir couleur blanche pour le texte */
    SetAPen(rp, 3);
    
    /* Afficher "Hello World!" centré */
    Move(rp, 110, 130);
    Text(rp, "Hello World!", 12);
    
    /* Afficher instructions */
    Move(rp, 80, 150);
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
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)IntuitionBase);
    
    return 0;
}