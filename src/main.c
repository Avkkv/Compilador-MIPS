#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "preprocessador.h"
#include "lexico.h"

static int nome_relatorio(const char *nome_lex, const char *extensao,
                          char *destino, size_t tamanho)
{
    const char *ponto = strrchr(nome_lex, '.');
    size_t base = ponto == NULL ? strlen(nome_lex) : (size_t)(ponto - nome_lex);
    if (base + strlen(extensao) + 1 > tamanho) return 0;
    memcpy(destino, nome_lex, base);
    destino[base] = '\0';
    return snprintf(destino + base, tamanho - base, "%s", extensao) >= 0;
}

int main(int argc, char *argv[]) {
    FILE *arquivo_entrada;
    FILE *arquivo_saida;
    FILE *arquivo_lex;
    FILE *arquivo_ts;
    FILE *arquivo_err;
    char nome_ts[1024];
    char nome_err[1024];

    if (argc != 4) {
        fprintf(stderr,
                "Uso: %s <entrada.asm> <saida.pre> <saida.lex>\n",
                argv[0]);
        return 1;
    }

    arquivo_entrada = fopen(argv[1], "r");
    if (arquivo_entrada == NULL) {
        fprintf(stderr,
                "Erro: nao foi possivel abrir o arquivo de entrada '%s'.\n",
                argv[1]);
        return 1;
    }

    arquivo_saida = fopen(argv[2], "w");
    if (arquivo_saida == NULL) {
        fprintf(stderr,
                "Erro: nao foi possivel criar o arquivo de saida '%s'.\n",
                argv[2]);
        fclose(arquivo_entrada);
        return 1;
    }

    preprocessar(arquivo_entrada, arquivo_saida);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    arquivo_saida = fopen(argv[2], "r");
    arquivo_lex = fopen(argv[3], "w");
    if (arquivo_saida == NULL || arquivo_lex == NULL ||
        !nome_relatorio(argv[3], ".ts", nome_ts, sizeof(nome_ts)) ||
        !nome_relatorio(argv[3], ".err", nome_err, sizeof(nome_err))) {
        fprintf(stderr, "Erro: nao foi possivel preparar as saidas lexicais.\n");
        if (arquivo_saida != NULL) fclose(arquivo_saida);
        if (arquivo_lex != NULL) fclose(arquivo_lex);
        return 1;
    }

    arquivo_ts = fopen(nome_ts, "w");
    arquivo_err = fopen(nome_err, "w");
    if (arquivo_ts == NULL || arquivo_err == NULL) {
        fprintf(stderr, "Erro: nao foi possivel criar os relatorios lexicais.\n");
        fclose(arquivo_saida);
        fclose(arquivo_lex);
        if (arquivo_ts != NULL) fclose(arquivo_ts);
        if (arquivo_err != NULL) fclose(arquivo_err);
        return 1;
    }

    AnaliseLexicaComSaidas(arquivo_saida, arquivo_lex, arquivo_ts, arquivo_err);
    if (ftell(arquivo_err) == 0) {
        fprintf(arquivo_err, "Nenhum erro lexico encontrado.\n");
    }

    fclose(arquivo_saida);
    fclose(arquivo_lex);
    fclose(arquivo_ts);
    fclose(arquivo_err);

    return 0;
}
