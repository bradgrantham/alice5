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
        addi a2, zero, 0
        addi a3, zero, 0
        addi a0, zero, 1
        addi a1, zero, 1
        beq a0, a1, .skipped
        addi a2, zero, 0x123 ; if branch works properly, a2 == 0
.skipped:
        addi a0, zero, 1
        addi a1, zero, 2
        beq a0, a1, .skipped2
        addi a3, zero, 0x321 ; if branch works properly, a3 == 0x321
.skipped2:
        addi a0, zero, 0xff
        addi a1, zero, 0xa5
        and a2, a0, a1
        or a3, a0, a1
        xor a4, a0, a1
        addi a1, zero, -10
        slli a5, a1, 1
        srli a6, a1, 1
        srai a7, a1, 1
        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

.segment data
.pihex:     .word   0x31415927
.result:     .word   0
