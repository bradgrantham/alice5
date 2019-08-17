.segment text
        addi x1, x0, 0x314
        addi x2, x0, 0x111
        add x3, x1, x2
        lui x4, %hi(0x31415000)
        jal x6, skip
cont:
        addi x5, x0, -1
        jal x6, finish
skip:
        jal x6, cont
finish:
        ebreak                        

