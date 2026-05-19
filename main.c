#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

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

// Armazenar areas similares
typedef struct
{
    int x1, y1, x2, y2; // Coordenadas: (x1,y1) de onde copiou, (x2,y2) para onde colou
    int dx, dy;         // Vetor de deslocamento (diferença entre x2-x1 e y2-y1)
    int valido;         // Flag: 0 = Falso/Suspeito, 1 = Confirmado pela filtragem
} Similar;

// As 2 imagens (Variáveis Globais que guardam a imagem lida e a que será salva)
Img in, out;

// Protótipos das funções (Avisa ao compilador que essas funções existem e serão detalhadas abaixo)
void load(char *name, Img *pic);
void draw_line(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade);
void desenhar_retangulo(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade);
void draw_line_transparente(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float intensidade);
float calcular_luminancia(Pixel p);
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho);
float calcular_variancia(Pixel bloco[], int tamanho);
float variacao_textura(int largura, int altura, Pixel matriz_entrada[][largura], int tamanho_bloco);
void detecta_clone(int largura, int altura, Pixel matriz_entrada[][largura], Pixel matriz_saida[][largura]);

// Função Principal do programa
int main(int argc, char *argv[])
{
    // Verifica se o usuário passou o nome da imagem via linha de comando
    if (argc < 2)
    {
        printf("forensics [origem]\n");
        exit(1); // Encerra o programa com erro se não passou
    }

    // Chama a função load para ler a imagem cujo nome está em argv[1] e salva em 'in'
    load(argv[1], &in);

    // Exibe as dimensões da imagem lida na tela
    printf("Origem   : %s %d x %d\n", argv[1], in.width, in.height);

    printf("Processando...\n");

    // Calcula o total de pixels da imagem (Largura * Altura)
    int tam = in.width * in.height;
    out = in; // Copia as configurações de largura/altura/canais para a imagem de saída

    // Aloca memória RAM suficiente para guardar todos os pixels da imagem de saída
    out.pixels = malloc(tam * sizeof(Pixel));
    // Preenche a memória alocada com zeros (deixa a imagem preta inicialmente)
    memset(out.pixels, 0, tam * sizeof(Pixel));

    // Converte o vetor linear (1D) de pixels em um formato de Matriz 2D (Linhas e Colunas)
    // Isso facilita muito para acessar a imagem usando [y][x]
    Pixel(*pin)[in.width] = (Pixel(*)[in.width])in.pixels;
    Pixel(*pout)[in.width] = (Pixel(*)[in.width])out.pixels;

    // Chama a função principal do seu algoritmo passando as matrizes 2D
    detecta_clone(in.width, in.height, pin, pout);

    // Pega a matriz resultante ('pout') e salva como um arquivo "saida.jpg" com qualidade 90
    stbi_write_jpg("saida.jpg", out.width, out.height, 3, pout, 90);

    // Libera a memória RAM que foi alocada para as imagens (boa prática de programação)
    free(in.pixels);
    free(out.pixels);

    return 0; // Finaliza o programa com sucesso
}

// Função para carregar a imagem do disco rígido
void load(char *name, Img *pic)
{
    // Usa a biblioteca stb_image para ler o arquivo e jogar os dados no ponteiro de pixels
    pic->pixels = (Pixel *)stbi_load(name, &pic->width, &pic->height, &pic->channels, 0);

    // Se o ponteiro voltar vazio, significa que não conseguiu abrir o arquivo
    if (!pic->pixels)
    {
        printf("STB loading error\n");
        exit(1); // Encerra o programa
    }

    // Imprime as informações lidas da imagem
    printf("Load: %d x %d x %d\n", pic->width, pic->height, pic->channels);

    // Loop simples para imprimir os valores RGB dos primeiros 16 pixels no terminal (apenas para teste)
    for (int i = 0; i < 16; i++)
    {
        printf("[%02X %02X %02X] ", pic->pixels[i].r, pic->pixels[i].g, pic->pixels[i].b);
    }
    printf("\n");
}

// Algoritmo de Bresenham modificado para desenhar uma linha com espessura e opacidade
void draw_line(int width, int height, Pixel img[][width], int x0, int y0, int x1, int y1, Pixel color, int thickness, float opacity)
{
    // Calcula a distância entre os pontos X e Y
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;

    int half = thickness / 2; // Calcula a metade da espessura para desenhar o "pincel"

    // Loop infinito que vai desenhando a linha passo a passo até chegar no destino
    while (1)
    {
        // Laços para criar a espessura da linha (desenha um pequeno quadrado ao redor do ponto)
        for (int i = -half; i <= half; i++)
        {
            for (int j = -half; j <= half; j++)
            {
                int xi = x0 + i, yj = y0 + j;
                // Verifica se a coordenada não está fora dos limites da imagem
                if (xi >= 0 && xi < width && yj >= 0 && yj < height)
                {
                    // Mistura a cor da linha com a cor que já estava na imagem (Alpha Blending/Opacidade)
                    img[yj][xi].r = (unsigned char)(img[yj][xi].r * (1.0f - opacity) + color.r * opacity);
                    img[yj][xi].g = (unsigned char)(img[yj][xi].g * (1.0f - opacity) + color.g * opacity);
                    img[yj][xi].b = (unsigned char)(img[yj][xi].b * (1.0f - opacity) + color.b * opacity);
                }
            }
        }
        // Se chegou no ponto final, para o loop
        if (x0 == x1 && y0 == y1)
            break;
        // Cálculos matemáticos do Bresenham para decidir se o próximo passo é em X ou em Y
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

// Função para desenhar um quadrado/retângulo (usada para marcar a área clonada)
void desenhar_retangulo(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float opacidade)
{
    // Desenha as 4 bordas do retângulo usando a função draw_line
    draw_line(largura, altura, matriz, x0, y0, x1, y0, cor, espessura, opacidade); // Topo
    draw_line(largura, altura, matriz, x0, y1, x1, y1, cor, espessura, opacidade); // Fundo
    draw_line(largura, altura, matriz, x0, y0, x0, y1, cor, espessura, opacidade); // Esquerda
    draw_line(largura, altura, matriz, x1, y0, x1, y1, cor, espessura, opacidade); // Direita
}

// Versão alternativa de desenhar linha (funciona somando luz à imagem base em vez de misturar tintas)
void draw_line_transparente(int largura, int altura, Pixel matriz[][largura], int x0, int y0, int x1, int y1, Pixel cor, int espessura, float intensidade)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2, e2;
    int metade_espessura = espessura / 2;

    while (1)
    {
        for (int i = -metade_espessura; i <= metade_espessura; i++)
        {
            for (int j = -metade_espessura; j <= metade_espessura; j++)
            {
                int xi = x0 + i, yj = y0 + j;
                if (xi >= 0 && xi < largura && yj >= 0 && yj < altura)
                {
                    // Em vez de mesclar, ele SOMA o valor RGB. Isso cria um efeito de "luz brilhante"
                    int r = matriz[yj][xi].r + (int)(cor.r * intensidade);
                    int g = matriz[yj][xi].g + (int)(cor.g * intensidade);
                    int b = matriz[yj][xi].b + (int)(cor.b * intensidade);

                    // Garante que o valor da cor não estoure o limite máximo de 255
                    matriz[yj][xi].r = r > 255 ? 255 : (unsigned char)r;
                    matriz[yj][xi].g = g > 255 ? 255 : (unsigned char)g;
                    matriz[yj][xi].b = b > 255 ? 255 : (unsigned char)b;
                }
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

// Transforma RGB em "brilho/luminância" (como se fosse preto e branco)
float calcular_luminancia(Pixel p)
{
    // Pesos padrão da percepção visual humana para as cores verde, vermelho e azul
    return 0.59f * p.g + 0.30f * p.r + 0.11f * p.b;
}

// Calcula o quão parecidos dois blocos de imagem são
float comparar_blocos(Pixel bloco1[], Pixel bloco2[], int tamanho)
{
    float soma = 0.0f;
    // Varre todos os pixels dos dois blocos
    for (int i = 0; i < tamanho; i++)
    {
        // Calcula a diferença de cor em cada canal (Erro matemático)
        float dif_r = bloco1[i].r - bloco2[i].r;
        float dif_g = bloco1[i].g - bloco2[i].g;
        float dif_b = bloco1[i].b - bloco2[i].b;
        // Eleva ao quadrado e soma (Erro Quadrático Médio)
        soma += dif_r * dif_r + dif_g * dif_g + dif_b * dif_b;
    }
    // Retorna a média da diferença. Quanto MENOR o retorno, mais IGUAIS são os blocos.
    return soma / (tamanho * 3);
}

// Calcula a variância da textura de um bloco
// Áreas lisas, exemplo céu, têm variância baixa
float calcular_variancia(Pixel bloco[], int tamanho)
{
    float media_r = 0;
    float media_g = 0;
    float media_b = 0;

    // Calcula cor média do bloco
    for (int i = 0; i < tamanho; i++)
    {
        media_r += bloco[i].r;
        media_g += bloco[i].g;
        media_b += bloco[i].b;
    }
    media_r /= tamanho;
    media_g /= tamanho;
    media_b /= tamanho;

    // Calcula a variância (fórmula) do bloco
    float variancia = 0;

    for (int i = 0; i < tamanho; i++)
    {
        float dif_r = bloco[i].r - media_r;
        float dif_g = bloco[i].g - media_g;
        float dif_b = bloco[i].b - media_b;

        variancia += dif_r * dif_r + dif_g * dif_g + dif_b * dif_b;
    }

    return variancia / (tamanho * 3);
}

// Metodo para ordenar quick sort
int comapara_float(const void *a, const void *b)
{
    float fa = *(float *)a;
    float fb = *(float *)b;

    return (fa > fb) - (fa < fb);
}

// Descobre qual a quantidade mínima de variação de textura que um bloco deve ter para ser analisado
float variacao_textura(int largura, int altura, Pixel matriz_entrada[][largura], int tamanho_bloco)
{

    int qtde_blocos_largura = largura / tamanho_bloco;
    int qtde_blocos_altura = altura / tamanho_bloco;
    int qtde_total_blocos = qtde_blocos_largura * qtde_blocos_altura;

    // Aloca um vetor para guardar a variância de todos os blocos
    float *variancias = malloc(qtde_total_blocos * sizeof(float));

    int idx_variancia = 0;
    int x_inicial;
    int y_inicial;

    // Divide a imagem em blocos
    for (int altura = 0; altura < qtde_blocos_altura; altura++)
    {
        for (int largura = 0; largura < qtde_blocos_largura; largura++)
        {
            // Calcula a coordenada X e Y inicial do bloco atual
            x_inicial = largura * tamanho_bloco;
            y_inicial = altura * tamanho_bloco;

            // Vetor para guardar os pixels do bloco
            Pixel bloco[tamanho_bloco * tamanho_bloco];

            int idx_bloco = 0;

            // Armazena os pixels da imagem para o vetor
            for (int coord_y = 0; coord_y < tamanho_bloco; coord_y++)
            {
                for (int coord_x = 0; coord_x < tamanho_bloco; coord_x++)
                {
                    // Garante que não irá pegar pixel fora da imagem
                    if (y_inicial + coord_y < altura && x_inicial + coord_x < largura)
                    {
                        bloco[idx_bloco++] = matriz_entrada[y_inicial + coord_y][x_inicial + coord_x];
                    }
                }
            }
            // Salva a variância calculada deste bloco
            variancias[idx_variancia++] = calcular_variancia(bloco, idx_bloco);
        }
    }

    // Ordena o vetor de variâncias (do bloco mais liso para o bloco com mais textura)
    qsort(variancias, idx_variancia, sizeof(float), comapara_float);

    // Queremos a variancia acima de 40% em relação a lista
    float var_40 = variancias[(int)(idx_variancia * 0.40f)];

    float variancia_minima; 

    if (var_40 > 100.0f)
    {
        // Se a variancia minima for MAIOR que 100, usamos ela mesma
        variancia_minima = var_40;
    }
    else
    {
        // Se for MENOR ou IGUAL a 100, forçamos o valor a ser 100.0
        variancia_minima = 100.0f;
    }

    free(variancias);

    return variancia_minima; // Retorna o valor de corte
}


void detecta_clone(int largura, int altura, Pixel matriz_entrada[][largura], Pixel matriz_saida[][largura])
{

    int tamanho_bloco = 24;
    float tolerancia_copia = 250.0f; // Máximo de diferença aceita para considerar "cópia"
    int ditancia_minima = tamanho_bloco * 2;

    // Quantidade de blocos horizontais e verticais
    int num_blocos_x = largura / tamanho_bloco;
    int num_blocos_y = altura / tamanho_bloco;

    // Copia a imagem de entrada para saída
    for (int i = 0; i < altura; i++)
        for (int j = 0; j < largura; j++)
            matriz_saida[i][j] = matriz_entrada[i][j];

    // Calcula o limite mínimo de variância para ignorar áreas lisas
    float variancia_minima = variacao_textura(largura, altura, matriz_entrada, tamanho_bloco);

    // Vetor, para marcar quais blocos já encontramos como clones,
    // para não testar duas vezes
    char *blocos_verificados = (char *)calloc(num_blocos_x * num_blocos_y, sizeof(char));

    // Lista vazia para guardar até 5000 cópias encontradas
    Similar Similares[5000];
    int total_Similares = 0;

    // --- ETAPA 1: ENCONTRAR SimilarES (CÓPIAS) ---
    // Loop principal: pega o Bloco 1
    for (int by1 = 0; by1 < num_blocos_y; by1++)
    {
        for (int bx1 = 0; bx1 < num_blocos_x; bx1++)
        {
            int indice1 = by1 * num_blocos_x + bx1;

            // Se esse bloco já foi marcado como clone antes, pula ele
            if (blocos_verificados[indice1])
                continue;

            // Extrai as informações e pixels do Bloco 1
            int x1_inicio = bx1 * tamanho_bloco, y1_inicio = by1 * tamanho_bloco;
            Pixel bloco1[tamanho_bloco * tamanho_bloco];
            int tam1 = 0;
            for (int i = 0; i < tamanho_bloco; i++)
                for (int j = 0; j < tamanho_bloco; j++)
                    if (y1_inicio + i < altura && x1_inicio + j < largura)
                        bloco1[tam1++] = matriz_entrada[y1_inicio + i][x1_inicio + j];

            // Se o Bloco 1 for liso demais (variância abaixo do limite), ignora ele
            if (tam1 == 0 || calcular_variancia(bloco1, tam1) < variancia_minima)
                continue;

            int centro_x1 = x1_inicio + tamanho_bloco / 2, centro_y1 = y1_inicio + tamanho_bloco / 2;

            // Variáveis para guardar qual foi o Bloco 2 mais idêntico encontrado
            float melhor_similaridade = 1e30f; // Começa com um valor infinito
            int melhor_bx2 = -1, melhor_by2 = -1;

            // Loop Secundário: Varre a imagem inteira de novo buscando o Bloco 2 para comparar
            for (int by2 = 0; by2 < num_blocos_y; by2++)
            {
                for (int bx2 = 0; bx2 < num_blocos_x; bx2++)
                {
                    int indice2 = by2 * num_blocos_x + bx2;
                    // Ignora se for o próprio Bloco 1 ou se o Bloco 2 já estiver verificado
                    if (indice2 == indice1 || blocos_verificados[indice2])
                        continue;

                    int x2_inicio = bx2 * tamanho_bloco, y2_inicio = by2 * tamanho_bloco;
                    int centro_x2 = x2_inicio + tamanho_bloco / 2, centro_y2 = y2_inicio + tamanho_bloco / 2;

                    // Calcula a distância física entre os blocos (Teorema de Pitágoras sem raiz)
                    int delta_x = centro_x1 - centro_x2, delta_y = centro_y1 - centro_y2;
                    // Se o Bloco 2 estiver muito perto fisicamente do Bloco 1, ignora
                    if (delta_x * delta_x + delta_y * delta_y < ditancia_minima * ditancia_minima)
                        continue;

                    // Extrai os pixels do Bloco 2
                    Pixel bloco2[tamanho_bloco * tamanho_bloco];
                    int tam2 = 0;
                    for (int i = 0; i < tamanho_bloco; i++)
                        for (int j = 0; j < tamanho_bloco; j++)
                            if (y2_inicio + i < altura && x2_inicio + j < largura)
                                bloco2[tam2++] = matriz_entrada[y2_inicio + i][x2_inicio + j];

                    // Se o Bloco 2 também for liso demais, ignora
                    if (tam2 == 0 || calcular_variancia(bloco2, tam2) < variancia_minima)
                        continue;

                    // Compara o Bloco 1 com o Bloco 2
                    int t_min = tam1 < tam2 ? tam1 : tam2;
                    float similaridade = comparar_blocos(bloco1, bloco2, t_min);

                    // Se a pontuação for melhor (menor diferença) do que a gravada, atualiza o recorde
                    if (similaridade < melhor_similaridade)
                    {
                        melhor_similaridade = similaridade;
                        melhor_bx2 = bx2;
                        melhor_by2 = by2;
                    }
                }
            }

            // Após varrer toda a imagem, verifica se o Bloco mais parecido encontrado
            // possui uma diferença menor do que o nosso limite aceitável de "cópia"
            if (melhor_bx2 >= 0 && melhor_similaridade < tolerancia_copia)
            {
                // Marca ambos os blocos como processados para economizar tempo futuro
                blocos_verificados[indice1] = 1;
                blocos_verificados[melhor_by2 * num_blocos_x + melhor_bx2] = 1;

                // Se houver espaço no nosso array de Similares, salva as evidências
                if (total_Similares < 5000)
                {
                    Similares[total_Similares].x1 = x1_inicio;
                    Similares[total_Similares].y1 = y1_inicio;
                    Similares[total_Similares].x2 = melhor_bx2 * tamanho_bloco;
                    Similares[total_Similares].y2 = melhor_by2 * tamanho_bloco;

                    // Guarda também a distância e direção exata do movimento do clone
                    Similares[total_Similares].dx = Similares[total_Similares].x2 - Similares[total_Similares].x1;
                    Similares[total_Similares].dy = Similares[total_Similares].y2 - Similares[total_Similares].y1;
                    Similares[total_Similares].valido = 0; // Inicia como falso até a etapa de filtragem
                    total_Similares++;                   // Aumenta o contador
                }
            }
        }
    }

    // --- ETAPA 2: FILTRAGEM ESPACIAL (ELIMINA FALSOS POSITIVOS) ---
    int min_tamanho_cluster = 3; // Um clone só é real se pelo menos 3 blocos foram movidos juntos

    // Varre todos os Similares encontrados
    for (int i = 0; i < total_Similares; i++)
    {
        int count_similares = 0;
        // Compara com todos os outros Similares
        for (int j = 0; j < total_Similares; j++)
        {
            // Se o Similar 'j' andou praticamente na mesma direção e distância (dx, dy) que o Similar 'i'
            if (abs(Similares[i].dx - Similares[j].dx) <= tamanho_bloco &&
                abs(Similares[i].dy - Similares[j].dy) <= tamanho_bloco)
            {
                count_similares++; // Soma no rebanho
            }
        }
        // Se formou um rebanho/cluster maior ou igual a 3, esse Similar é considerado real
        if (count_similares >= min_tamanho_cluster)
        {
            Similares[i].valido = 1;
        }
    }

    // --- ETAPA 3: DESENHAR RESULTADOS NA IMAGEM DE SAÍDA ---
    Pixel cor_roxa = {180, 50, 255}; // Define a cor para marcação

    // Varre todos os Similares
    for (int i = 0; i < total_Similares; i++)
    {
        // Apenas desenha os que passaram pelo filtro de agrupamento da Etapa 2
        if (Similares[i].valido)
        {
            // Calcula o final dos quadrados
            int x1_fim = Similares[i].x1 + tamanho_bloco - 1;
            int y1_fim = Similares[i].y1 + tamanho_bloco - 1;
            int x2_fim = Similares[i].x2 + tamanho_bloco - 1;
            int y2_fim = Similares[i].y2 + tamanho_bloco - 1;

            // Desenha um retângulo roxo na área que foi copiada (origem)
            desenhar_retangulo(largura, altura, matriz_saida, Similares[i].x1, Similares[i].y1, x1_fim, y1_fim, cor_roxa, 2, 0.8f);
            // Desenha um retângulo roxo na área que foi colada (destino)
            desenhar_retangulo(largura, altura, matriz_saida, Similares[i].x2, Similares[i].y2, x2_fim, y2_fim, cor_roxa, 2, 0.8f);

            // Calcula o centro dos dois blocos
            int centro_x1 = Similares[i].x1 + tamanho_bloco / 2;
            int centro_y1 = Similares[i].y1 + tamanho_bloco / 2;
            int centro_x2 = Similares[i].x2 + tamanho_bloco / 2;
            int centro_y2 = Similares[i].y2 + tamanho_bloco / 2;

            // Traça a linha que liga a origem ao destino, evidenciando o movimento da fraude
            draw_line_transparente(largura, altura, matriz_saida, centro_x1, centro_y1, centro_x2, centro_y2, cor_roxa, 2, 0.4f);
        }
    }

    // Libera a memória da flag que criamos no início desta função
    free(blocos_verificados);
}