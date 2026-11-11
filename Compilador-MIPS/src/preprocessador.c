#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "preprocessador.h"

static void processar_linha_final(const char *linha, FILE *saida, int com_quebra);

/*  Verifica se a string que comeca em entrada[inicio] (logo depois da aspa de abertura) e fechada na mesma linha.
    Uma aspa precedida de barra invertida (\") faz parte da string e nao a fecha. */
static int string_fecha_na_linha(const char *entrada, size_t inicio) {
    size_t i = inicio;
    while (entrada[i] != '\0') {
        if (entrada[i] == '\\') {
            if (entrada[i + 1] == '\0') return 0;
            i += 2;
            continue;
        }
        if (entrada[i] == '"') return 1;
        i++;
    }
    return 0;
}

/*  Remove o comentario da linha. O # so inicia comentario fora de strings.
    Se a string nao for fechada na linha, um # dentro dela e tratado como inicio de comentario,
    para que o analisador lexico acuse a string nao fechada sem carregar o texto do comentario. */
void remover_comentario(const char *entrada, char *saida) {
    size_t i = 0;
    size_t j = 0;
    int dentro_str = 0;
    int str_fechada = 0;

    while (entrada[i] != '\0') {
        char c = entrada[i];

        if (dentro_str) {
            if (c == '\\' && entrada[i + 1] != '\0') {
                saida[j++] = c;
                saida[j++] = entrada[i + 1];
                i += 2;
                continue;
            }
            if (c == '#' && !str_fechada) break;
            if (c == '"') dentro_str = 0;
            saida[j++] = c;
            i++;
            continue;
        }

        if (c == '#') break;
        if (c == '"') {
            dentro_str = 1;
            str_fechada = string_fecha_na_linha(entrada, i + 1);
        }
        saida[j++] = c;
        i++;
    }
    saida[j] = '\0';
}

// Troca tabulacoes por espaco, reduz espacos repetidos a um so e remove os espacos do inicio e do fim. O interior das strings nao e alterado.
void normalizar_espacos(const char *entrada, char *saida) {
    size_t i = 0;
    size_t j = 0;
    int dentro_str = 0;

    while (entrada[i] != '\0') {
        char c = entrada[i];

        if (dentro_str) {
            saida[j++] = c;
            if (c == '\\' && entrada[i + 1] != '\0') {
                saida[j++] = entrada[i + 1];
                i += 2;
                continue;
            }
            if (c == '"') dentro_str = 0;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
            if (j > 0 && saida[j - 1] != ' ') saida[j++] = ' ';
        } else if (c != '\r' && c != '\n') {
            if (c == '"') dentro_str = 1;
            saida[j++] = c;
        }
        i++;
    }

    while (j > 0 && saida[j - 1] == ' ') j--;
    saida[j] = '\0';
}

// Remove o comentario, normaliza os espacos e grava a linha se sobrar algum conteudo, seguida de '\n'.
void processar_linha(const char *linha, FILE *saida) {
    processar_linha_final(linha, saida, 1);
}

/*  Igual a processar_linha, mas so grava o '\n' se "com_quebra" for 1.
    Usada na ultima linha do arquivo quando ela nao termina em quebra de linha:
    assim uma string aberta nessa linha chega ao analisador lexico como "nao fechada antes do fim do arquivo". */
static void processar_linha_final(const char *linha, FILE *saida, int com_quebra) {
    size_t tamanho = strlen(linha) + 1;
    char *sem_comentario = malloc(tamanho);
    char *normalizada = malloc(tamanho);

    if (sem_comentario == NULL || normalizada == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
        free(sem_comentario);
        free(normalizada);
        return;
    }

    remover_comentario(linha, sem_comentario);
    normalizar_espacos(sem_comentario, normalizada);

    if (normalizada[0] != '\0') {
        fputs(normalizada, saida);
        if (com_quebra) fputc('\n', saida);
    }

    free(sem_comentario);
    free(normalizada);
}

/*  Le a entrada caractere por caractere, montando uma linha de cada vez.
    Aceita \n (Linux), \r\n (Windows) e \r (Mac antigo) como fim de linha. O buffer cresce conforme a necessidade, entao linhas longas nao sao cortadas. */
void PreProcessamento(FILE *in, FILE *out) {
    size_t capacidade = TAM_LINHA_INICIAL;
    size_t tamanho = 0;
    char *linha = malloc(capacidade);
    int c;

    if (linha == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
        return;
    }

    while ((c = fgetc(in)) != EOF) {
        if (c == '\r' || c == '\n') {
            if (c == '\r') {
                int proximo = fgetc(in);
                if (proximo != '\n' && proximo != EOF) ungetc(proximo, in);
            }
            linha[tamanho] = '\0';
            processar_linha(linha, out);
            tamanho = 0;
            continue;
        }

        if (tamanho + 1 >= capacidade) {
            char *maior = realloc(linha, capacidade * 2);
            if (maior == NULL) {
                fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
                free(linha);
                return;
            }
            linha = maior;
            capacidade *= 2;
        }
        linha[tamanho++] = (char)c;
    }

    if (tamanho > 0) {
        linha[tamanho] = '\0';
        processar_linha_final(linha, out, 0);
    }

    free(linha);
}