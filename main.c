#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings e memcmp
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

// Um bloco 8 x 8 (64 pixels)
typedef struct
{
    int altura, largura;     // Coordenadas da âncora (canto superior esquerdo)
    unsigned char lum[64];  // Array contendo a luminância de cada pixel do bloco
} Bloco;

// As 2 imagens
Img in, out;

// Protótipos
void load(char *name, Img *pic);
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness);
int comparar_blocos(const void *a, const void *b);

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

    // Cria imagem de saída e reserva a memória
    int tam = in.width * in.height;
    out = in;
    out.pixels = malloc(tam * sizeof(Pixel));
    
    // Converte para interpretar como matrizes
    Pixel(*pin)[in.width] = (Pixel(*)[in.height])in.pixels;
    Pixel(*pout)[in.width] = (Pixel(*)[in.height])out.pixels;

    // Inicializa a imagem de saída copiando os pixels exatos da imagem original
    int altura, largura;
    for (altura = 0; altura < in.height; altura++)
    {
        for (largura = 0; largura < in.width; largura++)
        {
            pout[altura][largura].r = pin[altura][largura].r;
            pout[altura][largura].g = pin[altura][largura].g;
            pout[altura][largura].b = pin[altura][largura].b;
        }
    }

    // Calcula até onde a lupa de 16x16 pode ir sem estourar o limite da imagem
    int max_altura = in.height - 8;
    int max_largura = in.width - 8;

    // Calcula o total de blocos sobrepostos gerados pela janela deslizante
    int total_blocos = (max_altura + 1) * (max_largura + 1);
    Bloco *todos_os_blocos = malloc(total_blocos * sizeof(Bloco));

    if (todos_os_blocos == NULL)
    {
        printf("Erro: Memória insuficiente para alocar o vetor de blocos.\n");
        exit(1);
    }

    unsigned char lum;
    int pos_lum;
    int pos_bloc = 0;  

    // 1. LAÇOS EXTERNOS: Movem a âncora (a lupa) pixel por pixel pela imagem inteira
    for (int altura_ancora = 0; altura_ancora <= max_altura; altura_ancora++)
    {
        for (int largura_ancora = 0; largura_ancora <= max_largura; largura_ancora++)
        {
            // Instancia a estrutura do bloco atual e armazena suas coordenadas de origem
            Bloco bloco;
            bloco.altura = altura_ancora;
            bloco.largura = largura_ancora;

            // Reseta o índice interno do array de luminância para começar da posição 0
            pos_lum = 0;

            // 2. LAÇOS INTERNOS: Leem e extraem os 64 pixels que estão dentro do bloco
            for (int deslocamento_altura = 0; deslocamento_altura < 8; deslocamento_altura++)
            {
                for (int deslocamento_largura = 0; deslocamento_largura < 8; deslocamento_largura++)
                {
                    // Calcula a luminância (tom de cinza) ponderada para o olho humano
                    lum = (unsigned char)(pin[altura_ancora + deslocamento_altura][largura_ancora + deslocamento_largura].r * 0.30 +
                                          pin[altura_ancora + deslocamento_altura][largura_ancora + deslocamento_largura].g * 0.59 +
                                          pin[altura_ancora + deslocamento_altura][largura_ancora + deslocamento_largura].b * 0.11);

                    // Insere na "foto" unidimensional do bloco e incrementa a posição
                    bloco.lum[pos_lum] = lum;
                    pos_lum++;
                }
            }

            // Armazena o bloco totalmente preenchido no vetor dinâmico
            todos_os_blocos[pos_bloc] = bloco;
            pos_bloc++;
        }
    }

    // Ordena o vetor de blocos usando o Quick Sort baseado no conteúdo lexicográfico dos pixels
    qsort(todos_os_blocos, total_blocos, sizeof(Bloco), comparar_blocos);

    // Distância mínima em pixels para evitar falsos positivos com regiões de cor sólida vizinhas (ex: céu)
    int limite_distancia = 8; 

    // 3. LAÇO DE BUSCA: Percorre o vetor ordenado procurando blocos adjacentes idênticos
    for (int pos = 0; pos < total_blocos - 1; pos++) 
    {
        // Verifica se os 64 pixels de luminância do bloco atual e do próximo são idênticos
        if (memcmp(todos_os_blocos[pos].lum, todos_os_blocos[pos+1].lum, 64) == 0) 
        {
            // Resgata os pontos de ancoragem originais de ambos os blocos
            int alt1 = todos_os_blocos[pos].altura;
            int larg1 = todos_os_blocos[pos].largura;
            
            int alt2 = todos_os_blocos[pos+1].altura;
            int larg2 = todos_os_blocos[pos+1].largura;

            // Aplica o filtro de distância para descartar blocos que estão colados um no outro
            if (abs(alt1 - alt2) > limite_distancia || abs(larg1 - larg2) > limite_distancia) 
            {
                // PINTA OS DOIS BLOCOS DE VERMELHO NA MATRIZ DE SAÍDA (pout)
                for (int dy = 0; dy < 8; dy++) 
                {
                    for (int dx = 0; dx < 8; dx++) 
                    {
                        // Bloco original (A)
                        pout[alt1 + dy][larg1 + dx].r = 255;
                        pout[alt1 + dy][larg1 + dx].g = 0;
                        pout[alt1 + dy][larg1 + dx].b = 0;

                        // Bloco clonado (B)
                        pout[alt2 + dy][larg2 + dx].r = 255;
                        pout[alt2 + dy][larg2 + dx].g = 0;
                        pout[alt2 + dy][larg2 + dx].b = 0;
                    }
                }

                // INDICAÇÃO VISUAL EXIGIDA: Desenha uma linha azul conectando o centro de gravidade das regiões clonadas
                Pixel azul = {0, 0, 255};
                draw_line(out.width, out.height, pout, larg1 + 8, alt1 + 8, larg2 + 8, alt2 + 8, azul, 1);
            }
        }
    }

    // Grava a imagem processada final como JPEG
    stbi_write_jpg("saida.jpg", out.width, out.height, 3, pout, 90);

    // Libera devidamente toda a memória alocada dinamicamente
    free(in.pixels);
    free(out.pixels);
    free(todos_os_blocos);
    
    printf("Processamento concluído com sucesso! Verifique o arquivo 'saida.jpg'.\n");
    return 0;
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
    for (int i = 0; i < 8; i++)
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

// Função de Callback exigida pelo qsort para ordenar os blocos em ordem alfabética de pixels (Lexicográfica)
int comparar_blocos(const void *a, const void *b) {
    const Bloco *blocoA = (const Bloco *)a;
    const Bloco *blocoB = (const Bloco *)b;

    // Compara elemento por elemento do array de luminância até encontrar a primeira divergência
    for (int i = 0; i < 64; i++) {
        if (blocoA->lum[i] < blocoB->lum[i]) {
            return -1; 
        } 
        else if (blocoA->lum[i] > blocoB->lum[i]) {
            return 1;  
        }
    }
    // Retorna 0 somente se os 64 tons de cinza forem idênticos
    return 0;
}