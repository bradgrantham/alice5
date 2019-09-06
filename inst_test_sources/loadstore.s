.segment text
	lui	a5,%hi(.pihex)
	lw	a6,%lo(.pihex)(a5)
	lui	a5,%hi(.result)
	sw	a6,%lo(.result)(a5)
        ebreak                        

.segment data
.pihex:         .word   0x31415927
.result:        .word   0
