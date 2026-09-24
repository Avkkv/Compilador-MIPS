# Compilador MIPS

## Estrutura

- include/: arquivos de cabeçalho
- src/: implementação em C
- tests/entradas/: arquivos Assembly de entrada
- tests/saidas/: arquivos gerados em execução
- tests/esperados/: saídas esperadas para validação
- Makefile: compilação automatizada

## Compilar

A partir da raiz do projeto:

```bash
gcc src/main.c src/preprocessador.c src/lexico.c -Iinclude -o main.exe
```

## Executar

```bash
./main.exe tests/entradas/correto1.asm tests/saidas/correto1.pre tests/saidas/correto1.lex
```

No Windows:

```bat
.\main.exe tests\entradas\correto1.asm tests\saidas\correto1.pre tests\saidas\correto1.lex
```

## Fluxo

O programa recebe 3 argumentos:

```bash
./main.exe <entrada.asm> <saida.pre> <saida.lex>
```

Exemplo:

```bash
./main.exe tests/entradas/correto1.asm tests/saidas/correto1.pre tests/saidas/correto1.lex
```

