; fclass.s test program

.segment data

.NaN:
        .word   0x7fc00000      ; quiet? NaN result of sqrt(-1)
.sNaN:
        .word   0x7fa00000      ; signaling nan
.pzero:
        .fword  0.0
.nzero:
        .fword  -0.0
.psubnorm:
        .word  0x0000FFFF       ; 9.1834E-41
.nsubnorm:
        .word  0x8000FFFF       ; 9.1834E-41
.pnorm:
        .fword  1.0
.nnorm:
        .fword  -1.0
.pinf:
        .word  0x7f800000
.ninf:
        .word  0xff800000

.results:
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0

.segment text

        ; a0 is pointer to .results
        lui a0, %hi(.results)
        addi a0, a0, %lo(.results)

	lui a1, %hi(.ninf)
	flw	ft0, %lo(.ninf)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.nnorm)
	flw	ft0, %lo(.nnorm)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.nsubnorm)
	flw	ft0, %lo(.nsubnorm)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.nzero)
	flw	ft0, %lo(.nzero)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.pzero)
	flw	ft0, %lo(.pzero)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.psubnorm)
	flw	ft0, %lo(.psubnorm)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.pnorm)
	flw	ft0, %lo(.pnorm)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.pinf)
	flw	ft0, %lo(.pinf)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.NaN)
	flw	ft0, %lo(.NaN)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

	lui a1, %hi(.sNaN)
	flw	ft0, %lo(.sNaN)(a1)
        fclass.s        a2, ft0
        sw     a2, 0(a0)
        addi    a0, a0, 4

        ebreak
