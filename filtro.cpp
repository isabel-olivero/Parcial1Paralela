#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024
#define MAX_MAGIC 3

int blurKernel[3][3] = {
    {1, 1, 1},
    {1, 1, 1},
    {1, 1, 1}
};
int blurDiv = 9;

int laplaceKernel[3][3] = {
    { 0, -1,  0},
    {-1,  4, -1},
    { 0, -1,  0}
};
int laplaceDiv = 1;

int sharpenKernel[3][3] = {
    { 0, -1,  0},
    {-1,  5, -1},
    { 0, -1,  0}
};
int sharpenDiv = 1;

class Imagen {
private:
    char magic[MAX_MAGIC];
    int width;
    int height;
    int max_color;
    int* pixels;
    int pixel_count;
    int channels;

public:

    Imagen() : width(0), height(0), max_color(0), pixels(nullptr), pixel_count(0), channels(1) {
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

        channels = 1;
        pixel_count = width * height;
        if (strcmp(magic, "P3") == 0) { 
            channels = 3;
            pixel_count = width * height * channels;
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
        if (strcmp(magic, "P3") == 0) return "PPM";;
    }
    
    

    void copiarDesde(const Imagen& otra) {
        liberarMemoria();
        
        strncpy(magic, otra.magic, MAX_MAGIC);
        width = otra.width;
        height = otra.height;
        max_color = otra.max_color;
        pixel_count = otra.pixel_count;
        channels = otra.channels;
        
        if (pixel_count > 0) {
            pixels = new int[pixel_count];
            for (int i = 0; i < pixel_count; i++) {
                pixels[i] = otra.pixels[i];
            }
        }
    }
    
   
    Imagen copiar() const {
        Imagen nueva;
        nueva.copiarDesde(*this);
        return nueva;
    }
    

    int aplicarKernel(int x, int y, int kernel[3][3], int divisor, int channel) const {
        int sum = 0;
        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int nx = x + kx;
                int ny = y + ky;
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    int idx = (ny * width + nx) * channels + channel;
                    sum += pixels[idx] * kernel[ky + 1][kx + 1];
                }
            }
        }
        sum /= divisor;
        if (sum < 0) sum = 0;
        if (sum > max_color) sum = max_color;
        return sum;
    }
    

    void aplicarBlur() {
        int* nuevos_pixels = new int[pixel_count];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < channels; c++) {
                    int index = (y * width + x) * channels + c;
                    nuevos_pixels[index] = aplicarKernel(x, y, blurKernel, blurDiv, c);
                }
            }
        }
        
        delete[] pixels;
        pixels = nuevos_pixels;
    }

    void aplicarLaplace() {
        int* nuevos_pixels = new int[pixel_count];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < channels; c++) {
                    int index = (y * width + x) * channels + c;
                    nuevos_pixels[index] = aplicarKernel(x, y, laplaceKernel, laplaceDiv, c);
                }
            }
        }
        
        delete[] pixels;
        pixels = nuevos_pixels;
    }
    
    void aplicarSharpening() {
        int* nuevos_pixels = new int[pixel_count];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < channels; c++) {
                    int index = (y * width + x) * channels + c;
                    nuevos_pixels[index] = aplicarKernel(x, y, sharpenKernel, sharpenDiv, c);
                }
            }
        }
        delete[] pixels;
        pixels = nuevos_pixels;
    }
};

int main(int argc, char* argv[]) {
    Imagen imagen;
    if (!imagen.cargarDesdeArchivo(argv[1])) {
        return 1;
    }
    const char* filtro = argv[3];
    
    if (strcmp(filtro, "blur") == 0) {
        imagen.aplicarBlur();
    }
    else if (strcmp(filtro, "laplace") == 0) {
        imagen.aplicarLaplace();
    }
    else if (strcmp(filtro, "sharpen") == 0) {
        imagen.aplicarSharpening();
    }
    if (!imagen.guardarEnArchivo(argv[2])) {
        return 1;
    }
    

    return 0;
}