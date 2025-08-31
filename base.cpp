#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024
#define MAX_MAGIC 3

using namespace std;

class Imagen {
private:
    char magic[MAX_MAGIC];
    int width;
    int height;
    int max_color;
    int* pixels;
    int pixel_count;

public:

    Imagen() : width(0), height(0), max_color(0), pixels(nullptr), pixel_count(0) {
        magic[0] = '\0';
    }
    
    ~Imagen() {
        liberarMemoria();
    }
    
    void liberarMemoria() {
        if (pixels != nullptr) {
            delete[] pixels;
            pixels = nullptr;
        }
        pixel_count = 0;
    }

    bool cargarDesdeArchivo(const char* filename) {
        liberarMemoria();
        
        FILE *file = fopen(filename, "r");
        if (file == NULL) {
            std::cout << "Error, incorrect path or incorrect file." << std::endl;
            return false;
        }

        char magic_buffer[MAX_MAGIC];
        fscanf(file, "%2s", magic_buffer);
        strncpy(magic, magic_buffer, MAX_MAGIC);
        
        fscanf(file, "%d %d", &width, &height);
        fscanf(file, "%d", &max_color);

        pixel_count = width * height;
        if (strcmp(magic, "P3") == 0) { 
            pixel_count = width * height * 3;
        }

        pixels = new int[pixel_count];

        int value;
        for (int i = 0; i < pixel_count; i++) {
            if (fscanf(file, "%d", &value) != 1) {
                std::cout << "Error reading pixels." << std::endl;
                liberarMemoria();
                fclose(file);
                return false;
            }
            pixels[i] = value;
        }
        
        fclose(file);
        return true;
    }
    
    bool guardarEnArchivo(const char* filename) {
        FILE *output = fopen(filename, "w");
        if (output == NULL) {
            std::cout << "Error creating output file." << std::endl;
            return false;
        }

        fprintf(output, "%s\n%d %d\n%d\n", magic, width, height, max_color);

        for (int i = 0; i < pixel_count; i++) {
            fprintf(output, "%d\n", pixels[i]);
        }

        fclose(output);
        return true;
    }
    
    const char* getTipo() const {
        if (strcmp(magic, "P2") == 0) return "PGM";
        if (strcmp(magic, "P3") == 0) return "PPM";
    }
};

int main(int argc, char* argv[]) {

    Imagen imagen;
    int ans;

    if (!imagen.cargarDesdeArchivo(argv[1])) {
        ans= 1;
    }
    if (!imagen.guardarEnArchivo(argv[2])) {
        ans=1;
    }
    ans =0;

    return ans;
}