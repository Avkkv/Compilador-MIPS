.data
valor: .word 10
texto: .ascii "Teste"

.text
main:
li $t0, 10
addi $t1, $t0, -5
lw $t2, 4($sp)
sw $t2, 8($sp)
beq $t0, $t1, fim
j fim

fim:
nop