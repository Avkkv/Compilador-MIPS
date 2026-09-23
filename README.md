# Compilador MIPS

## Estrutura

- include/: arquivos de cabeçalho
- src/: implementação em C
- tests/entradas/: arquivos Assembly de entrada
- tests/saidas/: arquivos gerados em execução
- tests/esperados/: saídas esperadas para validação
- Makefile: compilação automatizada

## Compilar

```bash
make
```

Isso cria o binário em `bin/main`.

ou caso não dê certo, é possível criar o diretório manualmente:

```bash
mkdir bin
```
Cria a pasta bin

```bash
mkdir tests\saidas -Force
```
Cria a pasta "saidas" em "tests"

```bash
gcc -std=c99 -Wall -Wextra -pedantic -Iinclude src/main.c src/preprocessador.c src/lexico.c -o bin/main.exe
```
Faz a compilação

## Executar

```bash
make run INPUT=tests/entradas/correto1.asm OUTPUT_PRE=tests/saidas/correto1.pre OUTPUT_LEX=tests/saidas/correto1.lex
```

O programa cria automaticamente os arquivos:
- `tests/saidas/correto1.ts`
- `tests/saidas/correto1.err`

ou pode ser feito sem o make:

```bash
.\bin\main.exe tests\entradas\correto1.asm tests\saidas\correto1.pre tests\saidas\correto1.lex
```

## Fluxo

O programa recebe 3 argumentos:

```bash
./bin/main <entrada.asm> <saida.pre> <saida.lex>
```

Exemplo:

```bash
./bin/main tests/entradas/correto1.asm tests/saidas/correto1.pre tests/saidas/correto1.lex
```

## Limpeza

```bash
make clean
```
