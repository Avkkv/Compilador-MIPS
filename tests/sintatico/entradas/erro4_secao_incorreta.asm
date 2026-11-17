.data
valor: .word 5
    li $t0, 1             # instrucao em .data
x:  add $t0, $t1, $t2     # rotulo seguido de instrucao em .data
.text
main:
    .word 5               # diretiva de dados em .text
msg: .asciiz "oi"         # rotulo seguido de diretiva em .text
    li $v0, 10
    syscall
