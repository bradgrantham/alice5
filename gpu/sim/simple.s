.segment text
        addi a0, zero, 0x314
        addi a1, zero, 0x111
        lui a3, %hi(0x31415000)
        jal ra, subr
        addi a4, zero, -1
	lui	a5,%hi(.pihex)
	lw	a6,%lo(.pihex)(a5)
        addi    a7, a6, 1
	lui	a5,%hi(.result)
	sw	a7,%lo(.result)(a5)
        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

.segment data
.pihex:     .word   0x31415927
.result:     .word   0
