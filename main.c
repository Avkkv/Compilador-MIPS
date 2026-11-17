/*  Compilacao do projeto:
        gcc main.c preprocessador.c lexico.c sintatico.c -o main.exe
    Execucao:
        ./main.exe <arquivo_entrada.asm> <prefixo_saida>
    Exemplo:
        ./main.exe teste.asm resultado
    Arquivos gerados: resultado.pre, resultado.lex, resultado.ts, resultado.lex.err,
                      resultado.sin.err e resultado.dot

    Codigos de encerramento:
        0  execucao concluida e programa sintaticamente valido;
        1  rejeicao por erros lexicos ou sintaticos;
        2  falha operacional ou uso incorreto da linha de comando. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "preprocessador.h"
#include "lexico.h"
#include "sintatico.h"

#define TAM_NOME_ARQUIVO 1024

#define SAIDA_VALIDO 0
#define SAIDA_REJEITADO 1
#define SAIDA_FALHA 2

/* Monta "prefixo + extensao" em destino. Devolve 0 se nao couber. */
static int montar_nome(const char *prefixo, const char *extensao, char *destino, size_t tamanho) {
    if (strlen(prefixo) + strlen(extensao) + 1 > tamanho) return 0;
    strcpy(destino, prefixo);
    strcat(destino, extensao);
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

/* Fecha o arquivo e informa se houve erro de gravacao ou de fechamento. Devolve 1 se deu tudo certo. */
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

/* Cria (ou esvazia) um arquivo de saida. Usado para que resultados de uma execucao anterior nao fiquem para tras. */
static int esvaziar(const char *nome) {
    FILE *f = abrir(nome, "wb");
    if (f == NULL) return 0;
    return fechar(f, nome);
}

/* Grava uma unica mensagem em um arquivo, substituindo o conteudo anterior. */
static int gravar_mensagem(const char *nome, const char *mensagem) {
    FILE *f = abrir(nome, "wb");
    if (f == NULL) return 0;
    fputs(mensagem, f);
    return fechar(f, nome);
}

#ifdef _WIN32
/*  As mensagens do terminal (ex.: "PROGRAMA SINTATICAMENTE VÁLIDO") estao em UTF-8. O console do Windows usa outra
    pagina de codigo por padrao e mostraria "VÃLIDO"; por isso a saida do console e trocada para UTF-8 durante a
    execucao, e a pagina original e restaurada ao terminar. Em outros sistemas nada precisa ser feito. */
static UINT pagina_original = 0;

static void restaurar_console(void) {
    if (pagina_original != 0) SetConsoleOutputCP(pagina_original);
}

static void preparar_console(void) {
    pagina_original = GetConsoleOutputCP();
    if (pagina_original != 0 && SetConsoleOutputCP(CP_UTF8)) atexit(restaurar_console);
}
#endif

static void uso(const char *programa) {
    fprintf(stderr,
            "Uso: %s <arquivo_entrada.asm> <prefixo_saida>\n"
            "Exemplo: %s teste.asm resultado\n"
            "Gera resultado.pre, resultado.lex, resultado.ts, resultado.lex.err,\n"
            "resultado.sin.err e resultado.dot.\n",
            programa, programa);
}

int main(int argc, char *argv[]) {
    const char *nome_asm;
    const char *prefixo;

    char nome_pre[TAM_NOME_ARQUIVO];
    char nome_lex[TAM_NOME_ARQUIVO];
    char nome_ts[TAM_NOME_ARQUIVO];
    char nome_lex_err[TAM_NOME_ARQUIVO];
    char nome_sin_err[TAM_NOME_ARQUIVO];
    char nome_dot[TAM_NOME_ARQUIVO];
    const char *saidas[6];

    FILE *in;
    FILE *out;
    FILE *ts;
    FILE *err;
    FILE *dot;

    int ok, i, resultado;

#ifdef _WIN32
    preparar_console();
#endif

    if (argc != 3) {
        fprintf(stderr, "Erro: quantidade de argumentos invalida.\n");
        uso(argc > 0 ? argv[0] : "main.exe");
        return SAIDA_FALHA;
    }

    nome_asm = argv[1];
    prefixo = argv[2];
    if (prefixo[0] == '\0') {
        fprintf(stderr, "Erro: o prefixo de saida nao pode ser vazio.\n");
        uso(argv[0]);
        return SAIDA_FALHA;
    }

    if (!montar_nome(prefixo, ".pre", nome_pre, sizeof(nome_pre)) ||
        !montar_nome(prefixo, ".lex", nome_lex, sizeof(nome_lex)) ||
        !montar_nome(prefixo, ".ts", nome_ts, sizeof(nome_ts)) ||
        !montar_nome(prefixo, ".lex.err", nome_lex_err, sizeof(nome_lex_err)) ||
        !montar_nome(prefixo, ".sin.err", nome_sin_err, sizeof(nome_sin_err)) ||
        !montar_nome(prefixo, ".dot", nome_dot, sizeof(nome_dot))) {
        fprintf(stderr, "Erro: prefixo de saida muito longo.\n");
        return SAIDA_FALHA;
    }

    saidas[0] = nome_pre;
    saidas[1] = nome_lex;
    saidas[2] = nome_ts;
    saidas[3] = nome_lex_err;
    saidas[4] = nome_sin_err;
    saidas[5] = nome_dot;
    for (i = 0; i < 6; i++) {
        if (strcmp(nome_asm, saidas[i]) == 0) {
            fprintf(stderr, "Erro: o arquivo de entrada nao pode ser tambem um arquivo de saida.\n");
            return SAIDA_FALHA;
        }
    }

    /* Confere a entrada antes de apagar qualquer saida. */
    in = abrir(nome_asm, "rb");
    if (in == NULL) return SAIDA_FALHA;

    /* Saidas da analise sintatica de uma execucao anterior nao podem parecer atuais. */
    if (!esvaziar(nome_sin_err) || !esvaziar(nome_dot)) {
        fclose(in);
        return SAIDA_FALHA;
    }

    /* Etapa 1: pre-processamento (.asm -> .pre) */
    out = abrir(nome_pre, "wb");
    if (out == NULL) {
        fclose(in);
        return SAIDA_FALHA;
    }

    PreProcessamento(in, out);

    ok = fechar(in, nome_asm);
    ok = fechar(out, nome_pre) && ok;
    if (!ok) return SAIDA_FALHA;

    /* Etapa 2: analise lexica (.pre -> .lex, .ts, .lex.err) */
    in = abrir(nome_pre, "rb");
    out = abrir(nome_lex, "wb");
    ts = abrir(nome_ts, "wb");
    err = abrir(nome_lex_err, "wb");
    if (in == NULL || out == NULL || ts == NULL || err == NULL) {
        if (in != NULL) fclose(in);
        if (out != NULL) fclose(out);
        if (ts != NULL) fclose(ts);
        if (err != NULL) fclose(err);
        return SAIDA_FALHA;
    }

    DefinirSaidasLexicas(ts, err);
    AnaliseLexica(in, out);

    ok = fechar(in, nome_pre);
    ok = fechar(out, nome_lex) && ok;
    ok = fechar(ts, nome_ts) && ok;
    ok = fechar(err, nome_lex_err) && ok;
    if (!ok) return SAIDA_FALHA;


    /* A integracao com o lexico e feita por QuantidadeErrosLexicos(): com erros lexicos a etapa 3 nao roda. */
    if (QuantidadeErrosLexicos() > 0) {
        if (!gravar_mensagem(nome_sin_err, "Análise sintática não executada: existem erros léxicos.\n")) {
            return SAIDA_FALHA;
        }
        printf("ANÁLISE SINTÁTICA NÃO EXECUTADA: EXISTEM ERROS LÉXICOS.\n");
        return SAIDA_REJEITADO;
    }

    /* Etapa 3: analise sintatica (.lex -> .dot, .sin.err) */
    in = abrir(nome_lex, "rb");
    dot = abrir(nome_dot, "wb");
    err = abrir(nome_sin_err, "wb");
    if (in == NULL || dot == NULL || err == NULL) {
        if (in != NULL) fclose(in);
        if (dot != NULL) fclose(dot);
        if (err != NULL) fclose(err);
        return SAIDA_FALHA;
    }

    resultado = AnaliseSintatica(in, dot, err);

    ok = fechar(in, nome_lex);
    ok = fechar(dot, nome_dot) && ok;
    ok = fechar(err, nome_sin_err) && ok;
    if (!ok || resultado == SINTATICO_FALHA) {
        if (resultado == SINTATICO_FALHA) esvaziar(nome_dot);
        fprintf(stderr, "Erro: a analise sintatica nao pode ser concluida.\n");
        return SAIDA_FALHA;
    }


    if (resultado == SINTATICO_VALIDO) {
        printf("PROGRAMA SINTATICAMENTE VÁLIDO\n");
        return SAIDA_VALIDO;
    }
    printf("PROGRAMA SINTATICAMENTE INVÁLIDO\n");
    return SAIDA_REJEITADO;
}
