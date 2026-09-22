#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "lexico.h"

#define MAX_LEXEMA 4096
#define MAX_SIMBOLOS 2048

/* Cada entrada associa um lexema reservado ao nome do token produzido. */
typedef struct {
    const char *lexema;
    const char *nome_token;
} Reservada;

typedef struct {
    char lexema[MAX_LEXEMA];
    char categoria[32];
    int linha;
    int coluna;
} Simbolo;

static const Reservada diretivas[] = {
    {".data", "DIR_DATA"}, {".text", "DIR_TEXT"},
    {".word", "DIR_WORD"}, {".asciiz", "DIR_ASCIIZ"},
    {".ascii", "DIR_ASCII"}, {".half", "DIR_HALF"},{".byte", "DIR_BYTE"},
    {".space", "DIR_SPACE"},{".globl", "DIR_GLOBL"},
    {".align", "DIR_ALIGN"},{NULL, NULL}
};

static const Reservada instrucoes[] = {
    {"add", "INS_ADD"}, {"addi", "INS_ADDI"}, {"addu", "INS_ADDU"},
    {"and", "INS_AND"}, {"beq", "INS_BEQ"}, {"bne", "INS_BNE"},
    {"blt", "INS_BLT"}, {"ble", "INS_BLE"}, {"bgt", "INS_BGT"},
    {"bge", "INS_BGE"}, {"div", "INS_DIV"}, {"j", "INS_J"},
    {"jal", "INS_JAL"}, {"jr", "INS_JR"}, {"li", "INS_LI"},
    {"lw", "INS_LW"}, {"la", "INS_LA"}, {"move", "INS_MOVE"}, {"mul", "INS_MUL"},
    {"nor", "INS_NOR"}, {"or", "INS_OR"}, {"sll", "INS_SLL"},
    {"slt", "INS_SLT"}, {"srl", "INS_SRL"}, {"sub", "INS_SUB"},
    {"syscall", "INS_SYSCALL"}, {"xor", "INS_XOR"},
    {"nop", "INS_NOP"}, {NULL, NULL}
};

/* Procura um lexema em uma tabela terminada por uma entrada NULL. */
static const char *buscar(const Reservada *lista, const char *lexema)
{
    int indice;
    for (indice = 0; lista[indice].lexema != NULL; indice++) {
        if (strcmp(lista[indice].lexema, lexema) == 0) {
            return lista[indice].nome_token;
        }
    }
    return NULL;
}

static int eh_registrador(const char *lexema)
{
    static const char *nomes[] = {
        "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
        "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
        "$t8", "$t9", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5",
        "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra", NULL
    };
    int indice;
    char *ponteiro_fim;
    long valor_numerico;

    /* Primeiro verifica os nomes simbolicos, como $t0 e $sp. */
    for (indice = 0; nomes[indice] != NULL; indice++) {
        if (strcmp(lexema, nomes[indice]) == 0) return 1;
    }
    if (lexema[0] != '$' || lexema[1] == '\0') return 0;
    /* Depois aceita a forma numerica somente no intervalo de 0 a 31. */
    valor_numerico = strtol(lexema + 1, &ponteiro_fim, 10);
    return *ponteiro_fim == '\0' && valor_numerico >= 0 && valor_numerico <= 31;
}

static int fim_lexema(int caractere)
{
    return isspace((unsigned char)caractere) || caractere == ',' ||
           caractere == ':' || caractere == '(' || caractere == ')' ||
           caractere == '#' || caractere == '\0';
}

static void registrar_simbolo(Simbolo *simbolos, int *quantidade_simbolos,
                              const char *lexema, const char *categoria,
                              int linha, int coluna)
{
    int indice;
    /* A tabela deve conter cada lexema uma unica vez. */
    for (indice = 0; indice < *quantidade_simbolos; indice++) {
        if (strcmp(simbolos[indice].lexema, lexema) == 0) return;
    }
    if (*quantidade_simbolos >= MAX_SIMBOLOS) return;
    strcpy(simbolos[*quantidade_simbolos].lexema, lexema);
    strcpy(simbolos[*quantidade_simbolos].categoria, categoria);
    simbolos[*quantidade_simbolos].linha = linha;
    simbolos[*quantidade_simbolos].coluna = coluna;
    (*quantidade_simbolos)++;
}

static void emitir(FILE *saida, const char *nome_token, const char *lexema,
                   int linha, int coluna)
{
    fprintf(saida, "<%s, %s> %d %d\n", nome_token, lexema, linha, coluna);
}

static void registrar_erro(FILE *erros, const char *tipo, const char *lexema,
                           int linha, int coluna)
{
    fprintf(erros, "<%s, %s> %d %d\n", tipo, lexema, linha, coluna);
}

static int numero_valido(const char *lexema)
{
    const char *ponteiro = lexema;
    int quantidade_digitos = 0;
    if (*ponteiro == '-') ponteiro++;
    if (ponteiro[0] == '0' &&
        (ponteiro[1] == 'x' || ponteiro[1] == 'X')) {
        ponteiro += 2;
        while (isxdigit((unsigned char)*ponteiro)) {
            quantidade_digitos++;
            ponteiro++;
        }
        return quantidade_digitos > 0 && *ponteiro == '\0';
    }
    while (isdigit((unsigned char)*ponteiro)) {
        quantidade_digitos++;
        ponteiro++;
    }
    return quantidade_digitos > 0 && *ponteiro == '\0';
}

void AnaliseLexicaComSaidas(FILE *entrada, FILE *saida,
                            FILE *tabela_simbolos, FILE *erros)
{
    static Simbolo simbolos[MAX_SIMBOLOS];
    int quantidade_simbolos = 0;
    int caractere, linha = 1, coluna = 1;

     /* O laço implementa o percurso do AFD: cada bloco trata um estado
         inicial diferente e consome exatamente um lexema ou simbolo. */
     while ((caractere = fgetc(entrada)) != EOF) {
        int linha_inicio = linha;
        int coluna_inicio = coluna;
        char lexema[MAX_LEXEMA];
        int tamanho = 0;

        if (caractere == '\n') { linha++; coluna = 1; continue; }
        if (isspace((unsigned char)caractere)) { coluna++; continue; }
        if (caractere == '#') {
            /* Comentarios sao consumidos ate a quebra de linha e nao geram token. */
            while ((caractere = fgetc(entrada)) != EOF && caractere != '\n') {
                coluna++;
            }
            if (caractere == '\n') { linha++; coluna = 1; }
            continue;
        }
        if (caractere == ',' || caractere == ':' || caractere == '(' ||
            caractere == ')') {
            const char *nome_token = caractere == ',' ? "SMB_COM" :
                                     caractere == ':' ? "SMB_COL" :
                                     caractere == '(' ? "SMB_OPA" : "SMB_CPA";
            lexema[0] = (char)caractere; lexema[1] = '\0';
            emitir(saida, nome_token, lexema, linha, coluna++);
            continue;
        }
        if (caractere == '"') {
            int fechada = 0;
            /* A aspa inicial e final fazem parte do lexema da STRING. */
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && caractere != '\n') {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
                if (caractere == '\\') {
                    caractere = fgetc(entrada);
                    if (caractere == EOF || caractere == '\n') break;
                    if (tamanho < MAX_LEXEMA - 1) {
                        lexema[tamanho++] = (char)caractere;
                    }
                    coluna++;
                } else if (caractere == '"') {
                    fechada = 1;
                    break;
                }
            }
            lexema[tamanho] = '\0';
            if (fechada) {
                emitir(saida, "STRING", lexema, linha_inicio, coluna_inicio);
                registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                  "cadeia", linha_inicio, coluna_inicio);
            } else {
                /* A analise continua depois da string incompleta. */
                registrar_erro(erros, "ERRO_STRING_NAO_FECHADA", lexema,
                               linha_inicio, coluna_inicio);
            }
            if (caractere == '\n') { linha++; coluna = 1; }
            continue;
        }
        if (caractere == '$') {
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && !fim_lexema(caractere)) {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
            }
            lexema[tamanho] = '\0';
            if (eh_registrador(lexema)) {
                emitir(saida, "REG", lexema, linha_inicio, coluna_inicio);
                registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                  "registrador", linha_inicio, coluna_inicio);
            } else {
                /* O restante iniciado por $ e consumido como um unico erro. */
                registrar_erro(erros, "ERRO_REGISTRADOR_INVALIDO", lexema,
                               linha_inicio, coluna_inicio);
            }
            if (caractere != EOF) ungetc(caractere, entrada);
            continue;
        }
        if (caractere == '-' || isdigit((unsigned char)caractere)) {
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && !fim_lexema(caractere)) {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
            }
            lexema[tamanho] = '\0';
            if (numero_valido(lexema)) {
                emitir(saida, "NUM_INT", lexema, linha_inicio, coluna_inicio);
                registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                  "numero", linha_inicio, coluna_inicio);
            } else {
                registrar_erro(erros, "ERRO_NUMERO_INVALIDO", lexema,
                               linha_inicio, coluna_inicio);
            }
            if (caractere != EOF) ungetc(caractere, entrada);
            continue;
        }
        if (caractere == '.' || isalpha((unsigned char)caractere) ||
            caractere == '_') {
            const char *nome_token;
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && !fim_lexema(caractere)) {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
            }
            lexema[tamanho] = '\0';
            if (lexema[0] == '.') {
                nome_token = buscar(diretivas, lexema);
                if (nome_token != NULL) {
                    if (caractere == ':') {
                        registrar_erro(erros, "ERRO_IDENTIFICADOR_RESERVADO", lexema,
                                       linha_inicio, coluna_inicio);
                    } else {
                        emitir(saida, nome_token, lexema, linha_inicio, coluna_inicio);
                        registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                          "diretiva", linha_inicio, coluna_inicio);
                    }
                } else registrar_erro(erros, "ERRO_DIRETIVA_INVALIDA", lexema,
                                      linha_inicio, coluna_inicio);
            } else {
                /* Palavra reservada vira instrucao; qualquer outra palavra vira ID. */
                nome_token = buscar(instrucoes, lexema);
                if (nome_token != NULL && caractere == ':') {
                    registrar_erro(erros, "ERRO_IDENTIFICADOR_RESERVADO", lexema,
                                   linha_inicio, coluna_inicio);
                } else {
                    emitir(saida, nome_token == NULL ? "ID" : nome_token, lexema,
                           linha_inicio, coluna_inicio);
                    registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                      nome_token == NULL ? "identificador/rotulo" :
                                      "instrucao", linha_inicio, coluna_inicio);
                }
            }
            if (caractere != EOF) ungetc(caractere, entrada);
            continue;
        }
        lexema[0] = (char)caractere; lexema[1] = '\0';
        registrar_erro(erros, "ERRO_CARACTERE_INVALIDO", lexema, linha, coluna++);
    }

    /* EOF tambem e escrito como token, usando a posicao seguinte ao ultimo char. */
    emitir(saida, "TK_EOF", "EOF", linha, coluna);
    fprintf(tabela_simbolos, "LEXEMA,CATEGORIA,LINHA,COLUNA\n");
    for (int indice = 0; indice < quantidade_simbolos; indice++) {
        fprintf(tabela_simbolos, "%s,%s,%d,%d\n", simbolos[indice].lexema,
                simbolos[indice].categoria, simbolos[indice].linha,
                simbolos[indice].coluna);
    }
}

void AnaliseLexica(FILE *entrada, FILE *saida)
{
    FILE *tabela_simbolos = tmpfile();
    FILE *erros = tmpfile();
    if (tabela_simbolos == NULL || erros == NULL) {
        if (tabela_simbolos != NULL) fclose(tabela_simbolos);
        if (erros != NULL) fclose(erros);
        return;
    }
    AnaliseLexicaComSaidas(entrada, saida, tabela_simbolos, erros);
    fclose(tabela_simbolos);
    fclose(erros);
}