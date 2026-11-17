.data 1
ok1: .word 1
erro: .word
ok2: .asciiz "continua"
.text main
    li $t0, 1
    add $t0, $t0
    li $t1, 2
    .data
    lw $t0, 0($sp)
.data
    .byte 1, 2,
.text
fim:
    li $v0, 10
    syscall
    li $t0,