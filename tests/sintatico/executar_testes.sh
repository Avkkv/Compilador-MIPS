#!/bin/sh
# Executa todos os casos de teste da analise sintatica e confere o codigo de encerramento.
# Uso (a partir da raiz do projeto, depois de compilar main.exe):
#     sh tests/sintatico/executar_testes.sh
# As saidas ficam em tests/sintatico/saidas/<nome>.{pre,lex,ts,lex.err,sin.err,dot}
# e, se o Graphviz estiver instalado, as arvores dos programas validos em <nome>.png.

ENTRADAS=tests/sintatico/entradas
SAIDAS=tests/sintatico/saidas
mkdir -p "$SAIDAS"

falhas=0
for arquivo in "$ENTRADAS"/*.asm; do
    nome=$(basename "$arquivo" .asm)
    case "$nome" in
        valido*) esperado=0 ;;
        *)       esperado=1 ;;
    esac
    ./main.exe "$arquivo" "$SAIDAS/$nome" > /dev/null
    obtido=$?
    if [ "$obtido" -eq "$esperado" ]; then
        echo "ok      $nome (codigo $obtido)"
    else
        echo "FALHOU  $nome (esperado $esperado, obtido $obtido)"
        falhas=$((falhas + 1))
    fi
    if [ "$obtido" -eq 0 ] && command -v dot > /dev/null 2>&1; then
        dot -Tpng "$SAIDAS/$nome.dot" -o "$SAIDAS/$nome.png"
    fi
done

echo "$falhas falha(s)."
[ "$falhas" -eq 0 ]
