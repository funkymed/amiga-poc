#include <proto/exec.h>
#include <proto/dos.h>

int main() {
    struct Library *DOSBase;
    
    DOSBase = OpenLibrary("dos.library", 36);
    if (!DOSBase) return 20;
    
    Printf("=== Demo Amiga - Test Simple ===\n");
    Printf("Hello from Amiga!\n");
    Printf("Programme fonctionne correctement.\n");
    Printf("\nAppuyez sur Entree...\n");
    
    getchar();
    
    CloseLibrary(DOSBase);
    return 0;
}