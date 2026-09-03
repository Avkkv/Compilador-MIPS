#ifndef PREPROCESSADOR_H
#define PREPROCESSADOR_H

#include <stdio.h>

#define TAM_LINHA 1024

void remover_comentario(const char *entrada, char *saida);
void normalizar_espacos(const char *entrada, char *saida);
void processar_linha(const char *linha, FILE *saida);
void preprocessar(FILE *entrada, FILE *saida);

#endif

/*
Referências:
 - https://www.mentebinaria.com.br/artigos/programa%C3%A7%C3%A3o/construindo-seu-pr%C3%B3prio-compilador-parte-1-r58/
 
*/