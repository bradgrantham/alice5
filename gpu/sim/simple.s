.segment text
        addi    a0, zero, 0x314
        addi    a1, zero, 0x111
        lui     a3, %hi(0x31415000)
        jal     ra, subr
        addi    a4, zero, -1
	lui	a5,%hi(.pihex)
	lw	a6,%lo(.pihex)(a5)
        addi    a7, a6, 1
	lui	a5,%hi(.result)
	sw	a7,%lo(.result)(a5)
        addi    a2, zero, 0
        addi    a3, zero, 0
        addi    a0, zero, 1
        addi    a1, zero, 1
        beq     a0, a1, .skipped
        addi    a2, zero, 0x123 ; if branch works properly, a2 == 0
.skipped:
        addi    a0, zero, 1
        addi    a1, zero, 2
        beq     a0, a1, .skipped2
        addi    a3, zero, 0x321 ; if branch works properly, a3 == 0x321
.skipped2:
        addi    a0, zero, 0xff
        addi    a1, zero, 0xa5
        and     a2, a0, a1
        or      a3, a0, a1
        xor     a4, a0, a1
        addi    a1, zero, -10
        slli    a5, a1, 1
        srli    a6, a1, 1
        srai    a7, a1, 1
	lui	a5,%hi(.one)
	flw	fa0,%lo(.one)(a5)
	lui	a5,%hi(.tmp)
	fsw	fa0,%lo(.tmp)(a5)
	lui	a5,%hi(.point5)
	flw	fa1,%lo(.point5)(a5)
        fmul.s  fa2, fa0, fa1
        fdiv.s  fa3, fa0, fa1
        fadd.s  fa4, fa0, fa1
        fsub.s  fa5, fa0, fa1
        addi    t0, zero, 0x1ff
        addi    t1, zero, 0x1ff
        addi    t2, zero, 0x1ff
        addi    t3, zero, 0x1ff
        addi    t4, zero, 0x1ff
        flt.s   t0, fa0, fa1  ; should be 0
        fle.s   t1, fa0, fa1  ; should be 0
        feq.s   t2, fa0, fa1  ; should be 0
        fle.s   t3, fa0, fa0  ; should be 1
        feq.s   t4, fa0, fa0  ; should be 1
	lui	a5,%hi(.number511point5)
	flw	fa6,%lo(.number511point5)(a5)
        fcvt.w.s  t5, fa6, rdn  ; t5 should become 511
        fcvt.s.w  fa7, t5, rdn  ; fa7 should become 511.0
        fsgnjn.s  ft0, fa7, fa7 ; should be -511
        fsgnjx.s  ft1, ft0, ft0 ; should be 511
        fsgnj.s ft2, fa1, ft0 ; should be -.5
        fmv.x.s t6, ft0
        fmv.s.x ft3, t6
        fmin.s  ft4, ft1, ft2 ; should be -.5
        fmax.s  ft5, ft1, ft2 ; should be 511
	lui	a5,%hi(.number1_5)
	flw     ft6,%lo(.number1_5)(a5)
        fdiv.s  ft7, fa0, ft6, rdn      ; should be .66...6
	lui	a5,%hi(.tmp2)
	fsw     ft7,%lo(.tmp2)(a5)
        fdiv.s  ft8, fa0, ft6, rne      ; should be .66...7
	lui	a5,%hi(.tmp3)
	fsw     ft8,%lo(.tmp3)(a5)
        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

.segment data
.pihex:     .word   0x31415927
.result:     .word   0
.one:
        .word   1065353216      ; 1.0
.tmp:     .word   0
.point5:
        .word   1056964608      ; .5
.number511point5:
        .word   1140834304      ; 0x43ffc000 = 511.5
.number1_5:
        .word   0x3fc00000      ; ;.fword  1.5
.tmp2:     .word   0
.tmp3:     .word   0
