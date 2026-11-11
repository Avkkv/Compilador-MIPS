/*  Testes unitarios do pre-processador.
    Compilar a partir da raiz do projeto:
    gcc -std=c99 -Wall -Wextra -pedantic -Iinclude tests/test_preprocessador.c src/preprocessador.c -o test_preprocessador.exe */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "preprocessador.h"

static int falhas = 0;
static int total = 0;

static void conferir(const char *nome, const char *obtido, const char *esperado) {
    total++;
    if (strcmp(obtido, esperado) != 0) {
        falhas++;
        printf("FALHOU: %s\n  esperado: [%s]\n  obtido:   [%s]\n", nome, esperado, obtido);
    } else {
        printf("ok      %s\n", nome);
    }
}

static void testar_comentario(const char *nome, const char *entrada, const char *esperado) {
    char saida[512];
    remover_comentario(entrada, saida);
    conferir(nome, saida, esperado);
}

static void testar_espacos(const char *nome, const char *entrada, const char *esperado) {
    char saida[512];
    normalizar_espacos(entrada, saida);
    conferir(nome, saida, esperado);
}

// Passa "entrada" inteira por PreProcessamento usando arquivos temporarios.
static void testar_arquivo(const char *nome, const char *entrada, const char *esperado) {
    FILE *in = tmpfile();
    FILE *out = tmpfile();
    static char saida[16384];
    size_t lidos;

    if (in == NULL || out == NULL) {
        printf("FALHOU: %s (nao foi possivel criar arquivo temporario)\n", nome);
        falhas++;
        total++;
        return;
    }
    fwrite(entrada, 1, strlen(entrada), in);
    rewind(in);
    PreProcessamento(in, out);
    rewind(out);
    lidos = fread(saida, 1, sizeof(saida) - 1, out);
    saida[lidos] = '\0';
    fclose(in);
    fclose(out);
    conferir(nome, saida, esperado);
}

int main(void) {
    char *longa;
    char *longa_esperada;
    int i;

    /* Comentarios */
    testar_comentario("comentario no fim da linha",
                      "add $t0, $t1, $t2 # realiza a soma", "add $t0, $t1, $t2 ");
    testar_comentario("linha so com comentario", "# so comentario", "");
    testar_comentario("# dentro de string e preservado",
                      "msg: .asciiz \"Valor # informado\"", "msg: .asciiz \"Valor # informado\"");
    testar_comentario("# depois de aspa escapada continua na string",
                      "m: .asciiz \"a \\\" # b\" # fora", "m: .asciiz \"a \\\" # b\" ");
    testar_comentario("barra dupla antes da aspa fecha a string",
                      "m: .asciiz \"a \\\\\" # fora", "m: .asciiz \"a \\\\\" ");
    testar_comentario("string nao fechada: # vira comentario",
                      "m: .asciiz \"sem fim # coment", "m: .asciiz \"sem fim ");

    /* Espacos e tabulacoes */
    testar_espacos("espacos repetidos", "add   $t0,   $t1", "add $t0, $t1");
    testar_espacos("tabulacoes viram espaco", "\tli\t$t0,\t5", "li $t0, 5");
    testar_espacos("espacos no inicio e no fim", "   main:   ", "main:");
    testar_espacos("espacos dentro de string sao mantidos",
                   "msg:   .asciiz   \"Resultado   da\toperacao\"",
                   "msg: .asciiz \"Resultado   da\toperacao\"");
    testar_espacos("aspa escapada nao fecha a string",
                   "m: .asciiz \"a \\\"   b\"   ", "m: .asciiz \"a \\\"   b\"");

    /* Arquivo completo */
    testar_arquivo("exemplo do enunciado (2.9)",
                   "# Exemplo de programa\n.data\nmsg: .asciiz \"Resultado # obtido\" # mensagem\n"
                   ".text\nmain:\n  li $t0, 5\n  li $t1, 10 # segundo numero\n  add $t2, $t0, $t1\n",
                   ".data\nmsg: .asciiz \"Resultado # obtido\"\n.text\nmain:\nli $t0, 5\n"
                   "li $t1, 10\nadd $t2, $t0, $t1\n");
    testar_arquivo("linhas vazias e so com espacos sao removidas",
                   "\n\n.text\n   \n\t\nmain:\n\n", ".text\nmain:\n");
    testar_arquivo("quebras \\r\\n, \\r e \\n",
                   ".text\r\nmain:\rli $t0, 1\nnop\r\n", ".text\nmain:\nli $t0, 1\nnop\n");
    testar_arquivo("rotulos e diretivas preservados",
                   "  laco_1:   .word   10\n", "laco_1: .word 10\n");
    testar_arquivo("ultima linha sem quebra continua sem quebra",
                   ".text\nnop", ".text\nnop");

    // Linha longa (maior que o buffer inicial) nao e cortada
    longa = malloc(10001);
    longa_esperada = malloc(10001);
    if (longa != NULL && longa_esperada != NULL) {
        longa[0] = '"';
        for (i = 1; i < 9998; i++) longa[i] = 'a';
        longa[9998] = '"';
        longa[9999] = '\n';
        longa[10000] = '\0';
        strcpy(longa_esperada, longa);
        testar_arquivo("linha de 10000 caracteres preservada", longa, longa_esperada);
    }
    free(longa);
    free(longa_esperada);

    printf("%d de %d teste(s) do pre-processador passaram.\n", total - falhas, total);
    return falhas == 0 ? 0 : 1;
}