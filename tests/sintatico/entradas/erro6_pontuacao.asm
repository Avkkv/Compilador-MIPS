.data
lista: .word 10,          # lista terminada por virgula
vetor .word 1, 2          # faltam os dois-pontos
nums: .word 1 2           # falta a virgula
nome: .asciiz "a", "b"    # lista de strings nao e permitida
    .globl
.text
inicio li $t0, 1          # faltam os dois-pontos
add $t0 $t1, $t2
    lw $t0, 4 $sp         # faltam os parenteses
    lw $t0, 4($sp         # falta o fecha parenteses
    sw $t0, 4 $sp)        # falta o abre parenteses
    li $t0 10             # falta a virgula
    li $v0, 10
    syscall
