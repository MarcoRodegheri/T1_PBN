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
    Pixel(*pin)[in.width] = (Pixel(*)[in.height])in.pixels;
    Pixel(*pout)[in.width] = (Pixel(*)[in.height])out.pixels;

    //
    // Neste ponto, voce deve implementar o algoritmo!
    // (ou chamar funcoes para fazer isso)
    //
    // Aplica o algoritmo em pin e gera a saida em pout
    // ...
    //


    
    int grupo_altura, grupo_largura;
    int px_comparar;
    int px_atual_altura, px_atual_largura;
    int pintar, grupo_igual = 1; // inicializa em 1 pra entrar 1 vez no for
    int contador_px_igual = 0;

    // Navega de grupo de 3 em 3
    for (grupo_altura = 0; grupo_altura < in.height; grupo_altura += 3)
    {
        // Navega de grupo de 3 em 3
        for (grupo_largura = 0; grupo_largura < in.width; grupo_largura += 3)
        {

            // Pega os pixeis a serem comparados
            for (px_comparar = 1; px_comparar < in.height; px_comparar++)
            {
                // Caso encontrar clone, passa denovo pintando
                for (pintar = 0; pintar < 2 && grupo_igual != 0; pintar++)
                {

                    // Toda vez tem de contar de 0 a 9, precisa ser reinciado
                    contador_px_igual = 0;

                    // Deixa fixo um 3 x 3 para comparar 1 grupo com o resto da imagem
                    for (px_atual_altura = 0; px_atual_altura < 3; px_atual_altura++)
                    {
                        // Deixa fixo um 3 x 3 para comparar 1 grupo com o resto da imagem
                        for (px_atual_largura = 0; px_atual_largura < 3; px_atual_largura++)
                        {

                            // Descobre se o grupo é igual
                            if (grupo_igual == 0)
                            {
                                // compara 1 fixo com o resto da imagem, pegando de 3 em 3
                                if (pin[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].r == pin[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].r && pin[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].g == pin[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].g && pin[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].b == pin[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].b)
                                {
                                    contador_px_igual++;
                                }
                            }

                            // Se o grupo é igual, somente altera a cor indicando que é clone
                            if (grupo_igual == 1)
                            {
                                pout[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].r = 255;
                                pout[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].g = 0;
                                pout[px_atual_altura + grupo_altura][px_atual_largura + grupo_largura].b = 0;

                                pout[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].r = 255;
                                pout[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].g = 0;
                                pout[px_atual_altura + grupo_altura + (3 * px_comparar)][px_atual_largura + grupo_largura + (3 * px_comparar)].b = 0;
                            }

                            // Caso o grupo 3 x 3 seja igual
                            if (contador_px_igual == 9)
                            {
                                // Achou, portanto vai rodar for denovo pintando
                                grupo_igual = 1;
                            }
                            else
                            {
                                // Enquanto grupo não é igual, não vai pintar
                                grupo_igual = 0;
                            }
                        }
                    }
                }
            }
        }
    }


    /*
    //
    // Neste ponto, voce deve implementar o algoritmo!
    //

    
    int tam_bloco = 8; // Equivalente ao seu 3, mas 8 evita que ele ache blocos iguais atoa.

    // 1. Percorre a imagem para pegar o grupo de ORIGEM
    for (int grupo_altura = 0; grupo_altura <= in.height - tam_bloco; grupo_altura++)
    {
        for (int grupo_largura = 0; grupo_largura <= in.width - tam_bloco; grupo_largura++)
        {
            
            // 2. Procura um grupo IGUAL no resto da imagem (DESTINO)
            for (int comparar_altura = grupo_altura; comparar_altura <= in.height - tam_bloco; comparar_altura++)
            {
                // Se estiver na mesma linha da altura, começa a procurar um pouco mais pra frente na largura 
                // para não comparar o grupo com ele mesmo
                int inicio_largura = (comparar_altura == grupo_altura) ? (grupo_largura + tam_bloco) : 0; 

                for (int comparar_largura = inicio_largura; comparar_largura <= in.width - tam_bloco; comparar_largura++)
                {
                    int grupo_igual = 1; // Assumimos que o grupo é igual no começo

                    // 3. Compara pixel a pixel dentro do grupo (fixando o tamanho do bloco)
                    for (int px_atual_altura = 0; px_atual_altura < tam_bloco; px_atual_altura++)
                    {
                        for (int px_atual_largura = 0; px_atual_largura < tam_bloco; px_atual_largura++)
                        {
                            // Pega o pixel da origem e o pixel do destino
                            Pixel p1 = pin[grupo_altura + px_atual_altura][grupo_largura + px_atual_largura];
                            Pixel p2 = pin[comparar_altura + px_atual_altura][comparar_largura + px_atual_largura];

                            // Descobre se os pixels do grupo são diferentes
                            if (p1.r != p2.r || p1.g != p2.g || p1.b != p2.b)
                            {
                                grupo_igual = 0; // Não é clone
                                break; // Sai do for da largura (px_atual_largura)
                            }
                        }
                        if (grupo_igual == 0) break; // Sai do for da altura (px_atual_altura)
                    }

                    // 4. Se o grupo é igual, altera a cor indicando que é clone
                    if (grupo_igual == 1)
                    {
                        for (int px_atual_altura = 0; px_atual_altura < tam_bloco; px_atual_altura++)
                        {
                            for (int px_atual_largura = 0; px_atual_largura < tam_bloco; px_atual_largura++)
                            {
                                // Pinta o grupo original (Verde, por exemplo)
                                pout[grupo_altura + px_atual_altura][grupo_largura + px_atual_largura].r = 0;
                                pout[grupo_altura + px_atual_altura][grupo_largura + px_atual_largura].g = 255;
                                pout[grupo_altura + px_atual_altura][grupo_largura + px_atual_largura].b = 0;

                                // Pinta o grupo clonado (Vermelho, conforme seu código original)
                                pout[comparar_altura + px_atual_altura][comparar_largura + px_atual_largura].r = 255;
                                pout[comparar_altura + px_atual_altura][comparar_largura + px_atual_largura].g = 0;
                                pout[comparar_altura + px_atual_altura][comparar_largura + px_atual_largura].b = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    */


    /*

    // Exemplo: inverte as cores
    for (int i = 0; i < in.height; i++)
    {
        for (int j = 0; j < in.width; j++)
        {
            pout[i][j].r = 255 - pin[i][j].r;
            pout[i][j].g = 255 - pin[i][j].g;
            pout[i][j].b = 255 - pin[i][j].b;
        }
    }

    // Exemplo: desenha uma linha vermelha de um canto a outro da imagem
    Pixel red = {255, 0, 0};
    draw_line(out.width, out.height, pout, 0, 0, out.width - 1, out.height - 1, red, 5);

    */

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
        // Draw a square of size thickness x thickness centered at (x0, y0)
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
