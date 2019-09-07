.segment text
	lui	a0,%hi(.pihex)
	lw	a1,%lo(.pihex)(a0)

	lui	a0,%hi(.result0)
        addi    a0, a0,%lo(.result0)

        addi    a2, a1, 0xff            ; a2 = 0x31415A26
	sw      a2,0(a0)

	addi    a0, a0, 4
        addi    a3, zero, 0xa5          ; a3 = 0xa5
	sw      a3,0(a0)

	addi    a0, a0, 4
        and     a4, a3, a1              ; a4 = 0x25
	sw      a4,0(a0)

	addi    a0, a0, 4
        or      a5, a3, a1              ; a5 = 0x314159A7
	sw      a5,0(a0)

	addi    a0, a0, 4
        xor     a2, a3, a1              ; a2 = 0x31415982
	sw      a2,0(a0)

	addi    a0, a0, 4
        addi    a3, zero, -10           ; a3 = 0xfffffff6
	sw      a3,0(a0)

	addi    a0, a0, 4
        slli    a4, a1, 1               ; a4 = 0x6282B24E
	sw      a4,0(a0)

	addi    a0, a0, 4
        srli    a5, a1, 1               ; a5 = 0x18A0AC93
	sw      a5,0(a0)

        lui     a2, %hi(0x80000000)
        or      a1, a1, a2              ; a1 = 0xB1415927
	addi    a0, a0, 4
        srai    a6, a1, 1               ; a6 = 0xD8A0AC93
	sw      a6,0(a0)

        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

.segment data
.pihex:         .word   0x31415927
.result0:       .word   0
                .word   0
                .word   0
                .word   0
                .word   0
                .word   0
                .word   0
                .word   0
                .word   0
