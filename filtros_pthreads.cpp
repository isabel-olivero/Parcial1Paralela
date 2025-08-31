#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>

#define MAX_FILENAME 256
#define BUFFER_SIZE 1024
#define MAX_MAGIC 3
#define NUM_THREADS 4


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

class Imagen;

Imagen* imagen_global = nullptr;
int filter_type = 0; // 0: blur, 1: laplace, 2: sharpen

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
            channels = 3; // RGB
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
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getMaxColor() const { return max_color; }
    int getPixelCount() const { return pixel_count; }
    int getChannels() const { return channels; }
    const char* getMagic() const { return magic; }
    
    const char* getTipo() const {
        if (strcmp(magic, "P2") == 0) return "PGM";
        if (strcmp(magic, "P3") == 0) return "PPM";
    }
    
    void procesarRegion(int start_x, int end_x, int start_y, int end_y, int kernel[3][3], int divisor, int filtro_type) {
        int* temp_pixels = new int[pixel_count];
        for (int i = 0; i < pixel_count; i++) {
            temp_pixels[i] = pixels[i];
        }
        
        for (int y = start_y; y < end_y; y++) {
            for (int x = start_x; x < end_x; x++) {
                for (int c = 0; c < channels; c++) {
                    int index = (y * width + x) * channels + c;
                    
                    int sum = 0;
                    for (int ky = -1; ky <= 1; ky++) {
                        for (int kx = -1; kx <= 1; kx++) {
                            int nx = x + kx;
                            int ny = y + ky;
                            if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                                int idx = (ny * width + nx) * channels + c;
                                sum += temp_pixels[idx] * kernel[ky + 1][kx + 1];
                            }
                        }
                    }
                    
                    if (divisor != 1) {
                        sum /= divisor;
                    }
                    
                    if (sum < 0) sum = 0;
                    if (sum > max_color) sum = max_color;
                    
                    pixels[index] = sum;
                
                }
            }
        }
        
        delete[] temp_pixels;
    }
};


void* procesarRegionThread(void* arg) {
    long thread_id = (long)arg;
    
    int width = imagen_global->getWidth();
    int height = imagen_global->getHeight();
    
    int start_x, end_x, start_y, end_y;
    
    // Dividir la imagen en 4
    int half_width = width / 2;
    int half_height = height / 2;
    
    switch(thread_id) {
        case 0: // Arriba-Izquierda
            start_x = 0;
            end_x = half_width;
            start_y = 0;
            end_y = half_height;
            break;
        case 1: // Arriba-Derecha
            start_x = half_width;
            end_x = width;
            start_y = 0;
            end_y = half_height;
            break;
        case 2: // Abajo-Izquierda
            start_x = 0;
            end_x = half_width;
            start_y = half_height;
            end_y = height;
            break;
        case 3: // Abajo-Derecha
            start_x = half_width;
            end_x = width;
            start_y = half_height;
            end_y = height;
            break;
    }
    
    if (filter_type == 0) {
        imagen_global->procesarRegion(start_x, end_x, start_y, end_y, blurKernel, blurDiv, filter_type);
    } else if (filter_type == 1) {
        imagen_global->procesarRegion(start_x, end_x, start_y, end_y, laplaceKernel, laplaceDiv, filter_type);
    } else if (filter_type == 2) {
        imagen_global->procesarRegion(start_x, end_x, start_y, end_y, sharpenKernel, sharpenDiv, filter_type);
    } else if (filter_type == 3) {
        imagen_global->procesarRegion(start_x, end_x, start_y, end_y, nullptr, 0, filter_type);
    }
    
    pthread_exit(NULL);
}

void aplicarFiltroConHilos(Imagen& imagen, const char* filtro) {
    imagen_global = &imagen;
    
    if (strcmp(filtro, "blur") == 0) {
        filter_type = 0;
    } else if (strcmp(filtro, "laplace") == 0) {
        filter_type = 1;
    } else if (strcmp(filtro, "sharpen") == 0) {
        filter_type = 2;
    }
    
    pthread_t threads[NUM_THREADS];
    
    for (long i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_create(&threads[i], NULL, procesarRegionThread, (void*)i);
        if (rc) {
            std::cout << "Error creating thread " << i << std::endl;
        }
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    imagen_global = nullptr;
}

int main(int argc, char* argv[]) {

  
    Imagen imagen;
    if (!imagen.cargarDesdeArchivo(argv[1])) {
        return 1;
    }
    aplicarFiltroConHilos(imagen, argv[3]);
    
    if (!imagen.guardarEnArchivo(argv[2])) {
        return 1;
    }
    

    return 0;
}