#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mpi.h>

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
        if (fscanf(file, "%2s", magic_buffer) != 1) {
            std::cout << "Error reading magic number." << std::endl;
            fclose(file);
            return false;
        }
        strncpy(magic, magic_buffer, MAX_MAGIC);
        
        if (fscanf(file, "%d %d", &width, &height) != 2) {
            std::cout << "Error reading width and height." << std::endl;
            fclose(file);
            return false;
        }
        
        if (fscanf(file, "%d", &max_color) != 1) {
            std::cout << "Error reading max color." << std::endl;
            fclose(file);
            return false;
        }

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
                std::cout << "Error reading pixels at position " << i << "." << std::endl;
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
            std::cout << "Error creating output file: " << filename << std::endl;
            return false;
        }

        if (fprintf(output, "%s\n%d %d\n%d\n", magic, width, height, max_color) < 0) {
            std::cout << "Error writing header." << std::endl;
            fclose(output);
            return false;
        }

        for (int i = 0; i < pixel_count; i++) {
            if (fprintf(output, "%d\n", pixels[i]) < 0) {
                std::cout << "Error writing pixels at position " << i << "." << std::endl;
                fclose(output);
                return false;
            }
        }

        fclose(output);
        return true;
    }
    
    const char* getTipo() const {
        if (strcmp(magic, "P2") == 0) return "PGM";
        if (strcmp(magic, "P3") == 0) return "PPM";
        return "Unknown";
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
    
    void aplicar(int n) {
        int* nuevos_pixels = new int[pixel_count];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                for (int c = 0; c < channels; c++) {
                    int index = (y * width + x) * channels + c;

                    if (n == 1) {
                        nuevos_pixels[index] = aplicarKernel(x, y, blurKernel, blurDiv, c);
                    }
                    else if (n == 2) {
                        nuevos_pixels[index] = aplicarKernel(x, y, laplaceKernel, laplaceDiv, c);
                    }
                    else if (n == 3) {
                        nuevos_pixels[index] = aplicarKernel(x, y, sharpenKernel, sharpenDiv, c);
                    }
                }
            }
        }
        
        delete[] pixels;
        pixels = nuevos_pixels;
    }
    
    // Métodos para MPI
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    int getMaxColor() const { return max_color; }
    int getChannels() const { return channels; }
    int getPixelCount() const { return pixel_count; }
    const char* getMagic() const { return magic; }
    int* getPixels() const { return pixels; }
    
    void setMetadata(const char* mgc, int w, int h, int mc, int ch) {
        strncpy(magic, mgc, MAX_MAGIC);
        width = w;
        height = h;
        max_color = mc;
        channels = ch;
        pixel_count = w * h * ch;
    }
    
    void allocatePixels() {
        if (pixels != nullptr) delete[] pixels;
        pixels = new int[pixel_count];
    }
};

void dividirImagen(const Imagen& imagenOriginal, Imagen& parteImagen, int rank, int size) {
    int height = imagenOriginal.getHeight();
    int width = imagenOriginal.getWidth();
    int channels = imagenOriginal.getChannels();
    
    int rows_per_process = height / size;
    int extra_rows = height % size;
    
    int start_row = rank * rows_per_process + (rank < extra_rows ? rank : extra_rows);
    int end_row = start_row + rows_per_process + (rank < extra_rows ? 1 : 0);
    
    int start_row_with_border = (start_row > 0) ? start_row - 1 : 0;
    int end_row_with_border = (end_row < height) ? end_row + 1 : height;
    
    int part_height = end_row_with_border - start_row_with_border;
    
    parteImagen.setMetadata(imagenOriginal.getMagic(), width, part_height, 
                           imagenOriginal.getMaxColor(), channels);
    parteImagen.allocatePixels();
    
    int* original_pixels = imagenOriginal.getPixels();
    int* part_pixels = parteImagen.getPixels();
    
    for (int y = 0; y < part_height; y++) {
        int orig_y = start_row_with_border + y;
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int orig_idx = (orig_y * width + x) * channels + c;
                int part_idx = (y * width + x) * channels + c;
                part_pixels[part_idx] = original_pixels[orig_idx];
            }
        }
    }
}

void combinarImagen(Imagen& imagenCompleta, const Imagen& parteImagen, int rank, int size) {
    int height = imagenCompleta.getHeight();
    int width = imagenCompleta.getWidth();
    int channels = imagenCompleta.getChannels();
    
    int rows_per_process = height / size;
    int extra_rows = height % size;
    
    int start_row = rank * rows_per_process + (rank < extra_rows ? rank : extra_rows);
    int end_row = start_row + rows_per_process + (rank < extra_rows ? 1 : 0);
    
    int start_row_in_part = (start_row > 0) ? 1 : 0;
    int part_height = end_row - start_row;
    
    int* complete_pixels = imagenCompleta.getPixels();
    const int* part_pixels = parteImagen.getPixels();
    
    for (int y = 0; y < part_height; y++) {
        int complete_y = start_row + y;
        int part_y = start_row_in_part + y;
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                int complete_idx = (complete_y * width + x) * channels + c;
                int part_idx = (part_y * width + x) * channels + c;
                complete_pixels[complete_idx] = part_pixels[part_idx];
            }
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    if (argc != 4) {
        if (rank == 0) {
            std::cout << "Uso: " << argv[0] << " <input_file> <output_file> <filter>" << std::endl;
            std::cout << "Filtros: blur, laplace, sharpen" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    const char* filtro = argv[3];
    
    Imagen imagenCompleta;
    Imagen parteImagen;
    
    double start_time, end_time;
    
    if (rank == 0) {
        if (!imagenCompleta.cargarDesdeArchivo(input_file)) {
            MPI_Abort(MPI_COMM_WORLD, 1);
            MPI_Finalize();
            return 1;
        }
        start_time = MPI_Wtime();
    }
    
    // Broadcast de metadata
    char magic[MAX_MAGIC];
    int width, height, max_color, channels;
    
    if (rank == 0) {
        strncpy(magic, imagenCompleta.getMagic(), MAX_MAGIC);
        width = imagenCompleta.getWidth();
        height = imagenCompleta.getHeight();
        max_color = imagenCompleta.getMaxColor();
        channels = imagenCompleta.getChannels();
    }
    
    MPI_Bcast(magic, MAX_MAGIC, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_color, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (rank != 0) {
        imagenCompleta.setMetadata(magic, width, height, max_color, channels);
    }
    
    // Distribuir la imagen
    if (rank == 0) {
        for (int dest = 1; dest < size; dest++) {
            Imagen tempPart;
            dividirImagen(imagenCompleta, tempPart, dest, size);
            
            int part_height = tempPart.getHeight();
            MPI_Send(&part_height, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            
            MPI_Send(tempPart.getPixels(), tempPart.getPixelCount(), MPI_INT, 
                    dest, 1, MPI_COMM_WORLD);
        }
        
        dividirImagen(imagenCompleta, parteImagen, 0, size);
    } else {
        int part_height;
        MPI_Recv(&part_height, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        parteImagen.setMetadata(magic, width, part_height, max_color, channels);
        parteImagen.allocatePixels();
        
        MPI_Recv(parteImagen.getPixels(), parteImagen.getPixelCount(), MPI_INT,
                0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Aplicar filtro
    if (strcmp(filtro, "blur") == 0) {
        parteImagen.aplicar(1);
    }
    else if (strcmp(filtro, "laplace") == 0) {
        parteImagen.aplicar(2);
    }
    else if (strcmp(filtro, "sharpen") == 0) {
        parteImagen.aplicar(3);
    }
    
    // Recolectar resultados
    if (rank == 0) {
        for (int src = 1; src < size; src++) {
            Imagen tempPart;
            int part_height;
            
            MPI_Recv(&part_height, 1, MPI_INT, src, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            tempPart.setMetadata(magic, width, part_height, max_color, channels);
            tempPart.allocatePixels();
            
            MPI_Recv(tempPart.getPixels(), tempPart.getPixelCount(), MPI_INT,
                    src, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            combinarImagen(imagenCompleta, tempPart, src, size);
        }
        
        combinarImagen(imagenCompleta, parteImagen, 0, size);
        
        end_time = MPI_Wtime();
        
        if (!imagenCompleta.guardarEnArchivo(output_file)) {
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        std::cout << "Tiempo de ejecución: " << (end_time - start_time) << " segundos" << std::endl;
        std::cout << "Procesado con " << size << " procesos" << std::endl;
        
    } else {
        int part_height = parteImagen.getHeight();
        MPI_Send(&part_height, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
        MPI_Send(parteImagen.getPixels(), parteImagen.getPixelCount(), MPI_INT,
                0, 3, MPI_COMM_WORLD);
    }
    
    MPI_Finalize();
    return 0;
}