# Testes do Pré-processador

Este diretório contém testes automatizados para verificar o funcionamento do pré-processador.

## Testes realizados

Os testes verificam:

- Remoção de comentários;
- Preservação do caractere `#` dentro de strings;
- Normalização de espaços e tabulações;
- Remoção de linhas vazias e comentários isolados;
- Comportamento do pré-processador com linhas maiores que `TAM_LINHA`.

## Como executar

A partir da pasta `Pré processador`, compile os testes com:

```bash
gcc tests/test_preprocessador.c src/preprocessador.c -Iincludes -o tests/test_preprocessador.exe
Depois execute:

./tests/test_preprocessador.exe

No Windows PowerShell:

.\tests\test_preprocessador.exe

Caso de teste de linha longa
Foi adicionado um teste para verificar o comportamento do pré-processador ao receber uma linha maior que TAM_LINHA.

Atualmente, esse teste não passa, pois uma linha muito longa é dividida em múltiplos trechos durante a leitura.

Esse teste foi incluído para identificar e documentar essa limitação da implementação atual.

Resultado esperado
Os testes básicos devem apresentar [PASSOU] quando executados corretamente.

Atualmente, o teste de linha longa apresenta [FALHOU], pois identifica uma limitação da implementação atual do pré-processador.

O resultado atual esperado é:

Resultado: 4 de 5 testes passaram.
ALGUNS TESTES FALHARAM!

A falha do teste de linha longa é intencional e serve para documentar esse caso de borda.
