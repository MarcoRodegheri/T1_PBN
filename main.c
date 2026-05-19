#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings
#include <time.h>
#include <math.h>

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

// Guarda os dados de onde um pedaço foi copiado e colado
typedef struct
{
    int x1, y1; // Onde ficava o pedaço original (centro)
    int x2, y2; // Onde o pedaço foi colado (centro)
    int dx, dy; // A distância e a direção do movimento
    int valido; // Diz se é uma cópia real (1) ou alarme falso (0)
} Clone;

// As 2 imagens
Img in, out;

// Protótipos
void load(char *name, Img *pic);
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness);
void desenha_quadrado(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness);
float calcular_variancia(Pixel bloco[], int tamanho);
float comparar_blocos(Pixel b1[], Pixel b2[], int tamanho);
float var_textura(int w, int h, Pixel img[][w], int tam_bloco);
void detectar_clones(int w, int h, Pixel pin[][w], Pixel pout[][w]);

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
    Pixel(*pin)[in.width] = (Pixel(*)[in.width])in.pixels;
    Pixel(*pout)[in.width] = (Pixel(*)[in.width])out.pixels;

    // Neste ponto, voce deve implementar o algoritmo!
    // (ou chamar funcoes para fazer isso)
    detectar_clones(in.width, in.height, pin, pout);

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
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    int half = thickness / 2;
    while (1)
    {
        for (int i = -half; i <= half; i++)
        {
            for (int j = -half; j <= half; j++)
            {
                int xi = x0 + i, yj = y0 + j;
                if (xi >= 0 && xi < width && yj >= 0 && yj < height)
                    img[yj][xi] = color;
            }
        }
        if (x0 == x1 && y0 == y1)
            break;
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// Desenha um quadrado na imagem juntando 4 linhas
void desenha_quadrado(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness)
{
    draw_line(width, height, img, x0, y0, x1, y0, color, thickness);
    draw_line(width, height, img, x0, y1, x1, y1, color, thickness);
    draw_line(width, height, img, x0, y0, x0, y1, color, thickness);
    draw_line(width, height, img, x1, y0, x1, y1, color, thickness);
}

// Mede o "detalhe" de um pedaço. Se for liso (céu), dá quase zero. Se for detalhado (folhas), dá um número alto.
float calcular_variancia(Pixel bloco[], int tamanho)
{
    float m_r = 0, m_g = 0, m_b = 0;
    for (int i = 0; i < tamanho; i++)
    {
        m_r += bloco[i].r;
        m_g += bloco[i].g;
        m_b += bloco[i].b;
    }
    m_r /= tamanho;
    m_g /= tamanho;
    m_b /= tamanho;

    float var = 0;
    for (int i = 0; i < tamanho; i++)
    {
        float dr = bloco[i].r - m_r;
        float dg = bloco[i].g - m_g;
        float db = bloco[i].b - m_b;
        var += dr * dr + dg * dg + db * db;
    }
    return var / (tamanho * 3);
}

// Compara duas fotos quadradas. Quanto menor o resultado, mais idênticas elas são.
float comparar_blocos(Pixel b1[], Pixel b2[], int tamanho)
{
    float soma = 0;
    for (int i = 0; i < tamanho; i++)
    {
        float dr = b1[i].r - b2[i].r;
        float dg = b1[i].g - b2[i].g;
        float db = b1[i].b - b2[i].b;
        soma += dr * dr + dg * dg + db * db;
    }
    return soma / (tamanho * 3);
}

// Organiza a lista de números do menor para o maior
int qsort_float(const void *a, const void *b)
{
    float fa = *(float *)a;
    float fb = *(float *)b;
    return (fa > fb) - (fa < fb);
}

// Descobre o nível mínimo de varinxia de textura que um bloco precisa ter para não ser ignorado
float var_textura(int w, int h, Pixel img[][w], int tam_bloco)
{
    int blocos_x = w / tam_bloco;
    int blocos_y = h / tam_bloco;
    int total_blocos = blocos_x * blocos_y;

    float *variancias = malloc(total_blocos * sizeof(float));
    int total_validos = 0;

    for (int b_y = 0; b_y < blocos_y; b_y++)
    {
        for (int b_x = 0; b_x < blocos_x; b_x++)
        {
            int start_x = b_x * tam_bloco;
            int start_y = b_y * tam_bloco;
            Pixel bloco[tam_bloco * tam_bloco];
            int tam_b = 0;

            for (int y = 0; y < tam_bloco && (start_y + y) < h; y++)
                for (int x = 0; x < tam_bloco && (start_x + x) < w; x++)
                    bloco[tam_b++] = img[start_y + y][start_x + x];

            variancias[total_validos++] = calcular_variancia(bloco, tam_b);
        }
    }

    qsort(variancias, total_validos, sizeof(float), qsort_float);

    // Escolhe ignorar os primeiros 40% dos blocos mais lisos da imagem
    float corte = variancias[(int)(total_validos * 0.40f)];
    free(variancias);

    return (corte > 100.0f) ? corte : 100.0f;
}

void detectar_clones(int w, int h, Pixel pin[][w], Pixel pout[][w])
{
    int tam_bloco = 24;
    float tolerancia = 250.0f;
    int dist_minima = tam_bloco * 2;

    int blocos_x = w / tam_bloco;
    int blocos_y = h / tam_bloco;

    // Copia a imagem original para a imagem de saída
    for (int i = 0; i < h; i++)
    {
        for (int j = 0; j < w; j++)
        {
            pout[i][j] = pin[i][j];
        }
    }
    float limiar_var = var_textura(w, h, pin, tam_bloco);
    char *visitados = calloc(blocos_x * blocos_y, sizeof(char));

    Clone matches[5000];
    int total_matches = 0;

    // Procurar blocos parecidos
    for (int by1 = 0; by1 < blocos_y; by1++)
    {
        for (int bx1 = 0; bx1 < blocos_x; bx1++)
        {
            int idx1 = by1 * blocos_x + bx1;
            if (visitados[idx1])
                continue;

            int x1 = bx1 * tam_bloco, y1 = by1 * tam_bloco;
            Pixel b1[tam_bloco * tam_bloco];
            int t1 = 0;

            for (int i = 0; i < tam_bloco && (y1 + i) < h; i++)
                for (int j = 0; j < tam_bloco && (x1 + j) < w; j++)
                    b1[t1++] = pin[y1 + i][x1 + j];

            // Pula se o bloco for liso demais (ex: céu azul)
            if (t1 == 0 || calcular_variancia(b1, t1) < limiar_var)
                continue;

            float melhor_score = 1e30f;
            int melhor_bx2 = -1, melhor_by2 = -1;

            // Varre o resto da imagem para achar um gêmeo para o Bloco 1
            for (int by2 = 0; by2 < blocos_y; by2++)
            {
                for (int bx2 = 0; bx2 < blocos_x; bx2++)
                {
                    int idx2 = by2 * blocos_x + bx2;
                    if (idx2 == idx1 || visitados[idx2])
                        continue;

                    int x2 = bx2 * tam_bloco, y2 = by2 * tam_bloco;

                    // Pula se o segundo bloco estiver colado ou muito perto do primeiro
                    int dx = (x1 + tam_bloco / 2) - (x2 + tam_bloco / 2);
                    int dy = (y1 + tam_bloco / 2) - (y2 + tam_bloco / 2);
                    if ((dx * dx + dy * dy) < (dist_minima * dist_minima))
                        continue;

                    Pixel b2[tam_bloco * tam_bloco];
                    int t2 = 0;
                    for (int i = 0; i < tam_bloco && (y2 + i) < h; i++)
                        for (int j = 0; j < tam_bloco && (x2 + j) < w; j++)
                            b2[t2++] = pin[y2 + i][x2 + j];

                    if (t2 == 0 || calcular_variancia(b2, t2) < limiar_var)
                        continue;

                    int t_min = (t1 < t2) ? t1 : t2;
                    float score = comparar_blocos(b1, b2, t_min);

                    if (score < melhor_score)
                    {
                        melhor_score = score;
                        melhor_bx2 = bx2;
                        melhor_by2 = by2;
                    }
                }
            }

            // Se achou um bloco muito parecido, salva as posições dele na lista
            if (melhor_bx2 >= 0 && melhor_score < tolerancia && total_matches < 5000)
            {
                visitados[idx1] = 1;
                visitados[melhor_by2 * blocos_x + melhor_bx2] = 1;

                matches[total_matches].x1 = x1;
                matches[total_matches].y1 = y1;
                matches[total_matches].x2 = melhor_bx2 * tam_bloco;
                matches[total_matches].y2 = melhor_by2 * tam_bloco;
                matches[total_matches].dx = matches[total_matches].x2 - x1;
                matches[total_matches].dy = matches[total_matches].y2 - y1;
                matches[total_matches].valido = 0;
                total_matches++;
            }
        }
    }

    
    // Remove copias falsas
    int cluster_min = 3;
    for (int i = 0; i < total_matches; i++)
    {
        int semelhantes = 0;
        for (int j = 0; j < total_matches; j++)
        {
            if (abs(matches[i].dx - matches[j].dx) <= tam_bloco &&
                abs(matches[i].dy - matches[j].dy) <= tam_bloco)
            {
                semelhantes++;
            }
        }
        // Se achou um grupo junto, confirma que é uma cópia real
        if (semelhantes >= cluster_min)
            matches[i].valido = 1;
    }

    // Desenhar na imagem
    Pixel roxo = {180, 50, 255};
    for (int i = 0; i < total_matches; i++)
    {
        if (!matches[i].valido)
            continue;

        // Desenha quadrados roxos em volta do blocos copiados
        desenha_quadrado(w, h, pout, matches[i].x1, matches[i].y1, matches[i].x1 + tam_bloco - 1, matches[i].y1 + tam_bloco - 1, roxo, 2);
        desenha_quadrado(w, h, pout, matches[i].x2, matches[i].y2, matches[i].x2 + tam_bloco - 1, matches[i].y2 + tam_bloco - 1, roxo, 2);

        // Liga o centro do bloco copiado e colado
        int c_x1 = matches[i].x1 + tam_bloco / 2;
        int c_y1 = matches[i].y1 + tam_bloco / 2;
        int c_x2 = matches[i].x2 + tam_bloco / 2;
        int c_y2 = matches[i].y2 + tam_bloco / 2;
        draw_line(w, h, pout, c_x1, c_y1, c_x2, c_y2, roxo, 1);
    }

    free(visitados);
}