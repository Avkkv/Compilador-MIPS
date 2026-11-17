#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "preprocessador.h"

static void processar_linha_final(const char *linha, FILE *saida, int com_quebra);

/* ------------------------------------------------------------------------
 * Mapa de posicoes .pre -> arquivo original
 * Para cada linha gravada no .pre guarda a linha original e, para cada coluna (caractere) do .pre, a coluna original.
 * E usado para que os diagnosticos sintaticos indiquem a posicao no programa escrito pelo usuario.
 * ------------------------------------------------------------------------ */
typedef struct {
    int linha;        /* linha no arquivo original */
    int *colunas;     /* colunas[k] = coluna original do (k+1)-esimo caractere da linha do .pre */
    int quantidade;
} MapaLinha;

static MapaLinha *mapa = NULL;
static int mapa_quantidade = 0;
static int mapa_capacidade = 0;
static int mapa_valido = 0;
static int linhas_originais = 0;          /* linhas lidas do arquivo original */
static int largura_ultima_linha = 0;      /* caracteres da ultima linha original */
static int terminou_com_quebra = 0;       /* 1 se o original termina com quebra de linha */

static int eh_continuacao(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

static int eh_branco(char c) {
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

static int contar_caracteres(const char *s) {
    int n = 0;
    for (; *s != '\0'; s++) if (!eh_continuacao(*s)) n++;
    return n;
}

static void mapa_liberar(void) {
    int i;
    for (i = 0; i < mapa_quantidade; i++) free(mapa[i].colunas);
    free(mapa);
    mapa = NULL;
    mapa_quantidade = 0;
    mapa_capacidade = 0;
    mapa_valido = 0;
    linhas_originais = 0;
    largura_ultima_linha = 0;
    terminou_com_quebra = 0;
}

/*  Registra o mapa da linha original "linha" (numero "numero"), se ela gerar conteudo no .pre.
    A linha normalizada e uma subsequencia da original: o pre-processador so remove comentarios e brancos e troca
    cada sequencia de brancos fora de strings por um espaco. Por isso basta percorrer as duas ao mesmo tempo. */
static void registrar_mapa(const char *linha, int numero) {
    size_t tamanho = strlen(linha) + 1;
    char *sem_comentario, *normalizada;
    size_t i = 0, k;
    int coluna_original = 0;
    MapaLinha m;

    if (!mapa_valido) return;
    sem_comentario = malloc(tamanho);
    normalizada = malloc(tamanho);
    if (sem_comentario == NULL || normalizada == NULL) {
        free(sem_comentario);
        free(normalizada);
        mapa_valido = 0;
        return;
    }
    remover_comentario(linha, sem_comentario);
    normalizar_espacos(sem_comentario, normalizada);
    if (normalizada[0] == '\0') {
        free(sem_comentario);
        free(normalizada);
        return;
    }

    m.linha = numero;
    m.quantidade = 0;
    m.colunas = malloc(sizeof(int) * (strlen(normalizada) + 1));
    if (m.colunas == NULL) {
        free(sem_comentario);
        free(normalizada);
        mapa_valido = 0;
        return;
    }

    for (k = 0; normalizada[k] != '\0'; k++) {
        char c = normalizada[k];
        /* avanca no original ate o caractere correspondente */
        while (linha[i] != '\0' && linha[i] != c && !(c == ' ' && eh_branco(linha[i]))) {
            if (!eh_continuacao(linha[i])) coluna_original++;
            i++;
        }
        if (linha[i] == '\0') break;
        if (!eh_continuacao(linha[i])) coluna_original++;
        if (!eh_continuacao(c)) m.colunas[m.quantidade++] = coluna_original;
        i++;
    }
    free(sem_comentario);
    free(normalizada);

    if (mapa_quantidade == mapa_capacidade) {
        int nova = mapa_capacidade == 0 ? 64 : mapa_capacidade * 2;
        MapaLinha *novo = realloc(mapa, sizeof(MapaLinha) * (size_t)nova);
        if (novo == NULL) {
            free(m.colunas);
            mapa_valido = 0;
            return;
        }
        mapa = novo;
        mapa_capacidade = nova;
    }
    mapa[mapa_quantidade++] = m;
}

/* Registra uma linha original lida: atualiza a contagem de linhas e o mapa. */
static void contar_linha(const char *linha) {
    linhas_originais++;
    largura_ultima_linha = contar_caracteres(linha);
    registrar_mapa(linha, linhas_originais);
}

int PosicaoOriginal(int linha_pre, int coluna_pre, int *linha, int *coluna) {
    const MapaLinha *m;

    if (!mapa_valido || linha_pre < 1 || coluna_pre < 1) return 0;
    if (linha_pre > mapa_quantidade) {
        /* depois da ultima linha do .pre: fim do arquivo original */
        if (linhas_originais == 0) {
            *linha = 1;
            *coluna = 1;
        } else if (terminou_com_quebra) {
            *linha = linhas_originais + 1;
            *coluna = 1;
        } else {
            *linha = linhas_originais;
            *coluna = largura_ultima_linha + 1;
        }
        return 1;
    }
    m = &mapa[linha_pre - 1];
    if (m->quantidade == 0) return 0;
    *linha = m->linha;
    if (coluna_pre <= m->quantidade) {
        *coluna = m->colunas[coluna_pre - 1];
    } else {
        *coluna = m->colunas[m->quantidade - 1] + (coluna_pre - m->quantidade);
    }
    return 1;
}

/*  Verifica se a string que comeca em entrada[inicio] (logo depois da aspa de abertura) e fechada na mesma linha.
    Uma aspa precedida de barra invertida (\") faz parte da string e nao a fecha. */
static int string_fecha_na_linha(const char *entrada, size_t inicio) {
    size_t i = inicio;
    while (entrada[i] != '\0') {
        if (entrada[i] == '\\') {
            if (entrada[i + 1] == '\0') return 0;
            i += 2;
            continue;
        }
        if (entrada[i] == '"') return 1;
        i++;
    }
    return 0;
}

/*  Remove o comentario da linha. O # so inicia comentario fora de strings.
    Se a string nao for fechada na linha, um # dentro dela e tratado como inicio de comentario,
    para que o analisador lexico acuse a string nao fechada sem carregar o texto do comentario. */
void remover_comentario(const char *entrada, char *saida) {
    size_t i = 0;
    size_t j = 0;
    int dentro_str = 0;
    int str_fechada = 0;

    while (entrada[i] != '\0') {
        char c = entrada[i];

        if (dentro_str) {
            if (c == '\\' && entrada[i + 1] != '\0') {
                saida[j++] = c;
                saida[j++] = entrada[i + 1];
                i += 2;
                continue;
            }
            if (c == '#' && !str_fechada) break;
            if (c == '"') dentro_str = 0;
            saida[j++] = c;
            i++;
            continue;
        }

        if (c == '#') break;
        if (c == '"') {
            dentro_str = 1;
            str_fechada = string_fecha_na_linha(entrada, i + 1);
        }
        saida[j++] = c;
        i++;
    }
    saida[j] = '\0';
}

// Troca tabulacoes por espaco, reduz espacos repetidos a um so e remove os espacos do inicio e do fim. O interior das strings nao e alterado.
void normalizar_espacos(const char *entrada, char *saida) {
    size_t i = 0;
    size_t j = 0;
    int dentro_str = 0;

    while (entrada[i] != '\0') {
        char c = entrada[i];

        if (dentro_str) {
            saida[j++] = c;
            if (c == '\\' && entrada[i + 1] != '\0') {
                saida[j++] = entrada[i + 1];
                i += 2;
                continue;
            }
            if (c == '"') dentro_str = 0;
            i++;
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\v' || c == '\f') {
            if (j > 0 && saida[j - 1] != ' ') saida[j++] = ' ';
        } else if (c != '\r' && c != '\n') {
            if (c == '"') dentro_str = 1;
            saida[j++] = c;
        }
        i++;
    }

    while (j > 0 && saida[j - 1] == ' ') j--;
    saida[j] = '\0';
}

// Remove o comentario, normaliza os espacos e grava a linha se sobrar algum conteudo, seguida de '\n'.
void processar_linha(const char *linha, FILE *saida) {
    processar_linha_final(linha, saida, 1);
}

/*  Igual a processar_linha, mas so grava o '\n' se "com_quebra" for 1.
    Usada na ultima linha do arquivo quando ela nao termina em quebra de linha:
    assim uma string aberta nessa linha chega ao analisador lexico como "nao fechada antes do fim do arquivo". */
static void processar_linha_final(const char *linha, FILE *saida, int com_quebra) {
    size_t tamanho = strlen(linha) + 1;
    char *sem_comentario = malloc(tamanho);
    char *normalizada = malloc(tamanho);

    if (sem_comentario == NULL || normalizada == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
        free(sem_comentario);
        free(normalizada);
        return;
    }

    remover_comentario(linha, sem_comentario);
    normalizar_espacos(sem_comentario, normalizada);

    if (normalizada[0] != '\0') {
        fputs(normalizada, saida);
        if (com_quebra) fputc('\n', saida);
    }

    free(sem_comentario);
    free(normalizada);
}

/*  Le a entrada caractere por caractere, montando uma linha de cada vez.
    Aceita \n (Linux), \r\n (Windows) e \r (Mac antigo) como fim de linha. O buffer cresce conforme a necessidade, entao linhas longas nao sao cortadas. */
void PreProcessamento(FILE *in, FILE *out) {
    size_t capacidade = TAM_LINHA_INICIAL;
    size_t tamanho = 0;
    char *linha = malloc(capacidade);
    int c;

    mapa_liberar();
    mapa_valido = 1;

    if (linha == NULL) {
        fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
        mapa_valido = 0;
        return;
    }

    while ((c = fgetc(in)) != EOF) {
        if (c == '\r' || c == '\n') {
            if (c == '\r') {
                int proximo = fgetc(in);
                if (proximo != '\n' && proximo != EOF) ungetc(proximo, in);
            }
            linha[tamanho] = '\0';
            contar_linha(linha);
            processar_linha(linha, out);
            terminou_com_quebra = 1;
            tamanho = 0;
            continue;
        }

        if (tamanho + 1 >= capacidade) {
            char *maior = realloc(linha, capacidade * 2);
            if (maior == NULL) {
                fprintf(stderr, "Erro: memoria insuficiente no pre-processamento.\n");
                free(linha);
                mapa_valido = 0;
                return;
            }
            linha = maior;
            capacidade *= 2;
        }
        linha[tamanho++] = (char)c;
        terminou_com_quebra = 0;
    }

    if (tamanho > 0) {
        linha[tamanho] = '\0';
        contar_linha(linha);
        processar_linha_final(linha, out, 0);
    }

    free(linha);
}
