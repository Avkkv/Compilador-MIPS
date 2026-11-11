#ifndef LEXICO_H
#define LEXICO_H

#include <stdio.h>

#define TAM_NOME_TOKEN 50
#define TAM_LEXEMA 100
#define TAM_CATEGORIA 30
#define MAX_SIMBOLOS 4096

// Token devolvido a cada chamada de ProximoToken (estrutura do enunciado).
typedef struct {
    char nome[TAM_NOME_TOKEN];
    char lexema[TAM_LEXEMA];
    int linha;
    int coluna;
} Token;

// Entrada da Tabela de Simbolos (estrutura do enunciado).
typedef struct {
    char lexema[TAM_LEXEMA];
    char categoria[TAM_CATEGORIA];
    int primeiraLinha;
    int primeiraColuna;
} Simbolo;

/*  Le o arquivo pre-processado "in" e grava em "out" um token por linha, no formato  <NOME, lexema> linha coluna, terminando com TK_EOF.
    Ao final grava a Tabela de Simbolos e os erros nos arquivos informados por DefinirSaidasLexicas (se nenhum for informado, eles nao sao gravados). */
void AnaliseLexica(FILE *in, FILE *out);

/*  Informa os arquivos .ts e .err usados por AnaliseLexica.
    Deve ser chamada antes de AnaliseLexica. Qualquer um pode ser NULL. */
void DefinirSaidasLexicas(FILE *tabela_simbolos, FILE *erros);

/*  IniciarAnaliseLexica prepara a leitura de "in" e carrega a Tabela de Simbolos com instrucoes, diretivas e registradores.
    Cada chamada de ProximoToken devolve o proximo token valido; os erros encontrados no caminho sao gravados no arquivo de erros.
    No fim do arquivo devolve TK_EOF. */
void IniciarAnaliseLexica(FILE *in);
Token ProximoToken(void);

// Grava a Tabela de Simbolos em CSV (LEXEMA,CATEGORIA,LINHA,COLUNA).
void GravarTabelaSimbolos(FILE *saida);

// Quantidade de erros lexicos encontrados desde IniciarAnaliseLexica.
int QuantidadeErrosLexicos(void);

#endif