/* Hello World simple pour Amiga 500 + Kickstart 1.3 */

/* Structure simple pour écran 320x256 */
struct SimpleScreen {
    char title[20];
    int width;
    int height;
    int depth;
};

/* Structure simple pour fenêtre */
struct SimpleWindow {
    int left;
    int top;
    int width; 
    int height;
};

/* Variables globales */
void *IntuitionBase;
void *GfxBase;
void *screen;
void *window;

/* Prototypes système minimaux */
void *OpenLibrary(char *name, unsigned long version);
void CloseLibrary(void *lib);
void *OpenScreenTags(void);
void *OpenWindowTags(void);
void CloseWindow(void *win);
void CloseScreen(void *scr);
void Delay(int ticks);

int main() {
    /* Ouverture des librairies système */
    IntuitionBase = OpenLibrary("intuition.library", 37);
    if (!IntuitionBase) return 20;
    
    GfxBase = OpenLibrary("graphics.library", 37);  
    if (!GfxBase) {
        CloseLibrary(IntuitionBase);
        return 20;
    }
    
    /* Attendre 3 secondes pour simuler l'affichage */
    Delay(150);  /* 150 ticks = 3 secondes */
    
    /* Nettoyage */
    CloseLibrary(GfxBase);
    CloseLibrary(IntuitionBase);
    
    return 0;
}