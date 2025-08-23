#include <stdio.h>
#include <stdlib.h>

struct RGB {
    unsigned char r, g, b;
};

struct Image {
    int width, height;
    struct RGB *pixels;
};

struct Image* create_image(int width, int height) {
    struct Image *img = malloc(sizeof(struct Image));
    if (!img) return NULL;
    
    img->width = width;
    img->height = height;
    img->pixels = malloc(width * height * sizeof(struct RGB));
    
    if (!img->pixels) {
        free(img);
        return NULL;
    }
    
    return img;
}

void set_pixel(struct Image *img, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    if (x >= 0 && x < img->width && y >= 0 && y < img->height) {
        int index = y * img->width + x;
        img->pixels[index].r = r;
        img->pixels[index].g = g;
        img->pixels[index].b = b;
    }
}

void draw_line(struct Image *img, int x1, int y1, int x2, int y2, unsigned char r, unsigned char g, unsigned char b) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    
    while (1) {
        set_pixel(img, x, y, r, g, b);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void save_ppm(struct Image *img, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "P3\n%d %d\n255\n", img->width, img->height);
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int index = y * img->width + x;
            fprintf(f, "%d %d %d ", 
                img->pixels[index].r,
                img->pixels[index].g, 
                img->pixels[index].b);
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
}

void free_image(struct Image *img) {
    if (img) {
        if (img->pixels) free(img->pixels);
        free(img);
    }
}

int main() {
    printf("=== Demo Amiga - Manipulation d'images ===\n");
    
    struct Image *img = create_image(320, 256);
    if (!img) {
        printf("Erreur: impossible de créer l'image\n");
        return 1;
    }
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            set_pixel(img, x, y, 0, 0, 50);
        }
    }
    
    draw_line(img, 50, 50, 270, 206, 255, 255, 255);
    draw_line(img, 50, 206, 270, 50, 255, 255, 255);
    
    for (int i = 0; i < 32; i++) {
        set_pixel(img, 160 + i, 128, 255, 0, 0);
        set_pixel(img, 160, 128 + i, 0, 255, 0);
    }
    
    printf("Image créée: %dx%d pixels\n", img->width, img->height);
    printf("Lignes et pixels dessinés\n");
    
    save_ppm(img, "output.ppm");
    printf("Image sauvée dans output.ppm\n");
    
    free_image(img);
    printf("Mémoire libérée\n");
    
    return 0;
}