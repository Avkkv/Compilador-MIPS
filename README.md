# Compilador MIPS

Pré-processador, analisador léxico e analisador sintático para um subconjunto do
Assembly MIPS (sintaxe do simulador MARS), escritos em linguagem C.

## Estrutura

```text
Compilador-MIPS/
├── README.md
├── main.c                          argumentos, abertura/fechamento de arquivos e chamada das etapas
├── preprocessador.c / .h           remoção de comentários, normalização do texto e mapa de posições
├── lexico.c / .h                   AFD, Tabela de Símbolos e registro de erros léxicos
├── sintatico.c / .h                análise descendente recursiva, recuperação de erros e árvore DOT
├── docs/
│   ├── afd.dot / afd.png / afd.svg diagrama do AFD (Graphviz)
│   ├── Relatorio_Compilador.pdf    relatório do pré-processador e do léxico
│   └── Relatorio_Analise_Sintatica.pdf  relatório da análise sintática
└── tests/
    ├── entradas/                   programas .asm de teste do léxico
    ├── test_preprocessador.c       testes unitários do pré-processador
    └── sintatico/
        ├── entradas/               programas .asm de teste do sintático
        ├── saidas/                 saídas de cada teste (.pre, .lex, .ts, .lex.err, .sin.err, .dot, .png)
        └── executar_testes.sh      executa todos os testes e confere o código de saída
```

## Compilar

A partir da raiz do projeto:

```bash
gcc main.c preprocessador.c lexico.c sintatico.c -o main.exe
```

## Executar

```bash
./main.exe <arquivo_entrada.asm> <prefixo_saida>
./main.exe teste.asm resultado
```

São gerados, a partir do prefixo:

| Arquivo | Conteúdo |
| --- | --- |
| `resultado.pre` | programa normalizado pelo pré-processador |
| `resultado.lex` | um token por linha: `<NOME, lexema> linha coluna` |
| `resultado.ts` | Tabela de Símbolos em CSV: `LEXEMA,CATEGORIA,LINHA,COLUNA` |
| `resultado.lex.err` | erros léxicos, ou `Nenhum erro léxico encontrado.` |
| `resultado.sin.err` | erros sintáticos, `Nenhum erro sintático encontrado.` ou o motivo da não execução |
| `resultado.dot` | árvore sintática concreta (vazio se o programa for inválido) |

Ao final o terminal mostra `PROGRAMA SINTATICAMENTE VÁLIDO`, `PROGRAMA SINTATICAMENTE INVÁLIDO`
ou `ANÁLISE SINTÁTICA NÃO EXECUTADA: EXISTEM ERROS LÉXICOS.`

Códigos de encerramento: `0` programa válido; `1` rejeição por erros léxicos ou sintáticos;
`2` falha operacional ou uso incorreto da linha de comando.

Linha e coluna dos diagnósticos sintáticos se referem ao programa original (`.asm`). Os tokens do `.lex` continuam com a posição do `.pre`.

No Windows, para ver os acentos corretamente no terminal, execute antes `chcp 65001`.

## Testes

```bash
sh tests/sintatico/executar_testes.sh
```

## Renderizar a árvore sintática

```bash
dot -Tpng resultado.dot -o resultado.png
```
