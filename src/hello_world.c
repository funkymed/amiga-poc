/* Hello World graphique pour Amiga 500 + Kickstart 1.3 */

struct Library *IntuitionBase;
struct GfxBase *GfxBase;

struct Screen *screen;
struct Window *window;
struct RastPort *rastport;

/* Prototypes des fonctions système */
struct Library *OpenLibrary(char *name, unsigned long version);
void CloseLibrary(struct Library *lib);
struct Screen *OpenScreen(void *tags);
struct Window *OpenWindow(void *tags);
void CloseWindow(struct Window *win);
void CloseScreen(struct Screen *scr);
void Move(struct RastPort *rp, int x, int y);
void Text(struct RastPort *rp, char *text, int len);
void SetAPen(struct RastPort *rp, int pen);
void SetRast(struct RastPort *rp, int pen);
unsigned long Wait(unsigned long mask);
void *GetMsg(void *port);
void ReplyMsg(void *msg);
int strlen(char *str);

/* Tags pour OpenScreen */
struct TagItem screen_tags[] = {
    {0x80000027, 320},      /* SA_Width */
    {0x80000028, 256},      /* SA_Height */
    {0x80000029, 4},        /* SA_Depth */
    {0x8000002a, 0x8000},   /* SA_DisplayID (LORES_KEY) */
    {0x80000030, (int)"Hello World"}, /* SA_Title */
    {0, 0}                  /* TAG_END */
};

/* Tags pour OpenWindow */
struct TagItem window_tags[] = {
    {0x80000065, 0},        /* WA_Left */
    {0x80000066, 0},        /* WA_Top */
    {0x80000067, 320},      /* WA_Width */
    {0x80000068, 256},      /* WA_Height */
    {0x8000006b, 1},        /* WA_Backdrop */
    {0x8000006c, 1},        /* WA_Borderless */
    {0x80000070, 1},        /* WA_Activate */
    {0x80000074, 0x00000200}, /* WA_IDCMP (IDCMP_RAWKEY) */
    {0, 0}                  /* TAG_END */
};

int strlen(char *str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int _main() {
    char *hello_text = "Hello World!";
    int text_len, text_x, text_y;
    struct IntuiMessage *msg;
    int running = 1;
    
    /* Ouvrir les librairies */
    IntuitionBase = OpenLibrary("intuition.library", 37);
    if (!IntuitionBase) return 20;
    
    GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if (!GfxBase) {
        CloseLibrary(IntuitionBase);
        return 20;
    }
    
    /* Ouvrir l'écran */
    screen = OpenScreen(screen_tags);
    if (!screen) {
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary(IntuitionBase);
        return 20;
    }
    
    /* Modifier les tags de la fenêtre pour utiliser notre écran */
    window_tags[8].ti_Tag = 0x80000064;     /* WA_CustomScreen */
    window_tags[8].ti_Data = (int)screen;
    
    /* Ouvrir la fenêtre */
    window = OpenWindow(window_tags);
    if (!window) {
        CloseScreen(screen);
        CloseLibrary((struct Library *)GfxBase);
        CloseLibrary(IntuitionBase);
        return 20;
    }
    
    rastport = window->RPort;
    
    /* Effacer l'écran en noir */
    SetRast(rastport, 0);
    
    /* Définir la couleur du texte en blanc */
    SetAPen(rastport, 1);
    
    /* Calculer la position centrée */
    text_len = strlen(hello_text);
    text_x = (320 - (text_len * 8)) / 2;  /* Approximation 8 pixels par caractère */
    text_y = 128;                          /* Centre vertical */
    
    /* Afficher le texte */
    Move(rastport, text_x, text_y);
    Text(rastport, hello_text, text_len);
    
    /* Boucle d'événements */
    while (running) {
        unsigned long signals = Wait(1L << window->UserPort->mp_SigBit);
        
        while ((msg = (struct IntuiMessage *)GetMsg(window->UserPort))) {
            if (msg->Class == 0x00000200) {  /* IDCMP_RAWKEY */
                if (msg->Code == 0x45) {     /* Code touche ESC */
                    running = 0;
                }
            }
            ReplyMsg((struct Message *)msg);
        }
    }
    
    /* Nettoyage */
    CloseWindow(window);
    CloseScreen(screen);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary(IntuitionBase);
    
    return 0;
}