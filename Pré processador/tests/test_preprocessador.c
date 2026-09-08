#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../includes/preprocessador.h"


int executar_teste(const char *nome,
                   const char *entrada,
                   const char *esperado)
{
    FILE *arquivo_entrada;
    FILE *arquivo_saida;

    char resultado[2048];

    arquivo_entrada = tmpfile();
    arquivo_saida = tmpfile();

    if (arquivo_entrada == NULL || arquivo_saida == NULL) {
        printf("ERRO: nao foi possivel criar arquivos temporarios.\n");
        return 0;
    }

    fputs(entrada, arquivo_entrada);

    rewind(arquivo_entrada);

    preprocessar(arquivo_entrada, arquivo_saida);

    rewind(arquivo_saida);

    memset(resultado, 0, sizeof(resultado));

    size_t quantidade_lida = fread(
        resultado,
        1,
        sizeof(resultado) - 1,
        arquivo_saida
    );

    resultado[quantidade_lida] = '\0';

    if (strcmp(resultado, esperado) == 0) {
        printf("[PASSOU] %s\n", nome);

        fclose(arquivo_entrada);
        fclose(arquivo_saida);

        return 1;
    }

    printf("[FALHOU] %s\n", nome);

    printf("\nResultado esperado:\n");
    printf("-----\n");
    printf("%s", esperado);
    printf("-----\n");

    printf("\nResultado obtido:\n");
    printf("-----\n");
    printf("%s", resultado);
    printf("-----\n");

    fclose(arquivo_entrada);
    fclose(arquivo_saida);

    return 0;
}


int teste_comentarios()
{
    const char *entrada =
        "# comentario inteiro\n"
        "ADD A B # comentario depois da instrucao\n"
        "LOAD A B\n";

    const char *esperado =
        "ADD A B\n"
        "LOAD A B\n";

    return executar_teste(
        "Remocao de comentarios",
        entrada,
        esperado
    );
}


int teste_string_com_hash()
{
    const char *entrada =
        "LOAD A \"teste # string\"\n"
        "ADD A B\n";

    const char *esperado =
        "LOAD A \"teste # string\"\n"
        "ADD A B\n";

    return executar_teste(
        "Preservacao de # dentro de strings",
        entrada,
        esperado
    );
}


int teste_espacos()
{
    const char *entrada =
        "    ADD      A       B\n"
        "LOAD\tA\tB\n"
        "       STORE       A       B\n";

    const char *esperado =
        "ADD A B\n"
        "LOAD A B\n"
        "STORE A B\n";

    return executar_teste(
        "Normalizacao de espacos e tabs",
        entrada,
        esperado
    );
}


int teste_linhas_vazias()
{
    const char *entrada =
        "\n"
        "     \n"
        "# comentario\n"
        "ADD A B\n"
        "\n"
        "LOAD A B\n";

    const char *esperado =
        "ADD A B\n"
        "LOAD A B\n";

    return executar_teste(
        "Remocao de linhas vazias",
        entrada,
        esperado
    );
}


int main()
{
    int total = 0;
    int aprovados = 0;

    printf("========================================\n");
    printf(" TESTES DO PRE-PROCESSADOR\n");
    printf("========================================\n\n");

    total++;
    if (teste_comentarios()) {
        aprovados++;
    }

    total++;
    if (teste_string_com_hash()) {
        aprovados++;
    }

    total++;
    if (teste_espacos()) {
        aprovados++;
    }

    total++;
    if (teste_linhas_vazias()) {
        aprovados++;
    }

    printf("\n========================================\n");
    printf("Resultado: %d de %d testes passaram.\n",
           aprovados, total);
    printf("========================================\n");

    if (aprovados == total) {
        printf("TODOS OS TESTES PASSARAM!\n");
        return 0;
    }

    printf("ALGUNS TESTES FALHARAM!\n");
    return 1;
}
