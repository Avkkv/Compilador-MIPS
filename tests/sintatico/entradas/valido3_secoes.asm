# Secoes .data e .text alternadas, secao vazia e todas as diretivas
.data
.globl numero
numero:   .word 10
vetor:    .word 10, 20, -30, 0xFF
pequeno:  .byte 65
valor:    .half 1000
mensagem: .asciiz "Resultado: "
texto:    .ascii "ABC \"aspas\" e # dentro da string\n"
buffer:   .space 100
          .align 2
isolado:
.text
.globl main
main:
    la $a0, mensagem
    li $v0, 4
    syscall

.data
.text
.data
extra: .byte 1, 2, 3
.text
    li $v0, 10
    syscall
