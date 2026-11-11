/*  Compilação do projeto:
        gcc -std=c99 -src/main.c src/preprocessador.c src/lexico.c -Iinclude -o main.exe
    Exemplo de Exceução (dois argumentos):
        ./main.exe tests\entradas\correto1.asm tests\saidas\correto1.le
    Exemplo de Execução (três argumentos):
        ./main.exe tests\entradas\correto1.asm tests\saidas\correto1.pre tests\saidas\correto1.lex */

#include <stdio.h>
#include <string.h>

#include "preprocessador.h"
#include "lexico.h"

#define TAM_NOME_ARQUIVO 1024

/*  Monta um nome de arquivo trocando a extensao de "base" por "extensao". Ex.: ("tests/saidas/teste.lex", ".ts") -> "tests/saidas/teste.ts".
    So considera como extensao o ultimo ponto depois da ultima barra. */
static int trocar_extensao(const char *base, const char *extensao, char *destino, size_t tamanho) {
    const char *ponto = strrchr(base, '.');
    const char *barra = strrchr(base, '/');
    const char *contrabarra = strrchr(base, '\\');
    size_t tamanho_base;

    if (contrabarra != NULL && (barra == NULL || contrabarra > barra)) barra = contrabarra;
    if (ponto != NULL && barra != NULL && ponto < barra) ponto = NULL;

    tamanho_base = ponto == NULL ? strlen(base) : (size_t)(ponto - base);
    if (tamanho_base + strlen(extensao) + 1 > tamanho) return 0;

    memcpy(destino, base, tamanho_base);
    strcpy(destino + tamanho_base, extensao);
    return 1;
}

/*  Os arquivos sao abertos em modo binario para que a saida use sempre '\n' como quebra de linha, no Windows e no Linux.
    As quebras \r\n e \r da entrada sao tratadas pelo proprio pre-processador. */
static FILE *abrir(const char *nome, const char *modo) {
    FILE *arquivo = fopen(nome, modo);
    if (arquivo == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s' para %s.\n", nome, modo[0] == 'r' ? "leitura" : "gravacao");
    }
    return arquivo;
}

// Fecha o arquivo e informa se houve erro de gravacao ou de fechamento. Devolve 1 se deu tudo certo.
static int fechar(FILE *arquivo, const char *nome) {
    int ok = 1;
    if (arquivo == NULL) return 1;
    if (ferror(arquivo)) {
        fprintf(stderr, "Erro: falha de leitura ou gravacao no arquivo '%s'.\n", nome);
        ok = 0;
    }
    if (fclose(arquivo) != 0) {
        fprintf(stderr, "Erro: nao foi possivel fechar o arquivo '%s'.\n", nome);
        ok = 0;
    }
    return ok;
}

static void uso(const char *programa) {
    fprintf(stderr,
            "Uso: %s <entrada.asm> <saida.lex>\n"
            "  ou %s <entrada.asm> <saida.pre> <saida.lex>\n"
            "Com dois argumentos, o arquivo .pre recebe o nome do .lex com a\n"
            "extensao trocada. Os arquivos .ts e .err sao criados ao lado do .lex.\n",
            programa, programa);
}

int main(int argc, char *argv[]) {
    const char *nome_asm;
    const char *nome_lex;

    char nome_pre[TAM_NOME_ARQUIVO];
    char nome_ts[TAM_NOME_ARQUIVO];
    char nome_err[TAM_NOME_ARQUIVO];

    FILE *in;
    FILE *out;
    FILE *ts;
    FILE *err;

    int ok;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Erro: quantidade de argumentos invalida.\n");
        uso(argv[0]);
        return 1;
    }

    nome_asm = argv[1];
    nome_lex = argv[argc - 1];

    if (argc == 4) {
        if (strlen(argv[2]) >= sizeof(nome_pre)) {
            fprintf(stderr, "Erro: nome do arquivo .pre muito longo.\n");
            return 1;
        }
        strcpy(nome_pre, argv[2]);
    } else if (!trocar_extensao(nome_lex, ".pre", nome_pre, sizeof(nome_pre))) {
        fprintf(stderr, "Erro: nome do arquivo .lex muito longo.\n");
        return 1;
    }

    if (!trocar_extensao(nome_lex, ".ts", nome_ts, sizeof(nome_ts)) ||
        !trocar_extensao(nome_lex, ".err", nome_err, sizeof(nome_err))) {
        fprintf(stderr, "Erro: nome do arquivo .lex muito longo.\n");
        return 1;
    }

    if (strcmp(nome_asm, nome_pre) == 0 || strcmp(nome_asm, nome_lex) == 0) {
        fprintf(stderr, "Erro: o arquivo de entrada nao pode ser tambem um arquivo de saida.\n");
        return 1;
    }

    // Etapa 1: pre-processamento (.asm -> .pre)
    in = abrir(nome_asm, "rb");
    if (in == NULL) return 1;
    out = abrir(nome_pre, "wb");
    if (out == NULL) {
        fclose(in);
        return 1;
    }

    PreProcessamento(in, out);

    ok = fechar(in, nome_asm);
    ok = fechar(out, nome_pre) && ok;
    if (!ok) return 1;

    // Etapa 2: analise lexica (.pre -> .lex, .ts, .err)
    in = abrir(nome_pre, "rb");
    out = abrir(nome_lex, "wb");
    ts = abrir(nome_ts, "wb");
    err = abrir(nome_err, "wb");
    if (in == NULL || out == NULL || ts == NULL || err == NULL) {
        if (in != NULL) fclose(in);
        if (out != NULL) fclose(out);
        if (ts != NULL) fclose(ts);
        if (err != NULL) fclose(err);
        return 1;
    }

    DefinirSaidasLexicas(ts, err);
    AnaliseLexica(in, out);

    ok = fechar(in, nome_pre);
    ok = fechar(out, nome_lex) && ok;
    ok = fechar(ts, nome_ts) && ok;
    ok = fechar(err, nome_err) && ok;
    if (!ok) return 1;

    printf("Pre-processamento:  %s\n", nome_pre);
    printf("Tokens:             %s\n", nome_lex);
    printf("Tabela de simbolos: %s\n", nome_ts);
    printf("Erros lexicos:      %s (%d erro(s))\n", nome_err, QuantidadeErrosLexicos());
    return 0;
}