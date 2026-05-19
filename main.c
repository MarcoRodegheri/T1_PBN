#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

// Um pixel (24 bits)
typedef struct
{
    unsigned char r, g, b;
} Pixel;

// Uma imagem
typedef struct
{
    int largura, altura;
    int canais;
    Pixel *dados_pixels;
} Imagem;

// Variáveis Globais
Imagem imagem_original, imagem_processada;

// Protótipos
void carregar_imagem(char *nome_arquivo, Imagem *img);
void desenhar_linha(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade);
void desenhar_retangulo(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade);
float calcular_luminancia(Pixel p);
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho);
float calcular_variancia(Pixel bloco[], int tamanho);
float calcular_limiar_adaptativo(int largura, int altura, Pixel matriz_entrada[][largura], int tamanho_bloco);
void processar_imagem_forense(int largura, int altura, Pixel matriz_entrada[][largura], Pixel matriz_saida[][largura]);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("forensics [origem]\n");
        exit(1);
    }

    carregar_imagem(argv[1], &imagem_original);
    printf("Origem   : %s %d x %d\n", argv[1], imagem_original.largura, imagem_original.altura);
    printf("Processando...\n");

    int total_pixels = imagem_original.largura * imagem_original.altura;
    imagem_processada = imagem_original;
    imagem_processada.dados_pixels = malloc(total_pixels * sizeof(Pixel));
    memset(imagem_processada.dados_pixels, 0, total_pixels * sizeof(Pixel));

    Pixel (*matriz_entrada)[imagem_original.largura]  = (Pixel(*)[imagem_original.altura]) imagem_original.dados_pixels;
    Pixel (*matriz_saida)[imagem_original.largura] = (Pixel(*)[imagem_original.altura]) imagem_processada.dados_pixels;

    processar_imagem_forense(imagem_original.largura, imagem_original.altura, matriz_entrada, matriz_saida);

    // NÃO ALTERAR A PARTIR DAQUI!

    stbi_write_jpg("saida.jpg", imagem_processada.largura, imagem_processada.altura, 3, matriz_saida, 90);

    free(imagem_original.dados_pixels);
    free(imagem_processada.dados_pixels);
}

void carregar_imagem(char *nome_arquivo, Imagem *img)
{
    img->dados_pixels = (Pixel *)stbi_load(nome_arquivo, &img->largura, &img->altura, &img->canais, 0);
    if (!img->dados_pixels)
    {
        printf("Erro ao carregar imagem via STB\n");
        exit(1);
    }
    printf("Load: %d x %d x %d\n", img->largura, img->altura, img->canais);
    for (int i = 0; i < 16; i++)
        printf("[%02X %02X %02X] ", img->dados_pixels[i].r, img->dados_pixels[i].g, img->dados_pixels[i].b);
    printf("\n");
}

void desenhar_linha(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    int metade_espessura = espessura / 2;
    
    while (1) {
        for (int i = -metade_espessura; i <= metade_espessura; i++) {
            for (int j = -metade_espessura; j <= metade_espessura; j++) {
                int xi = x0 + i, yj = y0 + j;
                if (xi >= 0 && xi < largura && yj >= 0 && yj < altura) {
                    matriz[yj][xi].r = (unsigned char)(matriz[yj][xi].r * (1.0f - opacidade) + cor.r * opacidade);
                    matriz[yj][xi].g = (unsigned char)(matriz[yj][xi].g * (1.0f - opacidade) + cor.g * opacidade);
                    matriz[yj][xi].b = (unsigned char)(matriz[yj][xi].b * (1.0f - opacidade) + cor.b * opacidade);
                }
            }
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 < dy)  { err += dx; y0 += sy; }
    }
}

void desenhar_retangulo(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade) {
    desenhar_linha(largura, altura, matriz, x0, y0, x1, y0, cor, espessura, opacidade);
    desenhar_linha(largura, altura, matriz, x0, y1, x1, y1, cor, espessura, opacidade);
    desenhar_linha(largura, altura, matriz, x0, y0, x0, y1, cor, espessura, opacidade);
    desenhar_linha(largura, altura, matriz, x1, y0, x1, y1, cor, espessura, opacidade);
}

float calcular_luminancia(Pixel p) {
    return 0.59f * p.g + 0.30f * p.r + 0.11f * p.b;
}

float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho) {
    float soma = 0.0f;
    for (int i = 0; i < tamanho; i++) {
        float dif_r = bloco1[i].r - bloco2[i].r;
        float dif_g = bloco1[i].g - bloco2[i].g;
        float dif_b = bloco1[i].b - bloco2[i].b;
        soma += dif_r*dif_r + dif_g*dif_g + dif_b*dif_b;
    }
    return soma / (tamanho * 3);
}

float calcular_variancia(Pixel bloco[], int tamanho) {
    float media_r = 0, media_g = 0, media_b = 0;
    for (int i = 0; i < tamanho; i++) {
        media_r += bloco[i].r; media_g += bloco[i].g; media_b += bloco[i].b;
    }
    media_r /= tamanho; media_g /= tamanho; media_b /= tamanho;
    
    float variancia = 0;
    for (int i = 0; i < tamanho; i++) {
        float dif_r = bloco[i].r - media_r;
        float dif_g = bloco[i].g - media_g;
        float dif_b = bloco[i].b - media_b;
        variancia += dif_r*dif_r + dif_g*dif_g + dif_b*dif_b;
    }
    return variancia / (tamanho * 3);
}

static int cmp_float(const void *a, const void *b) {
    float fa = *(float*)a, fb = *(float*)b;
    return (fa > fb) - (fa < fb);
}

float calcular_limiar_adaptativo(int largura, int altura, Pixel matriz_entrada[][largura], int tamanho_bloco) {
    int blocos_em_x = largura / tamanho_bloco;
    int blocos_em_y = altura / tamanho_bloco;
    int total_blocos = blocos_em_x * blocos_em_y;

    float *variancias = malloc(total_blocos * sizeof(float));
    if (!variancias) return 100.0f;

    int indice = 0;
    for (int by = 0; by < blocos_em_y; by++) {
        for (int bx = 0; bx < blocos_em_x; bx++) {
            int x_inicial = bx * tamanho_bloco, y_inicial = by * tamanho_bloco;
            Pixel bloco[tamanho_bloco * tamanho_bloco];
            int k = 0;
            for (int i = 0; i < tamanho_bloco; i++)
                for (int j = 0; j < tamanho_bloco; j++)
                    bloco[k++] = matriz_entrada[y_inicial + i][x_inicial + j];
            variancias[indice++] = calcular_variancia(bloco, k);
        }
    }

    qsort(variancias, indice, sizeof(float), cmp_float);

    float percentil_40 = variancias[(int)(indice * 0.40f)];
    float limiar_final = percentil_40 > 100.0f ? percentil_40 : 100.0f;
    printf("  Threshold adaptativo (max(p40,100)): %.1f\n", limiar_final);

    free(variancias);
    return limiar_final;
}

void processar_imagem_forense(int largura, int altura, Pixel matriz_entrada[][largura], Pixel matriz_saida[][largura]) {
    int TAMANHO_BLOCO = 24;
    float LIMIAR_SIMILARIDADE = 250.0f;
    int DISTANCIA_MINIMA = TAMANHO_BLOCO * 2;

    int num_blocos_x = largura / TAMANHO_BLOCO;
    int num_blocos_y = altura / TAMANHO_BLOCO;

    for (int i = 0; i < altura; i++)
        for (int j = 0; j < largura; j++)
            matriz_saida[i][j] = matriz_entrada[i][j];

    float variancia_minima = calcular_limiar_adaptativo(largura, altura, matriz_entrada, TAMANHO_BLOCO);

    Pixel cor_destaque = {255, 0, 0}; // Vermelho

    char *blocos_verificados = (char*)calloc(num_blocos_x * num_blocos_y, sizeof(char));
    if (!blocos_verificados) return;

    for (int by1 = 0; by1 < num_blocos_y; by1++) {
        for (int bx1 = 0; bx1 < num_blocos_x; bx1++) {
            int indice1 = by1 * num_blocos_x + bx1;
            if (blocos_verificados[indice1]) continue;

            int x1_inicio = bx1 * TAMANHO_BLOCO, y1_inicio = by1 * TAMANHO_BLOCO;
            int x1_fim = x1_inicio + TAMANHO_BLOCO - 1, y1_fim = y1_inicio + TAMANHO_BLOCO - 1;

            Pixel bloco1[TAMANHO_BLOCO * TAMANHO_BLOCO];
            int tam1 = 0;
            for (int i = 0; i < TAMANHO_BLOCO; i++)
                for (int j = 0; j < TAMANHO_BLOCO; j++)
                    if (y1_inicio+i < altura && x1_inicio+j < largura)
                        bloco1[tam1++] = matriz_entrada[y1_inicio+i][x1_inicio+j];
            if (tam1 == 0) continue;

            if (calcular_variancia(bloco1, tam1) < variancia_minima) continue;

            int centro_x1 = x1_inicio + TAMANHO_BLOCO/2, centro_y1 = y1_inicio + TAMANHO_BLOCO/2;

            float melhor_similaridade = 1e30f;
            int   melhor_bx2 = -1, melhor_by2 = -1;

            for (int by2 = 0; by2 < num_blocos_y; by2++) {
                for (int bx2 = 0; bx2 < num_blocos_x; bx2++) {
                    int indice2 = by2 * num_blocos_x + bx2;
                    if (indice2 == indice1 || blocos_verificados[indice2]) continue;

                    int x2_inicio = bx2 * TAMANHO_BLOCO, y2_inicio = by2 * TAMANHO_BLOCO;
                    int centro_x2 = x2_inicio + TAMANHO_BLOCO/2, centro_y2 = y2_inicio + TAMANHO_BLOCO/2;

                    int delta_x = centro_x1 - centro_x2, delta_y = centro_y1 - centro_y2;
                    if (delta_x*delta_x + delta_y*delta_y < DISTANCIA_MINIMA*DISTANCIA_MINIMA) continue;

                    Pixel bloco2[TAMANHO_BLOCO * TAMANHO_BLOCO];
                    int tam2 = 0;
                    for (int i = 0; i < TAMANHO_BLOCO; i++)
                        for (int j = 0; j < TAMANHO_BLOCO; j++)
                            if (y2_inicio+i < altura && x2_inicio+j < largura)
                                bloco2[tam2++] = matriz_entrada[y2_inicio+i][x2_inicio+j];
                    if (tam2 == 0) continue;

                    if (calcular_variancia(bloco2, tam2) < variancia_minima) continue;

                    int t_min = tam1 < tam2 ? tam1 : tam2;
                    float similaridade = comparar_blocos(bloco1, bloco2, t_min);
                    if (similaridade < melhor_similaridade) {
                        melhor_similaridade = similaridade;
                        melhor_bx2 = bx2;
                        melhor_by2 = by2;
                    }
                }
            }

            if (melhor_bx2 >= 0 && melhor_similaridade < LIMIAR_SIMILARIDADE) {
                int indice2 = melhor_by2 * num_blocos_x + melhor_bx2;
                blocos_verificados[indice1] = 1;
                blocos_verificados[indice2] = 1;

                int x2_inicio = melhor_bx2 * TAMANHO_BLOCO, y2_inicio = melhor_by2 * TAMANHO_BLOCO;
                int x2_fim = x2_inicio + TAMANHO_BLOCO - 1, y2_fim = y2_inicio + TAMANHO_BLOCO - 1;
                int centro_x2 = x2_inicio + TAMANHO_BLOCO/2, centro_y2 = y2_inicio + TAMANHO_BLOCO/2;

                desenhar_retangulo(largura, altura, matriz_saida, x1_inicio, y1_inicio, x1_fim, y1_fim, cor_destaque, 2, 0.4f);
                desenhar_retangulo(largura, altura, matriz_saida, x2_inicio, y2_inicio, x2_fim, y2_fim, cor_destaque, 2, 0.4f);
                
                desenhar_linha(largura, altura, matriz_saida, centro_x1, centro_y1, centro_x2, centro_y2, cor_destaque, 1, 0.6f);
            }
        }
    }

    free(blocos_verificados);
}