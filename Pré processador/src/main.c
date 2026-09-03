#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"preprocessador.h"

int main(int argc, char *argv[]) {

    FILE * arquivo_entrada;
    FILE * arquivo_saida;

    /* Verifica se foram informados exatamente dois argumentos */
    if (argc != 3) {

        fprintf(stderr,
                "Uso: %s <arquivo_entrada.asm> <arquivo_saida.pre>\n",
                argv[0]);

        return 1;
    }

    /* Abre o arquivo de entrada para leitura */
    arquivo_entrada = fopen(argv[1], "r");

    if (arquivo_entrada == NULL) {

        fprintf(stderr,
                "Erro: nao foi possivel abrir o arquivo de entrada '%s'.\n",
                argv[1]);

        return 1;
    }

    /* Abre o arquivo de saida para escrita */
    arquivo_saida = fopen(argv[2], "w");

    if (arquivo_saida == NULL) {
        
        fprintf(stderr,
                "Erro: nao foi possivel criar o arquivo de saida '%s'.\n",
                argv[2]);

        fclose(arquivo_entrada);

        return 1;
    }

    /* Executa o pre-processamento */
    preprocessar(arquivo_entrada, arquivo_saida);

    /* Fecha os arquivos */
    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    return 0;
}