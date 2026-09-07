#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include"../includes/preprocessador.h"

void remover_comentario(const char * entrada, char * saida) {

    int dentro_str = 0;
    int i = 0;
    int j = 0;
    int fechamento_aspas = 0;

    while (entrada[i] != '\0') {

        char c = entrada[i];

        if (c == '"') {   
            dentro_str = !dentro_str;
            saida[j++] = c; 
        }

        // Aqui o código altera oo valor da variável dentro_string (que vale 0 que representa falso), para 1(verdadeiro), e checa depois.
        else if (c == '#' && !dentro_str) { 
            while (entrada[i] != '\n' && entrada[i] != '\0') i++;       
        }
        else if (c == '#' && dentro_str) {            
                int tem_fechamento_na_linha = 0;

                 // Olha para a frente (sem mover o i) procurando aspa antes do \n
                for (int k = i + 1; entrada[k] != '\n' && entrada[k] != '\0'; k++) {
                    if (entrada[k] == '"') {
                        tem_fechamento_na_linha = 1;
                        break;
                    }
                }

        // Se tem aspa fechando na mesma linha, o '#' faz parte da string
        if (tem_fechamento_na_linha) {
            saida[j++] = c;
        } 
        // Se nao fecha na mesma linha, a string é aberta e o # vira comentario
        else {
            while (entrada[i] != '\n' && entrada[i] != '\0') {
                i++;
            }
        }
        }                 
        else saida[j++] = c;      
        if (entrada[i] != '\0') i++;
        
        saida[j] = '\0';       
    }

    
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