#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "preprocessador.h"

void remover_comentario(const char * entrada, char * saida) {

    int dentro_str = 0;
    int i = 0;
    int j = 0;

    while (entrada[i] != '\0') {
        char c = entrada[i];

        if (c == '"') {
            dentro_str = !dentro_str;
            saida[j++] = c;
        }
        else if (c == '#' && !dentro_str) {
            break;
        }
        else {
            saida[j++] = c;
        }

        i++;
    }

    saida[j] = '\0';
}

void normalizar_espacos(const char * entrada, char * saida) {

    int dentro_str = 0;
    int i = 0;
    int j = 0;
    int tem_conteudo = 0;

    while (entrada[i] != '\0') {

        char c = entrada[i];

        if (c == '"') {
            dentro_str = !dentro_str;
            saida[j++] = c;
            tem_conteudo = 1;
        }
        else if (dentro_str) {
            saida[j++] = c;
            tem_conteudo = 1;
        }
        else if (c == ' ' || c == '\t') {
            if (tem_conteudo && j > 0 && saida[j - 1] != ' ') {
                saida[j++] = ' ';
            }
        }
        else if (c != '\r' && c != '\n') {
            saida[j++] = c;
            tem_conteudo = 1;
        }

        i++;
    }

    while (j > 0 && saida[j - 1] == ' ') j--;
   
    saida[j] = '\0';
}

void processar_linha(const char *linha, FILE *saida) {

    char sem_comentario[TAM_LINHA];
    char normalizada[TAM_LINHA];

    remover_comentario(linha, sem_comentario);
    normalizar_espacos(sem_comentario, normalizada);

    if (normalizada[0] != '\0') {
        fprintf(saida, "%s\n", normalizada);
    }
}

void preprocessar(FILE * entrada, FILE * saida)
{
    char linha[TAM_LINHA];

    while (fgets(linha, TAM_LINHA, entrada) != NULL) {
        processar_linha(linha, saida);
    }
}