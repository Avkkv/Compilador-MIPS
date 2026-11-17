# Construcoes sintaticamente validas cujos problemas sao semanticos (nao devem ser rejeitadas)
.data
dup: .word 1
dup: .word 2              # rotulo duplicado
     .space -5            # tamanho invalido
     .align 99            # alinhamento invalido
.text
main:
    j inexistente         # rotulo que nao existe
    beq $t0, $t1, outro_inexistente
    la $a0, nao_declarado
    sll $t0, $t1, 99      # deslocamento invalido
    li $t0, 99999999999   # fora dos limites de 32 bits
    addi $t0, $t0, 70000  # imediato fora de 16 bits
    syscall               # sem preparar $v0
main:                     # rotulo duplicado
