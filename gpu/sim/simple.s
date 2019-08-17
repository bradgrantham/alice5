.segment text
        addi a0, zero, 0x314
        addi a1, zero, 0x111
        lui a3, %hi(0x31415000)
        jal ra, subr
        addi a4, zero, -1
        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

