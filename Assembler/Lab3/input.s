beq x1, x2, L1
jal x0, L2
L1: xor a5, a3, a7
lui x9, 0x10000
L2: add t1, x8, s10
add x3, x5, x9
jal x5, L2
jalr x4, 58(x5)
L2: addi x5, x6, 5