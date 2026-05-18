#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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

// Protótipos - Atualizados com parâmetro de opacidade (alpha)
void load(char *name, Img *pic);
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness, float alpha);
void draw_rectangle(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness, float alpha);
float calcular_luminancia(Pixel p);
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho);
float calcular_variancia(Pixel bloco[], int tamanho);
float calcular_threshold_adaptativo(int width, int height, Pixel pin[][width], int bloco_size);
void processar_imagem(int width, int height, Pixel pin[][width], Pixel pout[][width]);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("forensics [origem]\n");
        exit(1);
    }

    load(argv[1], &in);
    printf("Origem   : %s %d x %d\n", argv[1], in.width, in.height);
    printf("Processando...\n");

    int tam = in.width * in.height;
    out = in;
    out.pixels = malloc(tam * sizeof(Pixel));
    memset(out.pixels, 0, tam * sizeof(Pixel));

    Pixel (*pin)[in.width]  = (Pixel(*)[in.height]) in.pixels;
    Pixel (*pout)[in.width] = (Pixel(*)[in.height]) out.pixels;

    processar_imagem(in.width, in.height, pin, pout);

    // NÃO ALTERAR A PARTIR DAQUI!

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
    for (int i = 0; i < 16; i++)
        printf("[%02X %02X %02X] ", pic->pixels[i].r, pic->pixels[i].g, pic->pixels[i].b);
    printf("\n");
}

// Função de desenho de linha atualizada para suportar opacidade (alpha blending)
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness, float alpha) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    int half = thickness / 2;
    while (1) {
        for (int i = -half; i <= half; i++) {
            for (int j = -half; j <= half; j++) {
                int xi = x0 + i, yj = y0 + j;
                if (xi >= 0 && xi < width && yj >= 0 && yj < height) {
                    // Mistura a cor da linha com a cor atual do pixel
                    img[yj][xi].r = (unsigned char)(img[yj][xi].r * (1.0f - alpha) + color.r * alpha);
                    img[yj][xi].g = (unsigned char)(img[yj][xi].g * (1.0f - alpha) + color.g * alpha);
                    img[yj][xi].b = (unsigned char)(img[yj][xi].b * (1.0f - alpha) + color.b * alpha);
                }
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy)  { err += dx; y0 += sy; }
    }
}

// Função de retângulo atualizada para repassar o alpha
void draw_rectangle(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness, float alpha) {
    draw_line(width, height, img, x0, y0, x1, y0, color, thickness, alpha);
    draw_line(width, height, img, x0, y1, x1, y1, color, thickness, alpha);
    draw_line(width, height, img, x0, y0, x0, y1, color, thickness, alpha);
    draw_line(width, height, img, x1, y0, x1, y1, color, thickness, alpha);
}

float calcular_luminancia(Pixel p) {
    return 0.59f * p.g + 0.30f * p.r + 0.11f * p.b;
}

float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho) {
    float soma = 0.0f;
    for (int i = 0; i < tamanho; i++) {
        float dr = bloco1[i].r - bloco2[i].r;
        float dg = bloco1[i].g - bloco2[i].g;
        float db = bloco1[i].b - bloco2[i].b;
        soma += dr*dr + dg*dg + db*db;
    }
    return soma / (tamanho * 3);
}

float calcular_variancia(Pixel bloco[], int tamanho) {
    float mr = 0, mg = 0, mb = 0;
    for (int i = 0; i < tamanho; i++) {
        mr += bloco[i].r; mg += bloco[i].g; mb += bloco[i].b;
    }
    mr /= tamanho; mg /= tamanho; mb /= tamanho;
    float var = 0;
    for (int i = 0; i < tamanho; i++) {
        float dr = bloco[i].r - mr, dg = bloco[i].g - mg, db = bloco[i].b - mb;
        var += dr*dr + dg*dg + db*db;
    }
    return var / (tamanho * 3);
}

static int cmp_float(const void *a, const void *b) {
    float fa = *(float*)a, fb = *(float*)b;
    return (fa > fb) - (fa < fb);
}

float calcular_threshold_adaptativo(int width, int height, Pixel pin[][width], int bloco_size) {
    int num_bx = width  / bloco_size;
    int num_by = height / bloco_size;
    int total  = num_bx * num_by;

    float *variancias = malloc(total * sizeof(float));
    if (!variancias) return 100.0f;

    int n = 0;
    for (int by = 0; by < num_by; by++) {
        for (int bx = 0; bx < num_bx; bx++) {
            int xs = bx * bloco_size, ys = by * bloco_size;
            Pixel bloco[bloco_size * bloco_size];
            int k = 0;
            for (int i = 0; i < bloco_size; i++)
                for (int j = 0; j < bloco_size; j++)
                    bloco[k++] = pin[ys + i][xs + j];
            variancias[n++] = calcular_variancia(bloco, k);
        }
    }

    qsort(variancias, n, sizeof(float), cmp_float);

    float p40 = variancias[(int)(n * 0.40f)];
    float threshold = p40 > 100.0f ? p40 : 100.0f;
    printf("  Threshold adaptativo (max(p40,100)): %.1f\n", threshold);

    free(variancias);
    return threshold;
}

void processar_imagem(int width, int height, Pixel pin[][width], Pixel pout[][width]) {
    int BLOCO_SIZE   = 24;
    float THRESHOLD  = 250.0f;

    int DIST_MIN = BLOCO_SIZE * 3;

    int num_blocos_x = width  / BLOCO_SIZE;
    int num_blocos_y = height / BLOCO_SIZE;

    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            pout[i][j] = pin[i][j];

    float MIN_VARIANCIA = calcular_threshold_adaptativo(width, height, pin, BLOCO_SIZE);

    // Alterado para Vermelho
    Pixel cor_vermelha = {255, 0, 0};

    char *used = (char*)calloc(num_blocos_x * num_blocos_y, sizeof(char));
    if (!used) return;

    for (int by1 = 0; by1 < num_blocos_y; by1++) {
        for (int bx1 = 0; bx1 < num_blocos_x; bx1++) {
            int idx1 = by1 * num_blocos_x + bx1;
            if (used[idx1]) continue;

            int x1s = bx1 * BLOCO_SIZE, y1s = by1 * BLOCO_SIZE;
            int x1e = x1s + BLOCO_SIZE - 1, y1e = y1s + BLOCO_SIZE - 1;

            Pixel bloco1[BLOCO_SIZE * BLOCO_SIZE];
            int t1 = 0;
            for (int i = 0; i < BLOCO_SIZE; i++)
                for (int j = 0; j < BLOCO_SIZE; j++)
                    if (y1s+i < height && x1s+j < width)
                        bloco1[t1++] = pin[y1s+i][x1s+j];
            if (t1 == 0) continue;

            if (calcular_variancia(bloco1, t1) < MIN_VARIANCIA) continue;

            int cx1 = x1s + BLOCO_SIZE/2, cy1 = y1s + BLOCO_SIZE/2;

            float best_sim = 1e30f;
            int   best_bx2 = -1, best_by2 = -1;

            for (int by2 = 0; by2 < num_blocos_y; by2++) {
                for (int bx2 = 0; bx2 < num_blocos_x; bx2++) {
                    int idx2 = by2 * num_blocos_x + bx2;
                    if (idx2 == idx1 || used[idx2]) continue;

                    int x2s = bx2 * BLOCO_SIZE, y2s = by2 * BLOCO_SIZE;
                    int cx2 = x2s + BLOCO_SIZE/2, cy2 = y2s + BLOCO_SIZE/2;

                    int ddx = cx1-cx2, ddy = cy1-cy2;
                    if (ddx*ddx + ddy*ddy < DIST_MIN*DIST_MIN) continue;

                    Pixel bloco2[BLOCO_SIZE * BLOCO_SIZE];
                    int t2 = 0;
                    for (int i = 0; i < BLOCO_SIZE; i++)
                        for (int j = 0; j < BLOCO_SIZE; j++)
                            if (y2s+i < height && x2s+j < width)
                                bloco2[t2++] = pin[y2s+i][x2s+j];
                    if (t2 == 0) continue;

                    if (calcular_variancia(bloco2, t2) < MIN_VARIANCIA) continue;

                    int t = t1 < t2 ? t1 : t2;
                    float sim = comparar_blocos(bloco1, bloco2, t);
                    if (sim < best_sim) {
                        best_sim = sim;
                        best_bx2 = bx2;
                        best_by2 = by2;
                    }
                }
            }

            if (best_bx2 >= 0 && best_sim < THRESHOLD) {
                int idx2 = best_by2 * num_blocos_x + best_bx2;
                used[idx1] = 1;
                used[idx2] = 1;

                int x2s = best_bx2 * BLOCO_SIZE, y2s = best_by2 * BLOCO_SIZE;
                int x2e = x2s + BLOCO_SIZE - 1, y2e = y2s + BLOCO_SIZE - 1;
                int cx2 = x2s + BLOCO_SIZE/2,   cy2 = y2s + BLOCO_SIZE/2;

                // Desenha os retângulos vermelhos, espessura 1, 100% opacos (1.0f)
                draw_rectangle(width, height, pout, x1s, y1s, x1e, y1e, cor_vermelha, 1, 1.0f);
                draw_rectangle(width, height, pout, x2s, y2s, x2e, y2e, cor_vermelha, 1, 1.0f);
                
                // Desenha a linha central vermelha, espessura 1, com 40% de opacidade (0.4f)
                draw_line(width, height, pout, cx1, cy1, cx2, cy2, cor_vermelha, 1, 0.4f);
            }
        }
    }

    free(used);
}