.data
msg: .asciiz "Ola"

.text
main:
li $v0, 4
la $a0, msg
syscall
