#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "lexico.h"

#define MAX_LEXEMA 4096
#define MAX_SIMBOLOS 2048

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
    {"lw", "INS_LW"}, {"la", "INS_LA"}, {"sw", "INS_SW"},
    {"lb", "INS_LB"}, {"sb", "INS_SB"}, {"lh", "INS_LH"}, {"sh", "INS_SH"},
    {"move", "INS_MOVE"}, {"mul", "INS_MUL"},
    {"nor", "INS_NOR"}, {"or", "INS_OR"}, {"sll", "INS_SLL"},
    {"slt", "INS_SLT"}, {"srl", "INS_SRL"}, {"sub", "INS_SUB"},
    {"subu", "INS_SUBU"}, {"mflo", "INS_MFLO"}, {"mfhi", "INS_MFHI"},
    {"syscall", "INS_SYSCALL"}, {"xor", "INS_XOR"},
    {"nop", "INS_NOP"}, {NULL, NULL}
};

static const char *registradores_nomeados[] = {
    "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$t8", "$t9", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5",
    "$s6", "$s7", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra", NULL
};

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
    int indice;
    char *ponteiro_fim;
    long valor_numerico;

    for (indice = 0; registradores_nomeados[indice] != NULL; indice++) {
        if (strcmp(lexema, registradores_nomeados[indice]) == 0) return 1;
    }
    if (lexema[0] != '$' || lexema[1] == '\0') return 0;
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
    for (indice = 0; indice < *quantidade_simbolos; indice++) {
        if (strcmp(simbolos[indice].lexema, lexema) == 0) {
            if (simbolos[indice].linha == 0 && simbolos[indice].coluna == 0 &&
                (linha != 0 || coluna != 0)) {
                simbolos[indice].linha = linha;
                simbolos[indice].coluna = coluna;
            }
            return;
        }
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

static int identificador_valido(const char *lexema)
{
    int indice;
    if (!(isalpha((unsigned char)lexema[0]) || lexema[0] == '_')) return 0;
    for (indice = 1; lexema[indice] != '\0'; indice++) {
        if (!(isalnum((unsigned char)lexema[indice]) || lexema[indice] == '_')) {
            return 0;
        }
    }
    return 1;
}

static void para_minusculas(char *destino, const char *origem)
{
    int indice;
    for (indice = 0; origem[indice] != '\0'; indice++) {
        destino[indice] = (char)tolower((unsigned char)origem[indice]);
    }
    destino[indice] = '\0';
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
    int indice_inicial;

    for (indice_inicial = 0; diretivas[indice_inicial].lexema != NULL; indice_inicial++) {
        registrar_simbolo(simbolos, &quantidade_simbolos,
                          diretivas[indice_inicial].lexema, "diretiva", 0, 0);
    }
    for (indice_inicial = 0; instrucoes[indice_inicial].lexema != NULL; indice_inicial++) {
        registrar_simbolo(simbolos, &quantidade_simbolos,
                          instrucoes[indice_inicial].lexema, "instrucao", 0, 0);
    }
    for (indice_inicial = 0; registradores_nomeados[indice_inicial] != NULL; indice_inicial++) {
        registrar_simbolo(simbolos, &quantidade_simbolos,
                          registradores_nomeados[indice_inicial], "registrador", 0, 0);
    }

    while ((caractere = fgetc(entrada)) != EOF) {
        int linha_inicio = linha;
        int coluna_inicio = coluna;
        char lexema[MAX_LEXEMA];
        int tamanho = 0;

        if (caractere == '\r') continue;
        if (caractere == '\n') { linha++; coluna = 1; continue; }
        if (isspace((unsigned char)caractere)) { coluna++; continue; }
        if (caractere == '#') {
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
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && caractere != '\n') {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
                if (caractere == '\\') {
                    int coluna_barra = coluna - 1;
                    caractere = fgetc(entrada);
                    if (caractere == EOF || caractere == '\n') break;
                    if (caractere != 'n' && caractere != 't' && caractere != '"' &&
                        caractere != '\\' && caractere != '0') {
                        char sequencia[3] = { '\\', (char)caractere, '\0' };
                        registrar_erro(erros, "ERRO_SEQUENCIA_ESCAPE_INVALIDA",
                                       sequencia, linha_inicio, coluna_barra);
                    }
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
            } else if (caractere == EOF) {
                registrar_erro(erros, "ERRO_STRING_NAO_FECHADA_EOF", lexema,
                               linha_inicio, coluna_inicio);
            } else {
                registrar_erro(erros, "ERRO_STRING_NAO_FECHADA_LINHA", lexema,
                               linha_inicio, coluna_inicio);
            }
            if (caractere == '\n') { linha++; coluna = 1; }
            continue;
        }
        if (caractere == '$') {
            char lexema_min[MAX_LEXEMA];
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && !fim_lexema(caractere)) {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
            }
            lexema[tamanho] = '\0';
            para_minusculas(lexema_min, lexema);
            if (eh_registrador(lexema_min)) {
                emitir(saida, "REG", lexema_min, linha_inicio, coluna_inicio);
                registrar_simbolo(simbolos, &quantidade_simbolos, lexema_min,
                                  "registrador", linha_inicio, coluna_inicio);
            } else {
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
            } else {
                registrar_erro(erros, "ERRO_NUMERO_MALFORMADO", lexema,
                               linha_inicio, coluna_inicio);
            }
            if (caractere != EOF) ungetc(caractere, entrada);
            continue;
        }
        if (caractere == '.' || isalpha((unsigned char)caractere) || caractere == '_') {
            char lexema_min[MAX_LEXEMA];
            const char *nome_token;
            lexema[tamanho++] = (char)caractere; coluna++;
            while ((caractere = fgetc(entrada)) != EOF && !fim_lexema(caractere)) {
                if (tamanho < MAX_LEXEMA - 1) {
                    lexema[tamanho++] = (char)caractere;
                }
                coluna++;
            }
            lexema[tamanho] = '\0';
            para_minusculas(lexema_min, lexema);
            if (lexema[0] == '.') {
                nome_token = buscar(diretivas, lexema_min);
                if (nome_token != NULL) {
                    emitir(saida, nome_token, lexema_min, linha_inicio, coluna_inicio);
                    registrar_simbolo(simbolos, &quantidade_simbolos, lexema_min,
                                      "diretiva", linha_inicio, coluna_inicio);
                } else {
                    registrar_erro(erros, "ERRO_DIRETIVA_INVALIDA", lexema,
                                   linha_inicio, coluna_inicio);
                }
            } else if (!identificador_valido(lexema)) {
                registrar_erro(erros, "ERRO_IDENTIFICADOR_MALFORMADO", lexema,
                               linha_inicio, coluna_inicio);
            } else {
                nome_token = buscar(instrucoes, lexema_min);
                if (nome_token != NULL) {
                    emitir(saida, nome_token, lexema_min, linha_inicio, coluna_inicio);
                    registrar_simbolo(simbolos, &quantidade_simbolos, lexema_min,
                                      "instrucao", linha_inicio, coluna_inicio);
                } else {
                    emitir(saida, "ID", lexema, linha_inicio, coluna_inicio);
                    registrar_simbolo(simbolos, &quantidade_simbolos, lexema,
                                      "identificador/rotulo", linha_inicio, coluna_inicio);
                }
            }
            if (caractere != EOF) ungetc(caractere, entrada);
            continue;
        }

        lexema[0] = (char)caractere; lexema[1] = '\0';
        registrar_erro(erros, "ERRO_CARACTERE_INVALIDO", lexema, linha, coluna++);
    }

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
