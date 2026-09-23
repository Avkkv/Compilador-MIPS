#ifndef PREPROCESSADOR_H
#define PREPROCESSADOR_H

#include <stdio.h>

#define TAM_LINHA 4096

void remover_comentario(const char *entrada, char *saida);
void normalizar_espacos(const char *entrada, char *saida);
void processar_linha(const char *linha, FILE *saida);
void preprocessar(FILE *entrada, FILE *saida);

#endif
