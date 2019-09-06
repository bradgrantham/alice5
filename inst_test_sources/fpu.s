.segment text
	lui	a5,%hi(.one)
	flw	fa0,%lo(.one)(a5)
	lui	a5,%hi(.point5)
	flw	fa1,%lo(.point5)(a5)
	lui	a5,%hi(.result0)
        fmul.s  fa2, fa0, fa1           ; .5
        fsw     fa2, %lo(.result0)(a5)
	lui	a5,%hi(.result1)
        fdiv.s  fa3, fa0, fa1           ; 2.0
        fsw     fa3, %lo(.result1)(a5)
	lui	a5,%hi(.result2)
        fadd.s  fa4, fa0, fa1           ; 1.5
        fsw     fa4, %lo(.result2)(a5)
	lui	a5,%hi(.result3)
        fsub.s  fa5, fa1, fa0           ; -.5
        fsw     fa5, %lo(.result3)(a5)
        ebreak                        

subr:
        add a2, a1, a0
        jalr zero, ra, 0

.segment data
.result0:        .word   0
.result1:        .word   0
.result2:        .word   0
.result3:        .word   0
.one:           .fword  1.0
.point5:        .fword  0.5

