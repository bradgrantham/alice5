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
	lui	a5,%hi(.one)
	flw	fa0,%lo(.one)(a5)
	lui	a5,%hi(.tmp)
	fsw	fa0,%lo(.tmp)(a5)
	lui	a5,%hi(.point5)
	flw	fa1,%lo(.point5)(a5)
        fmul.s    fa2, fa0, fa1
        fdiv.s    fa3, fa0, fa1
        fadd.s    fa4, fa0, fa1
        fsub.s    fa5, fa0, fa1
        addi    t0, zero, 0x1ff
        addi    t1, zero, 0x1ff
        addi    t2, zero, 0x1ff
        addi    t3, zero, 0x1ff
        addi    t4, zero, 0x1ff
        flt.s     t0, fa0, fa1  ; should be 0
        fle.s     t1, fa0, fa1  ; should be 0
        feq.s     t2, fa0, fa1  ; should be 0
        fle.s     t3, fa0, fa0  ; should be 1
        feq.s     t4, fa0, fa0  ; should be 1
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

