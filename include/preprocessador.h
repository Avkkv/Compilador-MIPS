#ifndef PREPROCESSADOR_H
#define PREPROCESSADOR_H

#include <stdio.h>

// Tamanho inicial do buffer de linha. O buffer cresce sozinho quando uma linha e maior que isso, entao nao ha limite fixo de tamanho de linha.
#define TAM_LINHA_INICIAL 256

/*  Le o programa Assembly de "in" e grava em "out" a versao normalizada:
        - comentarios (# ate o fim da linha) removidos, exceto dentro de strings;
        - tabulacoes trocadas por espaco e espacos repetidos reduzidos a um;
        - espacos no inicio e no fim da linha removidos;
        - linhas vazias removidas;
        - quebras de linha \r\n, \r e \n aceitas na entrada e gravadas como \n.
    O conteudo das strings (inclusive \" e #) e preservado sem alteracao.
    Erros de leitura ou gravacao podem ser verificados com ferror(in) e ferror(out) depois da chamada. */
void PreProcessamento(FILE *in, FILE *out);

/*  Funcoes auxiliares, publicas para permitir testes unitarios.
    "linha" nao deve conter o caractere de quebra de linha.
    "saida" deve ter pelo menos strlen(entrada) + 1 bytes. */
void remover_comentario(const char *entrada, char *saida);
void normalizar_espacos(const char *entrada, char *saida);
void processar_linha(const char *linha, FILE *saida);

#endif