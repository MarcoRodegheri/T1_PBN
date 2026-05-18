#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

// Um pixel Pixel (24 bits)
typedef struct
{
    unsigned char r, g, b;
} Pixel;

// Uma imagem Pixel
typedef struct
{
    int width, height;
    int channels;
    Pixel *pixels;
} Img;

// As 2 imagens
Img in, out;

// Protótipos
void load(char *name, Img *pic);
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness);
void draw_rectangle(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness);
float calcular_luminancia(Pixel p);
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho);
float calcular_variancia(Pixel bloco[], int tamanho);
void processar_imagem(int width, int height, Pixel pin[][width], Pixel pout[][width]);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("forensics [origem]\n");
        exit(1);
    }

    // Carrega a imagem original
    load(argv[1], &in);

    // Exibe as dimensões na tela, para conferência
    printf("Origem   : %s %d x %d\n", argv[1], in.width, in.height);

    printf("Processando...\n");

    // Cria imagem de saída e "zera" ela
    int tam = in.width * in.height;
    out = in;
    out.pixels = malloc(tam * sizeof(Pixel));
    memset(out.pixels, 0, tam * sizeof(Pixel));

    // Converte para interpretar como matrizes
    Pixel (*pin)[in.width] = (Pixel(*)[in.height]) in.pixels;
    Pixel (*pout)[in.width] = (Pixel(*)[in.height]) out.pixels;

    //
    // Processa a imagem para detectar clones (copy-move-forgery)
    //
    processar_imagem(in.width, in.height, pin, pout);

    // NÃO ALTERAR A PARTIR DAQUI!

    // Grava a imagem como JPEG para registro
    stbi_write_jpg("saida.jpg", out.width, out.height, 3, pout, 90);

    free(in.pixels);
    free(out.pixels);
}

void load(char *name, Img *pic)
{
    pic->pixels = (Pixel *)stbi_load(name, &pic->width, &pic->height, &pic->channels, 0);
    if (!pic->pixels)
    {
        printf("STB loading error\n");
        exit(1);
    }
    printf("Load: %d x %d x %d\n", pic->width, pic->height, pic->channels);
    // Exibe os 16 primeiros pixels (teste)
    for (int i = 0; i < 16; i++)
    {
        printf("[%02X %02X %02X] ", pic->pixels[i].r, pic->pixels[i].g, pic->pixels[i].b);
    }
    printf("\n");
}

// Algoritmo de Bresenham para desenhar uma linha em uma matriz de pixels
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    int half = thickness / 2;
    while (1) {
        // Draw a square of size thickness x thickness centered at (x0, y0)
        for (int i = -half; i <= half; i++) {
            for (int j = -half; j <= half; j++) {
                int xi = x0 + i, yj = y0 + j;
                if (xi >= 0 && xi < width && yj >= 0 && yj < height)
                    img[yj][xi] = color;
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)  {
            err += dx;
            y0 += sy;
        }
    }
}

// Desenha um retângulo em volta de uma região
void draw_rectangle(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness) {
    // Top line
    draw_line(width, height, img, x0, y0, x1, y0, color, thickness);
    // Bottom line
    draw_line(width, height, img, x0, y1, x1, y1, color, thickness);
    // Left line
    draw_line(width, height, img, x0, y0, x0, y1, color, thickness);
    // Right line
    draw_line(width, height, img, x1, y0, x1, y1, color, thickness);
}

// Calcula a luminância (escala de cinza) de um pixel
float calcular_luminancia(Pixel p) {
    return 0.59f * p.g + 0.30f * p.r + 0.11f * p.b;
}

// Compara dois blocos de pixels usando distância euclidiana
// Retorna um valor entre 0 (idênticos) e infinito (completamente diferentes)
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho) {
    float soma = 0.0f;
    for (int i = 0; i < tamanho; i++) {
        float diff_r = bloco1[i].r - bloco2[i].r;
        float diff_g = bloco1[i].g - bloco2[i].g;
        float diff_b = bloco1[i].b - bloco2[i].b;
        soma += diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
    }
    return soma / (tamanho * 3);  // MSE normalizado para RGB
}

// Calcula a variância de um bloco (uniformidade)
float calcular_variancia(Pixel bloco[], int tamanho) {
    float media_r = 0, media_g = 0, media_b = 0;
    
    // Calcula média
    for (int i = 0; i < tamanho; i++) {
        media_r += bloco[i].r;
        media_g += bloco[i].g;
        media_b += bloco[i].b;
    }
    media_r /= tamanho;
    media_g /= tamanho;
    media_b /= tamanho;
    
    // Calcula variância
    float variancia = 0;
    for (int i = 0; i < tamanho; i++) {
        float diff_r = bloco[i].r - media_r;
        float diff_g = bloco[i].g - media_g;
        float diff_b = bloco[i].b - media_b;
        variancia += diff_r * diff_r + diff_g * diff_g + diff_b * diff_b;
    }
    return variancia / (tamanho * 3);
}

// Processa a imagem para detectar copy-move-forgery
void processar_imagem(int width, int height, Pixel pin[][width], Pixel pout[][width]) {
    int BLOCO_SIZE = 24;  // Tamanho do bloco menor para detectar texturas de árvore
    float THRESHOLD = 250.0f;  // Menos rigoroso para detectar similaridades
    float MIN_VARIANCIA = 80.0f;  // Permite detectar mais texturas

    int num_blocos_x = width / BLOCO_SIZE;
    int num_blocos_y = height / BLOCO_SIZE;

    // Copia a imagem original para a saída
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pout[i][j] = pin[i][j];
        }
    }

    // Cores para marcação
    Pixel cor_verde = {0, 255, 0};

    // Vetor para marcar blocos já pareados
    char *used = (char*)calloc(num_blocos_x * num_blocos_y, sizeof(char));
    if (!used) return;

    // Para cada bloco, encontra o melhor par disponível (greedy matching)
    for (int by1 = 0; by1 < num_blocos_y; by1++) {
        for (int bx1 = 0; bx1 < num_blocos_x; bx1++) {
            int idx_block1 = by1 * num_blocos_x + bx1;
            if (used[idx_block1]) continue; // já pareado

            int x1_start = bx1 * BLOCO_SIZE;
            int y1_start = by1 * BLOCO_SIZE;
            int x1_end = x1_start + BLOCO_SIZE - 1;
            int y1_end = y1_start + BLOCO_SIZE - 1;

            // Extrai o primeiro bloco
            Pixel bloco1[BLOCO_SIZE * BLOCO_SIZE];
            int idx = 0;
            for (int i = 0; i < BLOCO_SIZE; i++) {
                for (int j = 0; j < BLOCO_SIZE; j++) {
                    if (y1_start + i < height && x1_start + j < width) {
                        bloco1[idx++] = pin[y1_start + i][x1_start + j];
                    }
                }
            }

            int tamanho_real = idx; // número de pixels efetivos no bloco (bordas)
            if (tamanho_real == 0) continue;

            // Ignora blocos muito uniformes (como céu ou área de uma cor)
            float var1 = calcular_variancia(bloco1, tamanho_real);
            if (var1 < MIN_VARIANCIA) {
                continue;
            }

            // Procura o melhor par disponível
            float best_sim = 1e30f;
            int best_bx2 = -1, best_by2 = -1;

            for (int by2 = 0; by2 < num_blocos_y; by2++) {
                for (int bx2 = 0; bx2 < num_blocos_x; bx2++) {
                    int idx_block2 = by2 * num_blocos_x + bx2;
                    if (idx_block2 == idx_block1) continue;
                    if (used[idx_block2]) continue;

                    int x2_start = bx2 * BLOCO_SIZE;
                    int y2_start = by2 * BLOCO_SIZE;

                    // Extrai bloco2
                    Pixel bloco2[BLOCO_SIZE * BLOCO_SIZE];
                    int idx2 = 0;
                    for (int i = 0; i < BLOCO_SIZE; i++) {
                        for (int j = 0; j < BLOCO_SIZE; j++) {
                            if (y2_start + i < height && x2_start + j < width) {
                                bloco2[idx2++] = pin[y2_start + i][x2_start + j];
                            }
                        }
                    }
                    int tamanho2 = idx2;
                    if (tamanho2 == 0) continue;

                    // Ignora blocos muito uniformes
                    float var2 = calcular_variancia(bloco2, tamanho2);
                    if (var2 < MIN_VARIANCIA) continue;

                    // Para comparar, usa o menor tamanho comum (caso bordas)
                    int comparar_tamanho = tamanho_real < tamanho2 ? tamanho_real : tamanho2;

                    float sim = comparar_blocos(bloco1, bloco2, comparar_tamanho);
                    if (sim < best_sim) {
                        best_sim = sim;
                        best_bx2 = bx2;
                        best_by2 = by2;
                    }
                }
            }

            // Se o melhor par for suficientemente similar, marca ambos como usados e desenha
            if (best_bx2 >= 0 && best_sim < THRESHOLD) {
                int idx_block2 = best_by2 * num_blocos_x + best_bx2;
                used[idx_block1] = 1;
                used[idx_block2] = 1;

                int x2_start = best_bx2 * BLOCO_SIZE;
                int y2_start = best_by2 * BLOCO_SIZE;
                int x2_end = x2_start + BLOCO_SIZE - 1;
                int y2_end = y2_start + BLOCO_SIZE - 1;

                int cx1 = x1_start + BLOCO_SIZE / 2;
                int cy1 = y1_start + BLOCO_SIZE / 2;
                int cx2 = x2_start + BLOCO_SIZE / 2;
                int cy2 = y2_start + BLOCO_SIZE / 2;

                draw_rectangle(width, height, pout, x1_start, y1_start, x1_end, y1_end, cor_verde, 3);
                draw_rectangle(width, height, pout, x2_start, y2_start, x2_end, y2_end, cor_verde, 3);
                draw_line(width, height, pout, cx1, cy1, cx2, cy2, cor_verde, 1);
            }
        }
    }

    free(used);
}

