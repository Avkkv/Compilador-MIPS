#ifndef SINTATICO_H
#define SINTATICO_H

#include <stdio.h>

/*  Valores devolvidos por AnaliseSintatica. */
#define SINTATICO_VALIDO 0
#define SINTATICO_INVALIDO 1
#define SINTATICO_FALHA (-1)

/*  Analise sintatica descendente recursiva do subconjunto MIPS do enunciado.

    tokens: arquivo .lex (um token por linha, "<NOME, lexema> linha coluna"), aberto para leitura;
    arvore: arquivo .dot, aberto para gravacao. Recebe a arvore sintatica concreta somente quando
            o programa e valido; caso contrario permanece vazio;
    erros:  arquivo .sin.err, aberto para gravacao. Recebe um diagnostico por erro ou a mensagem
            "Nenhum erro sintático encontrado.".

    O marcador EOL e criado internamente a partir das linhas dos tokens, e TK_EOF e acrescentado
    se o .lex nao o tiver. Devolve 0 (valido), 1 (invalido) ou -1 (falha operacional: leitura,
    gravacao, memoria ou .lex mal formado). A abertura e o fechamento dos arquivos ficam com main(). */
int AnaliseSintatica(FILE *tokens, FILE *arvore, FILE *erros);

/*  Quantidade de erros sintaticos da ultima chamada de AnaliseSintatica. */
int QuantidadeErrosSintaticos(void);

#endif
