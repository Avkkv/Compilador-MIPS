# Testes do Pré-processador

Este diretório contém testes automatizados para verificar o funcionamento do pré-processador.

## Testes realizados

Os testes verificam:

- Remoção de comentários;
- Preservação do caractere `#` dentro de strings;
- Normalização de espaços e tabulações;
- Remoção de linhas vazias e comentários isolados.

## Como executar

A partir da pasta `Pré processador`, compile os testes com:

```bash
gcc tests/test_preprocessador.c src/preprocessador.c -Iincludes -o tests/test_preprocessador.exe
Depois execute:

./tests/test_preprocessador.exe

No Windows PowerShell:

.\tests\test_preprocessador.exe

Resultado esperado
Os testes devem apresentar [PASSOU] quando forem executados corretamente.

Ao final, deve aparecer:

TODOS OS TESTES PASSARAM!
