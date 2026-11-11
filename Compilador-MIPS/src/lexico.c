#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "lexico.h"

/* ------------------------------------------------------------------------
 * Palavras reservadas
 * ------------------------------------------------------------------------ */
typedef struct {
    const char *lexema;
    const char *nome_token;
} Reservada;

static const Reservada diretivas[] = {
    {".data", "DIR_DATA"},   {".text", "DIR_TEXT"},   {".word", "DIR_WORD"},
    {".half", "DIR_HALF"},   {".byte", "DIR_BYTE"},   {".space", "DIR_SPACE"},
    {".ascii", "DIR_ASCII"}, {".asciiz", "DIR_ASCIIZ"},
    {".globl", "DIR_GLOBL"}, {".align", "DIR_ALIGN"}, {NULL, NULL}
};

static const Reservada instrucoes[] = {
    // Transferencia de dados
    {"li", "INS_LI"},   {"la", "INS_LA"},   {"move", "INS_MOVE"},
    {"lw", "INS_LW"},   {"sw", "INS_SW"},   {"lb", "INS_LB"},
    {"sb", "INS_SB"},   {"lh", "INS_LH"},   {"sh", "INS_SH"},
    // Aritmeticas
    {"add", "INS_ADD"}, {"addi", "INS_ADDI"}, {"addu", "INS_ADDU"},
    {"sub", "INS_SUB"}, {"subu", "INS_SUBU"}, {"mul", "INS_MUL"},
    {"div", "INS_DIV"}, {"mflo", "INS_MFLO"}, {"mfhi", "INS_MFHI"},
    // Logicas, deslocamentos e comparacao
    {"and", "INS_AND"}, {"or", "INS_OR"},   {"xor", "INS_XOR"},
    {"nor", "INS_NOR"}, {"sll", "INS_SLL"}, {"srl", "INS_SRL"},
    {"slt", "INS_SLT"},
    // Desvios e controle
    {"beq", "INS_BEQ"}, {"bne", "INS_BNE"}, {"blt", "INS_BLT"},
    {"ble", "INS_BLE"}, {"bgt", "INS_BGT"}, {"bge", "INS_BGE"},
    {"j", "INS_J"},     {"jal", "INS_JAL"}, {"jr", "INS_JR"},
    {"syscall", "INS_SYSCALL"}, {"nop", "INS_NOP"},
    {NULL, NULL}
};

static const char *registradores_nomeados[] = {
    "$zero", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$k0", "$k1", "$gp", "$sp", "$fp", "$ra", NULL
};

#define CAT_DIRETIVA "diretiva"
#define CAT_INSTRUCAO "instrução"
#define CAT_REGISTRADOR "registrador"
#define CAT_ID "identificador/rótulo"

/* ------------------------------------------------------------------------
 * Estado global do analisador
 * ------------------------------------------------------------------------ */
#define MAX_DEVOLVIDOS 128

typedef struct {
    int caractere;
    int linha;   // posicao antes de ler o caractere
    int coluna;
} Devolvido;

static FILE *entrada_atual = NULL;
static FILE *arquivo_ts = NULL;
static FILE *arquivo_err = NULL;

static int linha_atual = 1;
static int coluna_atual = 1;
static Devolvido devolvidos[MAX_DEVOLVIDOS];
static int quantidade_devolvidos = 0;

static Simbolo tabela[MAX_SIMBOLOS];
static int quantidade_simbolos = 0;
static int quantidade_erros = 0;

/* ------------------------------------------------------------------------
 * Leitura caractere por caractere, com controle de linha e coluna
 * ------------------------------------------------------------------------ */
// Byte de continuacao UTF-8 (10xxxxxx): faz parte do caractere anterior e nao conta como uma nova coluna
static int eh_continuacao_utf8(int c) {
    return c != EOF && (c & 0xC0) == 0x80;
}

/* Lê o próximo caractere e atualiza linha e coluna. Ignora '\r'.
 * Guarda em *linha_antes e *coluna_antes a posicao do caractere lido. */
static int ler(int *linha_antes, int *coluna_antes) {
    int c;

    if (quantidade_devolvidos > 0) {
        Devolvido d = devolvidos[--quantidade_devolvidos];
        linha_atual = d.linha;
        coluna_atual = d.coluna;
        c = d.caractere;
    } else {
        do {
            c = fgetc(entrada_atual);
        } while (c == '\r');
    }

    if (linha_antes != NULL) *linha_antes = linha_atual;
    if (coluna_antes != NULL) *coluna_antes = coluna_atual;

    if (c == '\n') {
        linha_atual++;
        coluna_atual = 1;
    } else if (c != EOF && !eh_continuacao_utf8(c)) {
        coluna_atual++;
    }

    return c;
}

// Devolve um caractere lido, restaurando a posicao anterior a ele
static void devolver(int c, int linha_antes, int coluna_antes) {
    if (quantidade_devolvidos >= MAX_DEVOLVIDOS) return;
    devolvidos[quantidade_devolvidos].caractere = c;
    devolvidos[quantidade_devolvidos].linha = linha_antes;
    devolvidos[quantidade_devolvidos].coluna = coluna_antes;
    quantidade_devolvidos++;

    // A posicao atual passa a ser a do caractere devolvido
    linha_atual = linha_antes;
    coluna_atual = coluna_antes;
}

/* ------------------------------------------------------------------------
 * Lexema em construção
 * ------------------------------------------------------------------------ */
typedef struct {
    char texto[TAM_LEXEMA];
    int tamanho;
    int estourou;  // 1 se o lexema passou de TAM_LEXEMA - 1 caracteres
} Lexema;

static void lexema_limpar(Lexema *l) {
    l->texto[0] = '\0';
    l->tamanho = 0;
    l->estourou = 0;
}

static void lexema_adicionar(Lexema *l, int c) {
    if (l->tamanho < TAM_LEXEMA - 1) {
        l->texto[l->tamanho++] = (char)c;
        l->texto[l->tamanho] = '\0';
    } else {
        l->estourou = 1;
    }
}

/* ------------------------------------------------------------------------
 * Tabela de Simbolos
 * ------------------------------------------------------------------------ */
static void copiar(char *destino, const char *origem, size_t tamanho) {
    strncpy(destino, origem, tamanho - 1);
    destino[tamanho - 1] = '\0';
}

static void ts_registrar(const char *lexema, const char *categoria, int linha, int coluna) {
    int i;
    for (i = 0; i < quantidade_simbolos; i++) {
        if (strcmp(tabela[i].lexema, lexema) == 0) {
            if (tabela[i].primeiraLinha == 0 && linha != 0) {
                tabela[i].primeiraLinha = linha;
                tabela[i].primeiraColuna = coluna;
            }
            return;
        }
    }
    if (quantidade_simbolos >= MAX_SIMBOLOS) {
        fprintf(stderr, "Aviso: Tabela de Simbolos cheia; '%s' nao foi inserido.\n", lexema);
        return;
    }
    copiar(tabela[quantidade_simbolos].lexema, lexema, TAM_LEXEMA);
    copiar(tabela[quantidade_simbolos].categoria, categoria, TAM_CATEGORIA);
    tabela[quantidade_simbolos].primeiraLinha = linha;
    tabela[quantidade_simbolos].primeiraColuna = coluna;
    quantidade_simbolos++;
}

// Carrega instrucoes, diretivas e todos os registradores, nomeados e numericos ($0 a $31), com posicao 0 0 ate o primeiro uso
static void ts_inicializar(void) {
    int i;
    char numerico[8];

    quantidade_simbolos = 0;
    for (i = 0; diretivas[i].lexema != NULL; i++) {
        ts_registrar(diretivas[i].lexema, CAT_DIRETIVA, 0, 0);
    }
    for (i = 0; instrucoes[i].lexema != NULL; i++) {
        ts_registrar(instrucoes[i].lexema, CAT_INSTRUCAO, 0, 0);
    }
    for (i = 0; registradores_nomeados[i] != NULL; i++) {
        ts_registrar(registradores_nomeados[i], CAT_REGISTRADOR, 0, 0);
    }
    for (i = 0; i <= 31; i++) {
        sprintf(numerico, "$%d", i);
        ts_registrar(numerico, CAT_REGISTRADOR, 0, 0);
    }
}

void GravarTabelaSimbolos(FILE *saida) {
    int i;
    if (saida == NULL) return;
    fprintf(saida, "LEXEMA,CATEGORIA,LINHA,COLUNA\n");
    for (i = 0; i < quantidade_simbolos; i++) {
        fprintf(saida, "%s,%s,%d,%d\n", tabela[i].lexema, tabela[i].categoria,
                tabela[i].primeiraLinha, tabela[i].primeiraColuna);
    }
}

/* ------------------------------------------------------------------------
 * Consultas
 * ------------------------------------------------------------------------ */
static const char *buscar(const Reservada *lista, const char *lexema) {
    int i;
    for (i = 0; lista[i].lexema != NULL; i++) {
        if (strcmp(lista[i].lexema, lexema) == 0) return lista[i].nome_token;
    }
    return NULL;
}

static int eh_registrador(const char *lexema) {
    int i;
    char numerico[8];
    for (i = 0; registradores_nomeados[i] != NULL; i++) {
        if (strcmp(lexema, registradores_nomeados[i]) == 0) return 1;
    }
    for (i = 0; i <= 31; i++) {
        sprintf(numerico, "$%d", i);
        if (strcmp(lexema, numerico) == 0) return 1;
    }
    return 0;
}

static void para_minusculas(char *destino, const char *origem) {
    int i;
    for (i = 0; origem[i] != '\0'; i++) {
        destino[i] = (char)tolower((unsigned char)origem[i]);
    }
    destino[i] = '\0';
}

/* ------------------------------------------------------------------------
 * Erros
 * ------------------------------------------------------------------------ */
static void registrar_erro(const char *tipo, const char *lexema, int linha, int coluna) {
    quantidade_erros++;
    if (arquivo_err != NULL) {
        fprintf(arquivo_err, "<%s, %s> %d %d\n", tipo, lexema, linha, coluna);
    }
}

int QuantidadeErrosLexicos(void) {
    return quantidade_erros;
}

/* ------------------------------------------------------------------------
 * Classes de caracteres usadas nas transicoes do AFD
 * ------------------------------------------------------------------------ */
static int eh_letra(int c)  { return c != EOF && c < 128 && isalpha(c); }
static int eh_digito(int c) { return c != EOF && c < 128 && isdigit(c); }
static int eh_hexa(int c)   { return c != EOF && c < 128 && isxdigit(c); }
static int eh_espaco(int c) { return c == ' ' || c == '\t' || c == '\v' || c == '\f'; }

/* Caracteres que encerram um lexema de palavra, registrador ou numero. */
static int eh_delimitador(int c) {
    return c == EOF || c == '\n' || eh_espaco(c) || c == ',' || c == ':' || c == '(' || c == ')' || c == '#' || c == '"';
}

/* ------------------------------------------------------------------------
   ==== AFD ====
   Cada chamada de ProximoToken parte de Q0 e consome caractere por caractere, mudando de estado conforme a tabela de transicoes descrita em
   docs/afd.dot. Quando um caractere nao tem transicao valida no estado atual, o AFD vai para Q_ERRO, que consome o resto do lexema ate um
   delimitador e registra o erro (recuperacao em modo panico).
   ------------------------------------------------------------------------ */

typedef enum {
    Q0,          // inicial
    Q_ID,        // identificador ou instrucao
    Q_PONTO,     // leu '.'
    Q_DIR,       // diretiva
    Q_DOLAR,     // leu '$'
    Q_REG,       // registrador
    Q_MENOS,     // leu '-'
    Q_ZERO,      // leu '0' (pode virar hexadecimal)
    Q_ZERO_X,    // leu "0x"
    Q_HEX,       // numero hexadecimal
    Q_DEC,       // numero decimal
    Q_STR,       // dentro de cadeia
    Q_ESC,       // leu '\' dentro de cadeia
    Q_COMENT,    // comentario
    Q_ERRO       // lexema malformado: consome ate o delimitador
} Estado;

static Token criar_token(const char *nome, const char *lexema, int linha, int coluna) {
    Token t;
    copiar(t.nome, nome, TAM_NOME_TOKEN);
    copiar(t.lexema, lexema, TAM_LEXEMA);
    t.linha = linha;
    t.coluna = coluna;
    return t;
}

// Depois de uma instrucao ou diretiva, verifica se vem ':' (ignorando espacos na mesma linha). Se vier, consome o ':' e devolve 1.
static int seguido_de_dois_pontos(void) {
    Devolvido lidos[MAX_DEVOLVIDOS];
    int quantidade = 0;
    int c, l, col;

    for (;;) {
        c = ler(&l, &col);
        if (c == ':') return 1;
        if (!eh_espaco(c) || quantidade >= MAX_DEVOLVIDOS - 1) break;
        lidos[quantidade].caractere = c;
        lidos[quantidade].linha = l;
        lidos[quantidade].coluna = col;
        quantidade++;
    }
    if (c != EOF) devolver(c, l, col);
    while (quantidade > 0) {
        quantidade--;
        devolver(lidos[quantidade].caractere, lidos[quantidade].linha,
                 lidos[quantidade].coluna);
    }
    return 0;
}

static int eh_escape_valido(int c) {
    return c == 'n' || c == 't' || c == '"' || c == '\\' || c == '0';
}

void IniciarAnaliseLexica(FILE *in) {
    entrada_atual = in;
    linha_atual = 1;
    coluna_atual = 1;
    quantidade_devolvidos = 0;
    quantidade_erros = 0;
    ts_inicializar();
}

void DefinirSaidasLexicas(FILE *tabela_simbolos, FILE *erros) {
    arquivo_ts = tabela_simbolos;
    arquivo_err = erros;
}

Token ProximoToken(void) {
    for (;;) {
        Estado estado = Q0;
        Lexema lex;
        const char *tipo_erro = NULL;
        int linha_ini = linha_atual, coluna_ini = coluna_atual;
        int negativo = 0;
        int c, l, col;

        lexema_limpar(&lex);

        for (;;) {
            c = ler(&l, &col);

            switch (estado) {
            case Q0:
                linha_ini = l;
                coluna_ini = col;
                if (c == EOF) return criar_token("TK_EOF", "EOF", l, col);
                if (c == '\n' || eh_espaco(c)) break;
                lexema_adicionar(&lex, c);
                if (eh_letra(c) || c == '_') estado = Q_ID;
                else if (c == '.') estado = Q_PONTO;
                else if (c == '$') estado = Q_DOLAR;
                else if (c == '-') estado = Q_MENOS;
                else if (c == '0') estado = Q_ZERO;
                else if (eh_digito(c)) estado = Q_DEC;
                else if (c == '"') estado = Q_STR;
                else if (c == '#') { estado = Q_COMENT; lexema_limpar(&lex); }
                else if (c == ',') return criar_token("SMB_COM", ",", l, col);
                else if (c == ':') return criar_token("SMB_COL", ":", l, col);
                else if (c == '(') return criar_token("SMB_OPA", "(", l, col);
                else if (c == ')') return criar_token("SMB_CPA", ")", l, col);
                else {
                    // Caractere invalido. Um caractere UTF-8 de varios bytes (ex.: 'ç') e informado inteiro, como um so erro.
                    int prox, pl, pc;
                    while (eh_continuacao_utf8(prox = ler(&pl, &pc))) {
                        lexema_adicionar(&lex, prox);
                    }
                    if (prox != EOF) devolver(prox, pl, pc);
                    registrar_erro("ERRO_CARACTERE_INVALIDO", lex.texto, linha_ini, coluna_ini);
                    lexema_limpar(&lex);
                    estado = Q0;
                    goto proximo_lexema;
                }
                break;

            case Q_ID:
                if (eh_letra(c) || eh_digito(c) || c == '_') {
                    lexema_adicionar(&lex, c);
                } else if (eh_delimitador(c)) {
                    char minusculo[TAM_LEXEMA];
                    const char *nome;
                    if (c != EOF) devolver(c, l, col);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    para_minusculas(minusculo, lex.texto);
                    nome = buscar(instrucoes, minusculo);
                    if (nome == NULL) {
                        ts_registrar(lex.texto, CAT_ID, linha_ini, coluna_ini);
                        return criar_token("ID", lex.texto, linha_ini, coluna_ini);
                    }
                    if (seguido_de_dois_pontos()) {
                        lexema_adicionar(&lex, ':');
                        tipo_erro = "ERRO_ROTULO_RESERVADO";
                        goto erro;
                    }
                    ts_registrar(minusculo, CAT_INSTRUCAO, linha_ini, coluna_ini);
                    return criar_token(nome, minusculo, linha_ini, coluna_ini);
                } else {
                    lexema_adicionar(&lex, c);
                    tipo_erro = "ERRO_IDENTIFICADOR_MALFORMADO";
                    estado = Q_ERRO;
                }
                break;

            case Q_PONTO:
                if (eh_letra(c)) {
                    lexema_adicionar(&lex, c);
                    estado = Q_DIR;
                } else if (eh_delimitador(c)) {
                    if (c != EOF) devolver(c, l, col);
                    tipo_erro = "ERRO_DIRETIVA_INVALIDA";
                    goto erro;
                } else {
                    lexema_adicionar(&lex, c);
                    tipo_erro = "ERRO_DIRETIVA_INVALIDA";
                    estado = Q_ERRO;
                }
                break;

            case Q_DIR:
                if (eh_letra(c)) {
                    lexema_adicionar(&lex, c);
                } else if (eh_delimitador(c)) {
                    char minusculo[TAM_LEXEMA];
                    const char *nome;
                    if (c != EOF) devolver(c, l, col);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    para_minusculas(minusculo, lex.texto);
                    nome = buscar(diretivas, minusculo);
                    if (nome == NULL) {
                        tipo_erro = "ERRO_DIRETIVA_INVALIDA";
                        goto erro;
                    }
                    if (seguido_de_dois_pontos()) {
                        lexema_adicionar(&lex, ':');
                        tipo_erro = "ERRO_ROTULO_RESERVADO";
                        goto erro;
                    }
                    ts_registrar(minusculo, CAT_DIRETIVA, linha_ini, coluna_ini);
                    return criar_token(nome, minusculo, linha_ini, coluna_ini);
                } else {
                    lexema_adicionar(&lex, c);
                    tipo_erro = "ERRO_DIRETIVA_INVALIDA";
                    estado = Q_ERRO;
                }
                break;

            case Q_DOLAR:
            case Q_REG:
                if (eh_letra(c) || eh_digito(c)) {
                    lexema_adicionar(&lex, c);
                    estado = Q_REG;
                } else if (eh_delimitador(c)) {
                    char minusculo[TAM_LEXEMA];
                    if (c != EOF) devolver(c, l, col);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    para_minusculas(minusculo, lex.texto);
                    if (estado == Q_DOLAR || !eh_registrador(minusculo)) {
                        tipo_erro = "ERRO_REGISTRADOR_INVALIDO";
                        goto erro;
                    }
                    ts_registrar(minusculo, CAT_REGISTRADOR, linha_ini, coluna_ini);
                    return criar_token("REG", minusculo, linha_ini, coluna_ini);
                } else {
                    lexema_adicionar(&lex, c);
                    tipo_erro = "ERRO_REGISTRADOR_INVALIDO";
                    estado = Q_ERRO;
                }
                break;

            case Q_MENOS:
                // O '-' so faz parte do numero se vier um digito logo depois.
                negativo = 1;
                if (c == '0') {
                    lexema_adicionar(&lex, c);
                    estado = Q_ZERO;
                } else if (eh_digito(c)) {
                    lexema_adicionar(&lex, c);
                    estado = Q_DEC;
                } else {
                    if (c != EOF) devolver(c, l, col);
                    registrar_erro("ERRO_CARACTERE_INVALIDO", "-", linha_ini, coluna_ini);
                    goto proximo_lexema;
                }
                break;

            case Q_ZERO:
                if (c == 'x' || c == 'X') {
                    lexema_adicionar(&lex, c);
                    estado = Q_ZERO_X;
                    break;
                }
                // Depois de 0 vale o mesmo que no decimal.
                /* fall through */
            case Q_DEC:
                if (eh_digito(c)) {
                    lexema_adicionar(&lex, c);
                    estado = Q_DEC;
                } else if (eh_delimitador(c)) {
                    if (c != EOF) devolver(c, l, col);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    return criar_token("NUM_INT", lex.texto, linha_ini, coluna_ini);
                } else {
                    // "2valor" comeca com digito mas parece um nome: e um identificador malformado. "-5a" ou "12$" sao numeros malformados.
                    lexema_adicionar(&lex, c);
                    tipo_erro = (!negativo && (eh_letra(c) || c == '_')) ? "ERRO_IDENTIFICADOR_MALFORMADO" : "ERRO_NUMERO_MALFORMADO";
                    estado = Q_ERRO;
                }
                break;

            case Q_ZERO_X:
            case Q_HEX:
                if (eh_hexa(c)) {
                    lexema_adicionar(&lex, c);
                    estado = Q_HEX;
                } else if (eh_delimitador(c)) {
                    if (c != EOF) devolver(c, l, col);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    if (estado == Q_ZERO_X) {
                        tipo_erro = "ERRO_NUMERO_MALFORMADO";
                        goto erro;
                    }
                    return criar_token("NUM_INT", lex.texto, linha_ini, coluna_ini);
                } else {
                    lexema_adicionar(&lex, c);
                    tipo_erro = "ERRO_NUMERO_MALFORMADO";
                    estado = Q_ERRO;
                }
                break;

            case Q_STR:
                if (c == '"') {
                    lexema_adicionar(&lex, c);
                    if (lex.estourou) {
                        tipo_erro = "ERRO_LEXEMA_MUITO_LONGO";
                        goto erro;
                    }
                    return criar_token("STRING", lex.texto, linha_ini, coluna_ini);
                } else if (c == '\\') {
                    lexema_adicionar(&lex, c);
                    estado = Q_ESC;
                } else if (c == '\n') {
                    tipo_erro = "ERRO_STRING_NAO_FECHADA_LINHA";
                    goto erro;
                } else if (c == EOF) {
                    tipo_erro = "ERRO_STRING_NAO_FECHADA_EOF";
                    goto erro;
                } else {
                    lexema_adicionar(&lex, c);
                }
                break;

            case Q_ESC:
                if (c == '\n') {
                    tipo_erro = "ERRO_STRING_NAO_FECHADA_LINHA";
                    goto erro;
                } else if (c == EOF) {
                    tipo_erro = "ERRO_STRING_NAO_FECHADA_EOF";
                    goto erro;
                }
                lexema_adicionar(&lex, c);
                if (!eh_escape_valido(c)) {
                    /* Registra o erro e continua lendo a cadeia. A coluna informada e a da barra invertida. Se o caractere
                       depois da barra for UTF-8 de varios bytes (ex.: \ç), ele aparece inteiro na mensagem. */
                    char sequencia[8];
                    int n = 0;
                    int prox = EOF, pl = 0, pc = 0;
                    sequencia[n++] = '\\';
                    sequencia[n++] = (char)c;
                    while (n < 6) {
                        prox = ler(&pl, &pc);
                        if (!eh_continuacao_utf8(prox)) {
                            if (prox != EOF) devolver(prox, pl, pc);
                            break;
                        }
                        sequencia[n++] = (char)prox;
                        lexema_adicionar(&lex, prox);
                    }
                    sequencia[n] = '\0';
                    registrar_erro("ERRO_SEQUENCIA_ESCAPE_INVALIDA", sequencia, l, col - 1);
                }
                estado = Q_STR;
                break;

            case Q_COMENT:
                // Normalmente o pre-processador ja removeu os comentarios.
                if (c == '\n' || c == EOF) {
                    if (c == EOF) devolver(c, l, col);
                    goto proximo_lexema;
                }
                break;

            case Q_ERRO:
                if (eh_delimitador(c)) {
                    if (c != EOF) devolver(c, l, col);
                    goto erro;
                }
                lexema_adicionar(&lex, c);
                break;
            }
            continue;

        erro:
            if (lex.estourou) {
                // Lexema maior que o limite: informa o comeco seguido de "..." em vez de cortar em silencio.
                char resumo[TAM_LEXEMA];
                copiar(resumo, lex.texto, TAM_LEXEMA - 4);
                strcat(resumo, "...");
                registrar_erro("ERRO_LEXEMA_MUITO_LONGO", resumo, linha_ini, coluna_ini);
            } else {
                registrar_erro(tipo_erro, lex.texto, linha_ini, coluna_ini);
            }
            goto proximo_lexema;
        }

    proximo_lexema:
        ;
    }
}

void AnaliseLexica(FILE *in, FILE *out) {
    Token t;

    IniciarAnaliseLexica(in);
    do {
        t = ProximoToken();
        fprintf(out, "<%s, %s> %d %d\n", t.nome, t.lexema, t.linha, t.coluna);
    } while (strcmp(t.nome, "TK_EOF") != 0);

    GravarTabelaSimbolos(arquivo_ts);
    if (arquivo_err != NULL && quantidade_erros == 0) {
        fprintf(arquivo_err, "Nenhum erro léxico encontrado.\n");
    }
}