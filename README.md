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

## Executar

```bash
make run INPUT=tests/entradas/correto1.asm OUTPUT_PRE=tests/saidas/correto1.pre OUTPUT_LEX=tests/saidas/correto1.lex
```

O programa cria automaticamente os arquivos:
- `tests/saidas/correto1.ts`
- `tests/saidas/correto1.err`

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
