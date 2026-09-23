GUIA DE LEITURA E EXECUCAO DO COMPILADOR MIPS

1. OBJETIVO

O projeto possui duas etapas principais:

1. Pre-processamento: remove comentarios e normaliza os espacos do arquivo
   Assembly.
2. Analise lexica: le o arquivo normalizado, reconhece os lexemas e produz
   tokens, tabela de simbolos e erros lexicos.

A funcao main coordena as duas etapas nesta ordem.


2. ESTRUTURA DE PASTAS

Pre processador/
  includes/preprocessador.h
      Constantes e prototipos do pre-processador.
  src/preprocessador.c
      Remocao de comentarios, normalizacao de espacos e tratamento de linhas.
  src/main.c
      Funcao main, abertura dos arquivos e coordenacao do fluxo.
  tests/test_preprocessador.c
      Testes do pre-processador.

Analise lexica/
  lexico.h
      Prototipos da analise lexica.
  lexico.c
      Reconhecimento dos tokens e geracao da tabela de simbolos e dos erros.


3. COMO COMPILAR

Execute o comando a partir da raiz do projeto, isto e, da pasta
Compilador-MIPS:

gcc -std=c99 -Wall -Wextra -pedantic \
  "Pre processador/src/main.c" \
  "Pre processador/src/preprocessador.c" \
  "Analise lexica/lexico.c" \
  -o main

As aspas sao necessarias porque os nomes das pastas possuem espacos.

No Linux, o executavel pode ser executado com:

./main

No Windows, normalmente:

main.exe


4. COMO EXECUTAR

O programa exige tres argumentos:

./main entrada.asm saida.pre saida.lex

Exemplo:

./main "Pre processador/exemplo/TesteEx.asm" saida.pre saida.lex

Argumentos:

- entrada.asm: arquivo Assembly original.
- saida.pre: arquivo intermediario gerado pelo pre-processador.
- saida.lex: arquivo de tokens gerado pela analise lexica.

A partir do nome de saida.lex, o programa cria automaticamente:

- saida.ts: tabela de simbolos em formato CSV.
- saida.err: erros lexicos encontrados.


5. FLUXO DA EXECUCAO

1. main abre o arquivo Assembly de entrada.
2. preprocessar remove comentarios e normaliza os espacos.
3. O resultado e gravado em saida.pre.
4. main fecha e reabre saida.pre para leitura.
5. AnaliseLexicaComSaidas percorre o arquivo caractere por caractere.
6. Cada lexema reconhecido e escrito em saida.lex.
7. Cada novo lexema tambem e registrado uma unica vez em saida.ts.
8. Erros encontrados sao escritos em saida.err.
9. Ao final, e produzido o token TK_EOF.


6. TOKENS RECONHECIDOS

Diretivas:
.data, .text, .word, .asciiz, .ascii

Instrucoes:
add, addi, addu, and, beq, bne, blt, ble, bgt, bge, div, j, jal, jr,
li, lw, move, mul, nor, or, sll, slt, srl, sub, syscall, xor, nop.

Identificadores:
Comecam com letra ou sublinhado e continuam com letras, digitos ou
sublinhados. Exemplos: main, laco_1, soma_total, _fim.

Registradores:
Os nomes simbolicos permitidos sao os definidos no enunciado, como $zero,
$v0, $t0, $sp e $ra. Tambem sao aceitos $0 ate $31.

Numeros inteiros:
- Decimal: 0, 15, 2025.
- Decimal negativo: -1, -20.
- Hexadecimal: 0x10, 0xFF.
- Hexadecimal negativo: -0x20.

Strings:
Comecam e terminam com aspas duplas. As aspas permanecem no lexema. O
analisador aceita escapes e detecta string sem aspa de fechamento.

Simbolos:
virgula -> SMB_COM
aspa de rotulo (:) -> SMB_COL
abre parenteses -> SMB_OPA
fecha parenteses -> SMB_CPA

Comentarios:
Comecam com # e terminam na quebra de linha. Nao geram token.


7. FORMATO DAS SAIDAS

Arquivo .lex:
Cada linha possui:
< NOME_DO_TOKEN, LEXEMA > LINHA COLUNA

Exemplo real:
<DIR_TEXT, .text> 1 1
<ID, main> 2 1
<SMB_COL, :> 2 5
<INS_LI, li> 3 1
<REG, $v0> 3 4

Arquivo .ts:
Possui cabecalho CSV e uma linha para cada lexema diferente:
LEXEMA,CATEGORIA,LINHA,COLUNA

Categorias usadas:
diretiva, instrucao, identificador/rotulo, registrador, numero, cadeia.

Arquivo .err:
Cada erro possui:
<TIPO_DO_ERRO, LEXEMA> LINHA COLUNA

Exemplos:
<ERRO_REGISTRADOR_INVALIDO, $t10> 4 4
<ERRO_CARACTERE_INVALIDO, @> 6 17
<ERRO_STRING_NAO_FECHADA, "Mensagem> 8 14

Se nao houver erro, o arquivo escreve:
Nenhum erro lexico encontrado.


8. COMO O ANALISADOR FUNCIONA

O analisador mantem duas variaveis de posicao:

- linha: inicia em 1 e aumenta ao encontrar '\n'.
- coluna: inicia em 1 e aumenta a cada caractere da linha.

Ao encontrar um caractere inicial, o analisador escolhe um caminho:

- '#' leva ao estado de comentario.
- '"' leva ao estado de cadeia de caracteres.
- '$' leva ao reconhecimento de registrador.
- '-' ou digito leva ao reconhecimento de numero.
- ponto, letra ou '_' leva ao reconhecimento de diretiva, instrucao ou ID.
- virgula, dois-pontos e parenteses geram tokens imediatamente.
- qualquer outro caractere gera ERRO_CARACTERE_INVALIDO.

As tabelas diretivas[] e instrucoes[] funcionam como uma tabela de simbolos
reservados. Se uma palavra nao estiver na tabela de instrucoes, ela e tratada
como ID. Uma palavra reservada seguida de ':' e rejeitada como rotulo.


9. TESTES

Para compilar os testes existentes:

gcc -std=c99 -Wall -Wextra -pedantic \
  "Pre processador/tests/test_preprocessador.c" \
  "Pre processador/src/preprocessador.c" \
  -o teste_preprocessador

Para executar:

./teste_preprocessador

Os testes verificam:

- remocao de comentarios;
- preservacao de # dentro de strings;
- normalizacao de espacos e tabulacoes;
- remocao de linhas vazias;
- preservacao de linhas longas.


10. ORDEM SUGERIDA PARA LEITURA DO CODIGO

1. Leia este guia.
2. Leia preprocessador.h para conhecer a interface do pre-processador.
3. Leia preprocessador.c para entender a preparacao da entrada.
4. Leia lexico.h para conhecer a interface do analisador.
5. Leia as tabelas diretivas[] e instrucoes[] em lexico.c.
6. Leia as funcoes auxiliares de validacao e emissao.
7. Leia AnaliseLexicaComSaidas, que contem o percurso principal do AFD.
8. Por fim, leia main.c para acompanhar a integracao das etapas.


11. OBSERVACAO SOBRE A INTERFACE

A funcao principal do analisador e:

void AnaliseLexica(FILE *entrada, FILE *saida);

Ela existe como interface simples. O main usa a funcao:

void AnaliseLexicaComSaidas(FILE *entrada, FILE *saida,
                            FILE *tabela_simbolos, FILE *erros);

Essa segunda forma recebe explicitamente os quatro arquivos necessarios para
produzir .lex, .ts e .err.
