#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexico.h"
#include "preprocessador.h"
#include "sintatico.h"

/* ------------------------------------------------------------------------
 * Categorias usadas pelo sintatico
 * ------------------------------------------------------------------------
 * Os nomes sao os mesmos gravados pelo analisador lexico no arquivo .lex.
 * EOL nao existe no .lex: e criado aqui, ao carregar os tokens. */
#define CAT_EOL "EOL"
#define CAT_EOF "TK_EOF"
#define CAT_ID "ID"
#define CAT_REG "REG"
#define CAT_NUM "NUM_INT"
#define CAT_STRING "STRING"
#define CAT_COM "SMB_COM"
#define CAT_COL "SMB_COL"
#define CAT_OPA "SMB_OPA"
#define CAT_CPA "SMB_CPA"

static const char *const ops_tres_registradores[] = {
    "INS_ADD", "INS_ADDU", "INS_SUB", "INS_SUBU", "INS_MUL",
    "INS_AND", "INS_OR", "INS_XOR", "INS_NOR", "INS_SLT", NULL
};
static const char *const ops_dois_registradores[] = {"INS_MOVE", "INS_DIV", NULL};
static const char *const ops_um_registrador[] = {"INS_MFLO", "INS_MFHI", "INS_JR", NULL};
static const char *const ops_imediato[] = {"INS_ADDI", NULL};
static const char *const ops_deslocamento[] = {"INS_SLL", "INS_SRL", NULL};
static const char *const ops_li[] = {"INS_LI", NULL};
static const char *const ops_la[] = {"INS_LA", NULL};
static const char *const ops_memoria[] = {"INS_LW", "INS_SW", "INS_LB", "INS_SB", "INS_LH", "INS_SH", NULL};
static const char *const ops_desvio[] = {"INS_BEQ", "INS_BNE", "INS_BLT", "INS_BLE", "INS_BGT", "INS_BGE", NULL};
static const char *const ops_salto[] = {"INS_J", "INS_JAL", NULL};
static const char *const ops_sem_operandos[] = {"INS_SYSCALL", "INS_NOP", NULL};

static const char *const dirs_inteiros[] = {"DIR_WORD", "DIR_HALF", "DIR_BYTE", NULL};
static const char *const dirs_string[] = {"DIR_ASCII", "DIR_ASCIIZ", NULL};
static const char *const dirs_dados[] = {
    "DIR_WORD", "DIR_HALF", "DIR_BYTE", "DIR_ASCII", "DIR_ASCIIZ", "DIR_SPACE", "DIR_ALIGN", NULL
};

/* Textos usados nas mensagens de "esperado". */
#define ESP_SECAO "DIR_DATA(\".data\") ou DIR_TEXT(\".text\")"
#define ESP_EOL "EOL (fim da linha)"
#define ESP_LINHA_DADOS "rótulo (ID), diretiva de dados ou DIR_GLOBL(\".globl\")"
#define ESP_LINHA_CODIGO "rótulo (ID), instrução ou DIR_GLOBL(\".globl\")"
#define ESP_DIR_DADOS "diretiva de dados (.word, .half, .byte, .ascii, .asciiz, .space ou .align)"

/* ------------------------------------------------------------------------
 * Arvore sintatica concreta
 * ------------------------------------------------------------------------ */
typedef struct No {
    char nome[TAM_NOME_TOKEN];   /* "<programa>" para nao terminais, categoria para terminais */
    char lexema[TAM_LEXEMA];     /* vazio em nao terminais, EOL e TK_EOF */
    int terminal;
    int id;                      /* identificador unico, atribuido em pre-ordem na gravacao */
    struct No **filhos;
    int quantidade_filhos;
    int capacidade_filhos;
} No;

/* ------------------------------------------------------------------------
 * Estado do analisador
 * ------------------------------------------------------------------------ */
static Token *tokens = NULL;          /* tokens do .lex com os EOL inseridos e TK_EOF no fim */
static int quantidade_tokens = 0;
static int capacidade_tokens = 0;
static int posicao = 0;               /* indice do proximo token (lookahead) */

static FILE *arquivo_erros = NULL;
static int quantidade_erros = 0;
static int falha_memoria = 0;
static int falha_gravacao = 0;

/* Complemento da mensagem quando falta EOL logo depois de uma lista de inteiros. */
static const char *esperado_no_fim = NULL;

int QuantidadeErrosSintaticos(void) {
    return quantidade_erros;
}

static void copiar(char *destino, const char *origem, size_t tamanho) {
    size_t n = strlen(origem);
    if (n >= tamanho) n = tamanho - 1;
    memcpy(destino, origem, n);
    destino[n] = '\0';
}

static int pertence(const char *categoria, const char *const lista[]) {
    int i;
    for (i = 0; lista[i] != NULL; i++) {
        if (strcmp(categoria, lista[i]) == 0) return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------
 * Nos da arvore
 * ------------------------------------------------------------------------ */
static No *no_criar(const char *nome, const char *lexema, int terminal) {
    No *no = (No *)calloc(1, sizeof(No));
    if (no == NULL) {
        falha_memoria = 1;
        return NULL;
    }
    copiar(no->nome, nome, sizeof(no->nome));
    copiar(no->lexema, lexema != NULL ? lexema : "", sizeof(no->lexema));
    no->terminal = terminal;
    return no;
}

static void no_liberar(No *no) {
    int i;
    if (no == NULL) return;
    for (i = 0; i < no->quantidade_filhos; i++) no_liberar(no->filhos[i]);
    free(no->filhos);
    free(no);
}

/* Acrescenta "filho" como ultimo filho de "pai". Se pai for NULL (falta de memoria), descarta o filho. */
static No *no_adicionar(No *pai, No *filho) {
    if (filho == NULL) return NULL;
    if (pai == NULL) {
        no_liberar(filho);
        return NULL;
    }
    if (pai->quantidade_filhos == pai->capacidade_filhos) {
        int nova = pai->capacidade_filhos == 0 ? 4 : pai->capacidade_filhos * 2;
        No **novos = (No **)realloc(pai->filhos, (size_t)nova * sizeof(No *));
        if (novos == NULL) {
            falha_memoria = 1;
            no_liberar(filho);
            return NULL;
        }
        pai->filhos = novos;
        pai->capacidade_filhos = nova;
    }
    pai->filhos[pai->quantidade_filhos++] = filho;
    return filho;
}

/* Cria o no do nao terminal "<nome>" como filho de pai. */
static No *nao_terminal(No *pai, const char *nome) {
    return no_adicionar(pai, no_criar(nome, NULL, 0));
}

/* ------------------------------------------------------------------------
 * Leitura do arquivo .lex
 * ------------------------------------------------------------------------ */

/* Le uma linha de tamanho qualquer. Devolve NULL no fim do arquivo. O buffer cresce quando necessario. */
static char *ler_linha(FILE *f, char **buffer, size_t *capacidade) {
    size_t tamanho = 0;
    int c;

    if (*buffer == NULL) {
        *capacidade = 256;
        *buffer = (char *)malloc(*capacidade);
        if (*buffer == NULL) {
            falha_memoria = 1;
            return NULL;
        }
    }
    while ((c = fgetc(f)) != EOF) {
        if (tamanho + 1 >= *capacidade) {
            char *novo = (char *)realloc(*buffer, *capacidade * 2);
            if (novo == NULL) {
                falha_memoria = 1;
                return NULL;
            }
            *buffer = novo;
            *capacidade *= 2;
        }
        if (c == '\n') break;
        (*buffer)[tamanho++] = (char)c;
    }
    if (c == EOF && tamanho == 0) return NULL;
    (*buffer)[tamanho] = '\0';
    return *buffer;
}

/*  Interpreta uma linha "<NOME, lexema> linha coluna".
    O lexema pode conter ", " e ">" (ex.: strings), por isso a linha e a coluna sao lidas a partir do fim
    e o lexema e o texto entre o primeiro ", " e o ultimo "> " antes delas. Devolve 1 se a linha for valida. */
static int interpretar_linha(char *texto, Token *t) {
    size_t n = strlen(texto);
    char *fim, *separador, *p;
    long linha, coluna;
    size_t tamanho_nome, tamanho_lexema;

    while (n > 0 && (texto[n - 1] == '\r' || texto[n - 1] == ' ' || texto[n - 1] == '\t')) texto[--n] = '\0';
    if (n < 4 || texto[0] != '<') return 0;

    /* coluna */
    p = texto + n;
    while (p > texto && isdigit((unsigned char)p[-1])) p--;
    if (p == texto + n || p == texto || p[-1] != ' ') return 0;
    coluna = strtol(p, NULL, 10);
    p--;
    *p = '\0';
    /* linha */
    while (p > texto && isdigit((unsigned char)p[-1])) p--;
    if (*p == '\0' || p == texto || p[-1] != ' ') return 0;
    linha = strtol(p, NULL, 10);
    p--;
    *p = '\0';
    /* '>' que fecha o par */
    if (p == texto || p[-1] != '>') return 0;
    fim = p - 1;

    separador = strstr(texto, ", ");
    if (separador == NULL || separador > fim) return 0;

    tamanho_nome = (size_t)(separador - (texto + 1));
    if (tamanho_nome == 0 || tamanho_nome >= sizeof(t->nome)) return 0;
    memcpy(t->nome, texto + 1, tamanho_nome);
    t->nome[tamanho_nome] = '\0';

    tamanho_lexema = (size_t)(fim - (separador + 2));
    if (tamanho_lexema >= sizeof(t->lexema)) return 0;   /* nao trunca em silencio: o .lex e rejeitado */
    memcpy(t->lexema, separador + 2, tamanho_lexema);
    t->lexema[tamanho_lexema] = '\0';

    t->linha = (int)linha;
    t->coluna = (int)coluna;
    return 1;
}

static int acrescentar_token(const char *nome, const char *lexema, int linha, int coluna) {
    if (quantidade_tokens == capacidade_tokens) {
        int nova = capacidade_tokens == 0 ? 256 : capacidade_tokens * 2;
        Token *novos = (Token *)realloc(tokens, (size_t)nova * sizeof(Token));
        if (novos == NULL) {
            falha_memoria = 1;
            return 0;
        }
        tokens = novos;
        capacidade_tokens = nova;
    }
    copiar(tokens[quantidade_tokens].nome, nome, TAM_NOME_TOKEN);
    copiar(tokens[quantidade_tokens].lexema, lexema, TAM_LEXEMA);
    tokens[quantidade_tokens].linha = linha;
    tokens[quantidade_tokens].coluna = coluna;
    quantidade_tokens++;
    return 1;
}

/* Quantidade de caracteres (nao de bytes) do lexema, para posicionar o EOL logo depois dele. */
static int largura(const char *s) {
    int n = 0;
    for (; *s != '\0'; s++) {
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
    }
    return n;
}

/* Acrescenta um EOL logo depois do ultimo token, se ele ainda nao for um EOL. */
static int fechar_linha(void) {
    const Token *ultimo;
    if (quantidade_tokens == 0) return 1;
    ultimo = &tokens[quantidade_tokens - 1];
    if (strcmp(ultimo->nome, CAT_EOL) == 0) return 1;
    return acrescentar_token(CAT_EOL, "", ultimo->linha, ultimo->coluna + largura(ultimo->lexema));
}

/*  Carrega todos os tokens do .lex na memoria, inserindo um EOL depois do ultimo token de cada linha
    (inclusive a ultima, mesmo sem quebra de linha no fim do arquivo). Linhas vazias e de comentario nao
    geram tokens no .lex e, portanto, nao geram EOL. Se nao houver TK_EOF, ele e acrescentado; se houver,
    nao e duplicado e o que vier depois dele e ignorado. Devolve 1 se deu certo. */
static int carregar_tokens(FILE *f) {
    char *buffer = NULL;
    size_t capacidade = 0;
    char *linha_texto;
    int numero_linha = 0;
    int achou_eof = 0;
    Token t;

    while (!achou_eof && (linha_texto = ler_linha(f, &buffer, &capacidade)) != NULL) {
        numero_linha++;
        if (linha_texto[0] == '\0' || (linha_texto[0] == '\r' && linha_texto[1] == '\0')) continue;
        if (!interpretar_linha(linha_texto, &t)) {
            fprintf(stderr, "Erro: linha %d do arquivo de tokens mal formada.\n", numero_linha);
            if (arquivo_erros != NULL) {
                fprintf(arquivo_erros, "Falha operacional: linha %d do arquivo de tokens mal formada.\n", numero_linha);
            }
            free(buffer);
            return 0;
        }
        if (strcmp(t.nome, CAT_EOF) == 0) {
            if (!fechar_linha() || !acrescentar_token(CAT_EOF, "", t.linha, t.coluna)) break;
            achou_eof = 1;
            break;
        }
        if (quantidade_tokens > 0 && tokens[quantidade_tokens - 1].linha != t.linha) {
            if (!fechar_linha()) break;
        }
        if (!acrescentar_token(t.nome, t.lexema, t.linha, t.coluna)) break;
    }
    free(buffer);

    if (falha_memoria) return 0;
    if (ferror(f)) {
        fprintf(stderr, "Erro: falha de leitura no arquivo de tokens.\n");
        if (arquivo_erros != NULL) fprintf(arquivo_erros, "Falha operacional: erro de leitura no arquivo de tokens.\n");
        return 0;
    }
    if (!achou_eof) {
        int linha = 1, coluna = 1;
        if (!fechar_linha()) return 0;
        if (quantidade_tokens > 0) {
            linha = tokens[quantidade_tokens - 1].linha;
            coluna = tokens[quantidade_tokens - 1].coluna;
        }
        if (!acrescentar_token(CAT_EOF, "", linha, coluna)) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------------
 * Acesso aos tokens
 * ------------------------------------------------------------------------ */
static const Token *atual(void) {
    return &tokens[posicao];
}

static int eh(const char *categoria) {
    return strcmp(atual()->nome, categoria) == 0;
}

/* Avanca para o proximo token. TK_EOF nunca e ultrapassado. */
static void avancar(void) {
    if (!eh(CAT_EOF)) posicao++;
}

static int inicio_secao(void) {
    return eh("DIR_DATA") || eh("DIR_TEXT");
}

/* ------------------------------------------------------------------------
 * Diagnosticos e recuperacao
 * ------------------------------------------------------------------------ */

/* Descreve o token encontrado: categoria e lexema, ou o marcador de fim. */
static void descrever_encontrado(const Token *t, char *destino, size_t tamanho) {
    if (strcmp(t->nome, CAT_EOL) == 0) {
        snprintf(destino, tamanho, "EOL (fim da linha)");
    } else if (strcmp(t->nome, CAT_EOF) == 0) {
        snprintf(destino, tamanho, "TK_EOF (fim do arquivo)");
    } else if (t->lexema[0] == '"') {
        snprintf(destino, tamanho, "%s(%s)", t->nome, t->lexema);   /* STRING ja traz as aspas */
    } else {
        snprintf(destino, tamanho, "%s(\"%s\")", t->nome, t->lexema);
    }
}

/* Descricao padrao de um terminal esperado, ex.: SMB_COM(","). */
static const char *descrever_esperado(const char *categoria) {
    if (strcmp(categoria, CAT_COM) == 0) return "SMB_COM(\",\")";
    if (strcmp(categoria, CAT_COL) == 0) return "SMB_COL(\":\")";
    if (strcmp(categoria, CAT_OPA) == 0) return "SMB_OPA(\"(\")";
    if (strcmp(categoria, CAT_CPA) == 0) return "SMB_CPA(\")\")";
    if (strcmp(categoria, CAT_EOL) == 0) return ESP_EOL;
    if (strcmp(categoria, CAT_EOF) == 0) return "TK_EOF (fim do arquivo)";
    if (strcmp(categoria, "DIR_DATA") == 0) return "DIR_DATA(\".data\")";
    if (strcmp(categoria, "DIR_TEXT") == 0) return "DIR_TEXT(\".text\")";
    if (strcmp(categoria, "DIR_GLOBL") == 0) return "DIR_GLOBL(\".globl\")";
    if (strcmp(categoria, "DIR_SPACE") == 0) return "DIR_SPACE(\".space\")";
    if (strcmp(categoria, "DIR_ALIGN") == 0) return "DIR_ALIGN(\".align\")";
    if (strcmp(categoria, "INS_ADDI") == 0) return "INS_ADDI(\"addi\")";
    if (strcmp(categoria, "INS_LI") == 0) return "INS_LI(\"li\")";
    if (strcmp(categoria, "INS_LA") == 0) return "INS_LA(\"la\")";
    return categoria;
}

/* Registra um erro sintatico na posicao do token atual. */
static void erro(const char *esperado) {
    char encontrado[TAM_NOME_TOKEN + TAM_LEXEMA + 32];
    const Token *t = atual();

    int linha = t->linha, coluna = t->coluna;

    quantidade_erros++;
    descrever_encontrado(t, encontrado, sizeof(encontrado));
    /* Os tokens trazem a posicao no .pre; o diagnostico informa a posicao no programa original (.asm). */
    PosicaoOriginal(t->linha, t->coluna, &linha, &coluna);
    if (arquivo_erros != NULL) {
        if (fprintf(arquivo_erros, "Linha %d, coluna %d:\nEncontrado %s; esperado %s.\n",
                    linha, coluna, encontrado, esperado) < 0) {
            falha_gravacao = 1;
        }
    }
}

/*  Recuperacao em modo panico por linha: descarta os tokens restantes da linha atual e consome o EOL.
    Se o token atual ja for EOL, consome so ele; se for TK_EOF, nao faz nada. Nunca descarta tokens de
    uma linha posterior, porque para no primeiro EOL. */
static void recuperar(void) {
    while (!eh(CAT_EOL) && !eh(CAT_EOF)) posicao++;
    if (eh(CAT_EOL)) posicao++;
}

/*  Consome o token atual se ele for da categoria esperada, criando a folha correspondente em "pai".
    Caso contrario registra o erro e devolve 0 (sem consumir). */
static int esperar(No *pai, const char *categoria, const char *descricao) {
    if (eh(categoria)) {
        const Token *t = atual();
        int com_lexema = strcmp(categoria, CAT_EOL) != 0 && strcmp(categoria, CAT_EOF) != 0;
        no_adicionar(pai, no_criar(t->nome, com_lexema ? t->lexema : NULL, 1));
        avancar();
        return 1;
    }
    erro(descricao != NULL ? descricao : descrever_esperado(categoria));
    return 0;
}

/* Igual a esperar, mas aceita qualquer categoria da lista. */
static int esperar_um_de(No *pai, const char *const lista[], const char *descricao) {
    if (pertence(atual()->nome, lista)) return esperar(pai, atual()->nome, NULL);
    erro(descricao);
    return 0;
}

/* ------------------------------------------------------------------------
 * Funcoes de reconhecimento: uma para cada nao terminal da gramatica.
 * Cada funcao cria o seu no, pendura-o em "pai" e devolve 1 se reconheceu
 * a construcao ou 0 se encontrou um erro (que ja foi registrado).
 * ------------------------------------------------------------------------ */
static void programa(No **raiz);
static void secao(No *pai);
static void secao_dados(No *pai);
static void secao_codigo(No *pai);
static int linha_dados(No *pai);
static int linha_codigo(No *pai);
static int rotulo(No *pai);
static int diretiva_global(No *pai);
static int diretiva_dados(No *pai);
static int diretiva_inteiros(No *pai);
static int lista_inteiros(No *pai);
static int diretiva_string(No *pai);
static int instrucao(No *pai);
static int instrucao_tres_registradores(No *pai);
static int op_tres_registradores(No *pai);
static int instrucao_dois_registradores(No *pai);
static int op_dois_registradores(No *pai);
static int instrucao_um_registrador(No *pai);
static int op_um_registrador(No *pai);
static int instrucao_imediato(No *pai);
static int instrucao_deslocamento(No *pai);
static int op_deslocamento(No *pai);
static int instrucao_li(No *pai);
static int instrucao_la(No *pai);
static int instrucao_memoria(No *pai);
static int op_memoria(No *pai);
static int endereco(No *pai);
static int instrucao_desvio(No *pai);
static int op_desvio(No *pai);
static int instrucao_salto(No *pai);
static int op_salto(No *pai);
static int instrucao_sem_operandos(No *pai);

/* <programa> ::= <secao> { <secao> } TK_EOF */
static void programa(No **raiz) {
    No *no = no_criar("<programa>", NULL, 0);
    *raiz = no;

    if (eh(CAT_EOF)) {
        erro(ESP_SECAO);   /* arquivo vazio ou so com comentarios */
    }
    /* Linhas antes da primeira secao: cada uma e um erro; a analise e retomada na linha seguinte. */
    while (!inicio_secao() && !eh(CAT_EOF)) {
        erro(ESP_SECAO);
        recuperar();
    }
    while (inicio_secao()) secao(no);
    esperar(no, CAT_EOF, NULL);
}

/* <secao> ::= <secao_dados> | <secao_codigo> */
static void secao(No *pai) {
    No *no = nao_terminal(pai, "<secao>");
    if (eh("DIR_DATA")) secao_dados(no);
    else secao_codigo(no);
}

/* <secao_dados> ::= ".data" EOL { <linha_dados> }
   A repeticao termina na abertura de outra secao ou em TK_EOF. */
static void secao_dados(No *pai) {
    No *no = nao_terminal(pai, "<secao_dados>");
    esperar(no, "DIR_DATA", NULL);
    if (!esperar(no, CAT_EOL, NULL)) recuperar();
    while (!inicio_secao() && !eh(CAT_EOF)) {
        if (!linha_dados(no)) recuperar();
    }
}

/* <secao_codigo> ::= ".text" EOL { <linha_codigo> } */
static void secao_codigo(No *pai) {
    No *no = nao_terminal(pai, "<secao_codigo>");
    esperar(no, "DIR_TEXT", NULL);
    if (!esperar(no, CAT_EOL, NULL)) recuperar();
    while (!inicio_secao() && !eh(CAT_EOF)) {
        if (!linha_codigo(no)) recuperar();
    }
}

/* Mensagem para uma construcao que nao pode iniciar (ou seguir um rotulo em) uma linha de dados. */
static void erro_em_linha_dados(int depois_de_rotulo) {
    if (strncmp(atual()->nome, "INS_", 4) == 0) {
        erro(depois_de_rotulo
                 ? "diretiva de dados ou EOL (fim da linha); instruções só são permitidas na seção .text"
                 : ESP_LINHA_DADOS "; instruções só são permitidas na seção .text");
    } else {
        erro(depois_de_rotulo ? ESP_DIR_DADOS " ou EOL (fim da linha)" : ESP_LINHA_DADOS);
    }
}

/* <linha_dados> ::= <rotulo> [ <diretiva_dados> ] EOL
                   | <diretiva_dados> EOL
                   | <diretiva_global> EOL */
static int linha_dados(No *pai) {
    No *no = nao_terminal(pai, "<linha_dados>");
    esperado_no_fim = NULL;

    if (eh(CAT_ID)) {
        if (!rotulo(no)) return 0;
        if (pertence(atual()->nome, dirs_dados)) {
            if (!diretiva_dados(no)) return 0;
        } else if (!eh(CAT_EOL)) {
            erro_em_linha_dados(1);
            return 0;
        }
    } else if (pertence(atual()->nome, dirs_dados)) {
        if (!diretiva_dados(no)) return 0;
    } else if (eh("DIR_GLOBL")) {
        if (!diretiva_global(no)) return 0;
    } else {
        erro_em_linha_dados(0);
        return 0;
    }
    return esperar(no, CAT_EOL, esperado_no_fim);
}

/* Mensagem para uma construcao que nao pode iniciar (ou seguir um rotulo em) uma linha de codigo. */
static void erro_em_linha_codigo(int depois_de_rotulo) {
    if (pertence(atual()->nome, dirs_dados)) {
        erro(depois_de_rotulo
                 ? "instrução ou EOL (fim da linha); diretivas de dados só são permitidas na seção .data"
                 : ESP_LINHA_CODIGO "; diretivas de dados só são permitidas na seção .data");
    } else {
        erro(depois_de_rotulo ? "instrução ou EOL (fim da linha)" : ESP_LINHA_CODIGO);
    }
}

static int inicio_instrucao(void) {
    const char *c = atual()->nome;
    return pertence(c, ops_tres_registradores) || pertence(c, ops_dois_registradores) ||
           pertence(c, ops_um_registrador) || pertence(c, ops_imediato) ||
           pertence(c, ops_deslocamento) || pertence(c, ops_li) || pertence(c, ops_la) ||
           pertence(c, ops_memoria) || pertence(c, ops_desvio) || pertence(c, ops_salto) ||
           pertence(c, ops_sem_operandos);
}

/* <linha_codigo> ::= <rotulo> [ <instrucao> ] EOL
                    | <instrucao> EOL
                    | <diretiva_global> EOL */
static int linha_codigo(No *pai) {
    No *no = nao_terminal(pai, "<linha_codigo>");
    esperado_no_fim = NULL;

    if (eh(CAT_ID)) {
        if (!rotulo(no)) return 0;
        if (inicio_instrucao()) {
            if (!instrucao(no)) return 0;
        } else if (!eh(CAT_EOL)) {
            erro_em_linha_codigo(1);
            return 0;
        }
    } else if (inicio_instrucao()) {
        if (!instrucao(no)) return 0;
    } else if (eh("DIR_GLOBL")) {
        if (!diretiva_global(no)) return 0;
    } else {
        erro_em_linha_codigo(0);
        return 0;
    }
    return esperar(no, CAT_EOL, NULL);
}

/* <rotulo> ::= ID ":" */
static int rotulo(No *pai) {
    No *no = nao_terminal(pai, "<rotulo>");
    return esperar(no, CAT_ID, NULL) && esperar(no, CAT_COL, NULL);
}

/* <diretiva_global> ::= ".globl" ID */
static int diretiva_global(No *pai) {
    No *no = nao_terminal(pai, "<diretiva_global>");
    return esperar(no, "DIR_GLOBL", NULL) && esperar(no, CAT_ID, NULL);
}

/* <diretiva_dados> ::= <diretiva_inteiros> <lista_inteiros>
                      | <diretiva_string> STRING
                      | ".space" NUM_INT
                      | ".align" NUM_INT */
static int diretiva_dados(No *pai) {
    No *no = nao_terminal(pai, "<diretiva_dados>");

    if (pertence(atual()->nome, dirs_inteiros)) {
        return diretiva_inteiros(no) && lista_inteiros(no);
    }
    if (pertence(atual()->nome, dirs_string)) {
        return diretiva_string(no) && esperar(no, CAT_STRING, NULL);
    }
    if (eh("DIR_SPACE")) {
        return esperar(no, "DIR_SPACE", NULL) && esperar(no, CAT_NUM, NULL);
    }
    if (eh("DIR_ALIGN")) {
        return esperar(no, "DIR_ALIGN", NULL) && esperar(no, CAT_NUM, NULL);
    }
    erro(ESP_DIR_DADOS);
    return 0;
}

/* <diretiva_inteiros> ::= ".word" | ".half" | ".byte" */
static int diretiva_inteiros(No *pai) {
    No *no = nao_terminal(pai, "<diretiva_inteiros>");
    return esperar_um_de(no, dirs_inteiros, "DIR_WORD, DIR_HALF ou DIR_BYTE");
}

/* <lista_inteiros> ::= NUM_INT { "," NUM_INT } */
static int lista_inteiros(No *pai) {
    No *no = nao_terminal(pai, "<lista_inteiros>");
    if (!esperar(no, CAT_NUM, NULL)) return 0;
    while (eh(CAT_COM)) {
        esperar(no, CAT_COM, NULL);
        if (!esperar(no, CAT_NUM, NULL)) return 0;   /* ex.: ".word 10," */
    }
    /* Depois da lista so pode vir outra virgula ou o fim da linha. */
    esperado_no_fim = "SMB_COM(\",\") ou EOL (fim da linha)";
    return 1;
}

/* <diretiva_string> ::= ".ascii" | ".asciiz" */
static int diretiva_string(No *pai) {
    No *no = nao_terminal(pai, "<diretiva_string>");
    return esperar_um_de(no, dirs_string, "DIR_ASCII ou DIR_ASCIIZ");
}

/* <instrucao> ::= <instrucao_tres_registradores> | <instrucao_dois_registradores> | ...
   A alternativa e escolhida pela categoria do proximo token (a operacao). */
static int instrucao(No *pai) {
    No *no = nao_terminal(pai, "<instrucao>");
    const char *c = atual()->nome;

    if (pertence(c, ops_tres_registradores)) return instrucao_tres_registradores(no);
    if (pertence(c, ops_dois_registradores)) return instrucao_dois_registradores(no);
    if (pertence(c, ops_um_registrador)) return instrucao_um_registrador(no);
    if (pertence(c, ops_imediato)) return instrucao_imediato(no);
    if (pertence(c, ops_deslocamento)) return instrucao_deslocamento(no);
    if (pertence(c, ops_li)) return instrucao_li(no);
    if (pertence(c, ops_la)) return instrucao_la(no);
    if (pertence(c, ops_memoria)) return instrucao_memoria(no);
    if (pertence(c, ops_desvio)) return instrucao_desvio(no);
    if (pertence(c, ops_salto)) return instrucao_salto(no);
    if (pertence(c, ops_sem_operandos)) return instrucao_sem_operandos(no);
    erro("instrução");
    return 0;
}

/* <instrucao_tres_registradores> ::= <op_tres_registradores> REG "," REG "," REG */
static int instrucao_tres_registradores(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_tres_registradores>");
    return op_tres_registradores(no) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL);
}

/* <op_tres_registradores> ::= "add" | "addu" | "sub" | "subu" | "mul" | "and" | "or" | "xor" | "nor" | "slt" */
static int op_tres_registradores(No *pai) {
    No *no = nao_terminal(pai, "<op_tres_registradores>");
    return esperar_um_de(no, ops_tres_registradores, "operação com três registradores");
}

/* <instrucao_dois_registradores> ::= <op_dois_registradores> REG "," REG */
static int instrucao_dois_registradores(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_dois_registradores>");
    return op_dois_registradores(no) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL);
}

/* <op_dois_registradores> ::= "move" | "div" */
static int op_dois_registradores(No *pai) {
    No *no = nao_terminal(pai, "<op_dois_registradores>");
    return esperar_um_de(no, ops_dois_registradores, "INS_MOVE ou INS_DIV");
}

/* <instrucao_um_registrador> ::= <op_um_registrador> REG */
static int instrucao_um_registrador(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_um_registrador>");
    return op_um_registrador(no) && esperar(no, CAT_REG, NULL);
}

/* <op_um_registrador> ::= "mflo" | "mfhi" | "jr" */
static int op_um_registrador(No *pai) {
    No *no = nao_terminal(pai, "<op_um_registrador>");
    return esperar_um_de(no, ops_um_registrador, "INS_MFLO, INS_MFHI ou INS_JR");
}

/* <instrucao_imediato> ::= "addi" REG "," REG "," NUM_INT */
static int instrucao_imediato(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_imediato>");
    return esperar(no, "INS_ADDI", NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_NUM, NULL);
}

/* <instrucao_deslocamento> ::= <op_deslocamento> REG "," REG "," NUM_INT */
static int instrucao_deslocamento(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_deslocamento>");
    return op_deslocamento(no) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_NUM, NULL);
}

/* <op_deslocamento> ::= "sll" | "srl" */
static int op_deslocamento(No *pai) {
    No *no = nao_terminal(pai, "<op_deslocamento>");
    return esperar_um_de(no, ops_deslocamento, "INS_SLL ou INS_SRL");
}

/* <instrucao_li> ::= "li" REG "," NUM_INT */
static int instrucao_li(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_li>");
    return esperar(no, "INS_LI", NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_NUM, NULL);
}

/* <instrucao_la> ::= "la" REG "," ID */
static int instrucao_la(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_la>");
    return esperar(no, "INS_LA", NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_ID, NULL);
}

/* <instrucao_memoria> ::= <op_memoria> REG "," <endereco> */
static int instrucao_memoria(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_memoria>");
    return op_memoria(no) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           endereco(no);
}

/* <op_memoria> ::= "lw" | "sw" | "lb" | "sb" | "lh" | "sh" */
static int op_memoria(No *pai) {
    No *no = nao_terminal(pai, "<op_memoria>");
    return esperar_um_de(no, ops_memoria, "INS_LW, INS_SW, INS_LB, INS_SB, INS_LH ou INS_SH");
}

/* <endereco> ::= ID | NUM_INT "(" REG ")" */
static int endereco(No *pai) {
    No *no = nao_terminal(pai, "<endereco>");
    if (eh(CAT_ID)) return esperar(no, CAT_ID, NULL);
    if (eh(CAT_NUM)) {
        return esperar(no, CAT_NUM, NULL) && esperar(no, CAT_OPA, NULL) &&
               esperar(no, CAT_REG, NULL) && esperar(no, CAT_CPA, NULL);
    }
    erro("ID ou NUM_INT seguido de SMB_OPA(\"(\") REG SMB_CPA(\")\")");
    return 0;
}

/* <instrucao_desvio> ::= <op_desvio> REG "," REG "," ID */
static int instrucao_desvio(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_desvio>");
    return op_desvio(no) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_REG, NULL) && esperar(no, CAT_COM, NULL) &&
           esperar(no, CAT_ID, NULL);
}

/* <op_desvio> ::= "beq" | "bne" | "blt" | "ble" | "bgt" | "bge" */
static int op_desvio(No *pai) {
    No *no = nao_terminal(pai, "<op_desvio>");
    return esperar_um_de(no, ops_desvio, "INS_BEQ, INS_BNE, INS_BLT, INS_BLE, INS_BGT ou INS_BGE");
}

/* <instrucao_salto> ::= <op_salto> ID */
static int instrucao_salto(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_salto>");
    return op_salto(no) && esperar(no, CAT_ID, NULL);
}

/* <op_salto> ::= "j" | "jal" */
static int op_salto(No *pai) {
    No *no = nao_terminal(pai, "<op_salto>");
    return esperar_um_de(no, ops_salto, "INS_J ou INS_JAL");
}

/* <instrucao_sem_operandos> ::= "syscall" | "nop" */
static int instrucao_sem_operandos(No *pai) {
    No *no = nao_terminal(pai, "<instrucao_sem_operandos>");
    return esperar_um_de(no, ops_sem_operandos, "INS_SYSCALL ou INS_NOP");
}

/* ------------------------------------------------------------------------
 * Gravacao da arvore em DOT
 * ------------------------------------------------------------------------ */
static void numerar(No *no, int *proximo) {
    int i;
    no->id = (*proximo)++;
    for (i = 0; i < no->quantidade_filhos; i++) numerar(no->filhos[i], proximo);
}

/* Grava o texto escapando aspas e barras invertidas para uso dentro de um rotulo DOT. */
static void gravar_escapado(FILE *f, const char *s) {
    for (; *s != '\0'; s++) {
        if (*s == '"' || *s == '\\') fputc('\\', f);
        if (*s == '\n') fputs("\\n", f);
        else if (*s != '\r') fputc(*s, f);
    }
}

static void gravar_nos(FILE *f, const No *no) {
    int i;
    fprintf(f, "    n%d [label=\"", no->id);
    gravar_escapado(f, no->nome);
    if (no->terminal && no->lexema[0] != '\0') {
        fputs("\\n", f);
        gravar_escapado(f, no->lexema);
    }
    fputs("\"];\n", f);
    for (i = 0; i < no->quantidade_filhos; i++) gravar_nos(f, no->filhos[i]);
}

static void gravar_arestas(FILE *f, const No *no) {
    int i;
    for (i = 0; i < no->quantidade_filhos; i++) fprintf(f, "    n%d -> n%d;\n", no->id, no->filhos[i]->id);
    for (i = 0; i < no->quantidade_filhos; i++) gravar_arestas(f, no->filhos[i]);
}

static void gravar_arvore(FILE *f, No *raiz) {
    int proximo = 0;
    numerar(raiz, &proximo);
    fprintf(f, "digraph ArvoreSintatica {\n");
    fprintf(f, "    graph [ordering=out];\n");
    fprintf(f, "    node [shape=box, fontname=\"Arial\"];\n");
    fprintf(f, "    edge [arrowhead=none];\n\n");
    gravar_nos(f, raiz);
    fprintf(f, "\n");
    gravar_arestas(f, raiz);
    fprintf(f, "}\n");
}

/* ------------------------------------------------------------------------
 * Interface publica
 * ------------------------------------------------------------------------ */
static void liberar_tokens(void) {
    free(tokens);
    tokens = NULL;
    quantidade_tokens = 0;
    capacidade_tokens = 0;
    posicao = 0;
}

int AnaliseSintatica(FILE *arquivo_tokens, FILE *arvore, FILE *erros) {
    No *raiz = NULL;
    int resultado;

    arquivo_erros = erros;
    quantidade_erros = 0;
    falha_memoria = 0;
    falha_gravacao = 0;
    esperado_no_fim = NULL;
    liberar_tokens();

    if (arquivo_tokens == NULL || arvore == NULL || erros == NULL) return SINTATICO_FALHA;
    rewind(arquivo_tokens);

    if (!carregar_tokens(arquivo_tokens)) {
        if (falha_memoria) {
            fprintf(stderr, "Erro: memoria insuficiente para a analise sintatica.\n");
            fprintf(erros, "Falha operacional: memória insuficiente.\n");
        }
        liberar_tokens();
        return SINTATICO_FALHA;
    }

    programa(&raiz);

    if (falha_memoria) {
        fprintf(stderr, "Erro: memoria insuficiente para a analise sintatica.\n");
        fprintf(erros, "Falha operacional: memória insuficiente.\n");
        resultado = SINTATICO_FALHA;
    } else if (quantidade_erros == 0) {
        /* Programa valido: grava a arvore completa. */
        fprintf(erros, "Nenhum erro sintático encontrado.\n");
        gravar_arvore(arvore, raiz);
        resultado = SINTATICO_VALIDO;
    } else {
        /* Programa invalido: o .dot fica vazio. */
        resultado = SINTATICO_INVALIDO;
    }

    if (falha_gravacao || ferror(arvore) || ferror(erros)) {
        fprintf(stderr, "Erro: falha de gravacao nas saidas da analise sintatica.\n");
        resultado = SINTATICO_FALHA;
    }

    no_liberar(raiz);
    liberar_tokens();
    return resultado;
}
