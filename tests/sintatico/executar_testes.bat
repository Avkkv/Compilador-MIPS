@echo off
rem Executa todos os casos de teste da analise sintatica e confere o codigo de encerramento.
rem Uso (no cmd ou no terminal do VS Code, a partir da raiz do projeto, depois de compilar main.exe):
rem     .\tests\sintatico\executar_testes.bat
rem As saidas ficam em tests\sintatico\saidas\<nome>.{pre,lex,ts,lex.err,sin.err,dot}
rem e, se o Graphviz estiver instalado, as arvores dos programas validos em <nome>.png.
setlocal enabledelayedexpansion

set ENTRADAS=tests\sintatico\entradas
set SAIDAS=tests\sintatico\saidas

if not exist main.exe (
    echo main.exe nao encontrado. Compile primeiro:
    echo     gcc main.c preprocessador.c lexico.c sintatico.c -o main.exe
    exit /b 2
)
if not exist %SAIDAS% mkdir %SAIDAS%

set falhas=0
for %%F in (%ENTRADAS%\*.asm) do (
    set nome=%%~nF
    set esperado=1
    if /i "!nome:~0,6!"=="valido" set esperado=0
    main.exe "%%F" "%SAIDAS%\!nome!" > nul
    set obtido=!errorlevel!
    if "!obtido!"=="!esperado!" (
        echo ok      !nome! ^(codigo !obtido!^)
    ) else (
        echo FALHOU  !nome! ^(esperado !esperado!, obtido !obtido!^)
        set /a falhas+=1
    )
    if "!obtido!"=="0" (
        where dot > nul 2> nul && dot -Tpng "%SAIDAS%\!nome!.dot" -o "%SAIDAS%\!nome!.png"
    )
)

echo !falhas! falha^(s^).
if !falhas! gtr 0 exit /b 1
exit /b 0
