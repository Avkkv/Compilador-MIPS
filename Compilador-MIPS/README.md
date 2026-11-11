# Compilador MIPS

Pré-processador e analisador léxico para um subconjunto do Assembly MIPS
(sintaxe do simulador MARS), escritos em Linguagem C.

## Estrutura

```text
Compilador-MIPS/
├── README.md
├── include/
│   ├── preprocessador.h            interface do pré-processador (PreProcessamento)
│   └── lexico.h                    interface do léxico (AnaliseLexica, ProximoToken, Token, Simbolo)
├── src/
│   ├── main.c                      argumentos, abertura/fechamento de arquivos e chamada das etapas
│   ├── preprocessador.c            remoção de comentários e normalização do texto
│   └── lexico.c                    AFD, Tabela de Símbolos e registro de erros
├── docs/
│   ├── afd.dot                     diagrama do AFD (Graphviz)
│   ├── afd.png / afd.svg           diagrama renderizado
│   ├── Relatorio_Compilador.pdf    explica como o código funciona
└── tests/
    ├── entradas/                   programas .asm de teste
    ├── saidas/                     saídas (.pre, .lex, .ts, .err) de cada teste
    ├── test_preprocessador.c       testes unitários do pré-processador
```

## Compilar

A partir da raiz do projeto:

```bash
gcc -std=c99 -src/main.c src/preprocessador.c src/lexico.c -Iinclude -o main.exe
```

## Executar

Com dois argumentos (formato do enunciado da análise léxica):

```bash
./main.exe tests\entradas\correto1.asm tests\saidas\correto1.lex
```

Com três argumentos (informando também o arquivo pré-processado):

```bash
./main.exe tests\entradas\correto1.asm tests\saidas\correto1.pre tests\saidas\correto1.lex
```

Nos dois casos são gerados, ao lado do `.lex`:

| Arquivo | Conteúdo |
| --- | --- |
| `.pre` | programa normalizado pelo pré-processador |
| `.lex` | um token por linha: `<NOME, lexema> linha coluna` |
| `.ts`  | Tabela de Símbolos em CSV: `LEXEMA,CATEGORIA,LINHA,COLUNA` |
| `.err` | erros léxicos: `<TIPO, lexema> linha coluna`, ou `Nenhum erro léxico encontrado.` |

Linha e coluna se referem ao arquivo `.pre`, que é a entrada do analisador léxico.
O programa devolve código 0 em caso de sucesso e 1 se houver erro de argumentos,
de abertura, leitura, gravação ou fechamento de arquivos.

## Diagrama do AFD

```bash
dot -Tpng docs/afd.dot -o docs/afd.png
```