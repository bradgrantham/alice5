.segment text
        ; x regs
	lui	a5,%hi(.pihex)
	lw	a6,%lo(.pihex)(a5)
	lui	a5,%hi(.result0)
	sw	a6,%lo(.result0)(a5)

        ; f regs
	lui	a5,%hi(.one)
	flw	fa0,%lo(.one)(a5)
	lui	a5,%hi(.result1)
	fsw	fa0,%lo(.result1)(a5)
        ebreak                        

.segment data
.pihex:         .word   0x31415927
.result0:        .word   0
.one:           .fword  1.0
.result1:        .word   0
