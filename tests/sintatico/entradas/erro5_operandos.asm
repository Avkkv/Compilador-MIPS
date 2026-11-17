.data
mensagem: .asciiz "ok"
.text
main:
    add $t0, $t1          # falta um registrador
    move $t0, $t1, $t2    # registrador excedente
    syscall $t0           # syscall nao admite operandos
    nop nop               # token adicional
    jr $ra, $t0           # operando excedente
    addi $t0, 5, $t1      # operandos fora de ordem
    li $t0, mensagem      # exige um inteiro
    la $t0, 10            # exige um identificador
    j $ra                 # exige um identificador
    beq $t0, $t1, 10      # destino deve ser um rotulo
    lw $t0, $t1           # endereco invalido
    sll $t0, $t1, $t2     # quantidade deve ser inteiro
    mflo                  # falta o registrador
    li $v0, 10
    syscall
