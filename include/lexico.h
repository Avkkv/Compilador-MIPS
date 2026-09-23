#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

void AnaliseLexica(FILE *entrada, FILE *saida);
void AnaliseLexicaComSaidas(FILE *entrada, FILE *saida, FILE *tabela_simbolos, FILE *erros);

#endif
