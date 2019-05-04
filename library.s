; library of math functions

.sin:
        ; uses a5, f5, f4, f0, and a3
        sw      a3, -4(sp)      ; save registers
        sw      a4, -8(sp)
        sw      a5, -12(sp)
        fsw     f0, -16(sp)
        fsw     f4, -20(sp)
        fsw     f5, -24(sp)
        flw     f0, 0(sp)       ; load first parameter ("x")
	lui	a5,%hi(.LC0)
	flw	fa5,%lo(.LC0)(a5)
	fmv.s.x	fa4,zero
	lui	a5,%hi(.LC1)
	fmul.s	fa5,fa0,fa5
	flt.s	a4,fa0,fa4
	flw	fa4,%lo(.LC1)(a5)
	fcvt.w.s a5,fa5,rtz
	sub	a5,a5,a4
	fcvt.s.w	fa0,a5
	fsub.s	fa0,fa5,fa0
	fmul.s	fa0,fa0,fa4
	fge.s	a5,fa0,fa4
	beqz	a5,.L4
	fsub.s	fa0,fa0,fa4
.L4:
	fcvt.wu.s a5,fa0,rtz
	lui	a3,%hi(sinTable_f32)
	addi	a3,a3,%lo(sinTable_f32)
	andi	a5,a5,511
	fcvt.s.wu	fa5,a5
	addi	a4,a5,1
	slli	a4,a4,2
	fsub.s	fa0,fa0,fa5
	add	a4,a4,a3
	flw	fa4,0(a4)
	lui	a4,%hi(.LC2)
	flw	fa5,%lo(.LC2)(a4)
	slli	a5,a5,2
	add	a5,a5,a3
	fsub.s	fa5,fa5,fa0
	fmul.s	fa0,fa0,fa4
	flw	fa4,0(a5)
	fmadd.s	fa0,fa5,fa4,fa0
        fsw     f0, 0(sp)       ; store return value
        lw      a3, -4(sp)      ; restore registers
        lw      a4, -8(sp)
        lw      a5, -12(sp)
        flw     f0, -16(sp)
        flw     f4, -20(sp)
        flw     f5, -24(sp)
        jalr x0, ra, 0                

sinTable_f32:
        .word   0
        .word   1011420818
        .word   1019808433
        .word   1024901931
        .word   1028193070
        .word   1031482229
        .word   1033283844
        .word   1034925696
        .word   1036565814
        .word   1038203951
        .word   1039839859
        .word   1040830343
        .word   1041645699
        .word   1042459573
        .word   1043271842
        .word   1044082383
        .word   1044891074
        .word   1045697793
        .word   1046502419
        .word   1047304831
        .word   1048104908
        .word   1048739264
        .word   1049136787
        .word   1049532962
        .word   1049927730
        .word   1050321030
        .word   1050712805
        .word   1051102994
        .word   1051491540
        .word   1051878383
        .word   1052263466
        .word   1052646729
        .word   1053028117
        .word   1053407571
        .word   1053785034
        .word   1054160449
        .word   1054533760
        .word   1054904911
        .word   1055273845
        .word   1055640507
        .word   1056004842
        .word   1056366795
        .word   1056726311
        .word   1057023972
        .word   1057201213
        .word   1057377154
        .word   1057551771
        .word   1057725035
        .word   1057896922
        .word   1058067405
        .word   1058236458
        .word   1058404057
        .word   1058570176
        .word   1058734790
        .word   1058897873
        .word   1059059403
        .word   1059219353
        .word   1059377701
        .word   1059534422
        .word   1059689493
        .word   1059842890
        .word   1059994590
        .word   1060144571
        .word   1060292809
        .word   1060439283
        .word   1060583971
        .word   1060726850
        .word   1060867899
        .word   1061007097
        .word   1061144423
        .word   1061279856
        .word   1061413377
        .word   1061544963
        .word   1061674597
        .word   1061802258
        .word   1061927928
        .word   1062051586
        .word   1062173216
        .word   1062292797
        .word   1062410313
        .word   1062525745
        .word   1062639077
        .word   1062750291
        .word   1062859370
        .word   1062966298
        .word   1063071059
        .word   1063173637
        .word   1063274017
        .word   1063372183
        .word   1063468122
        .word   1063561817
        .word   1063653256
        .word   1063742424
        .word   1063829308
        .word   1063913895
        .word   1063996172
        .word   1064076126
        .word   1064153747
        .word   1064229022
        .word   1064301939
        .word   1064372488
        .word   1064440658
        .word   1064506439
        .word   1064569821
        .word   1064630795
        .word   1064689350
        .word   1064745479
        .word   1064799173
        .word   1064850424
        .word   1064899224
        .word   1064945565
        .word   1064989442
        .word   1065030846
        .word   1065069773
        .word   1065106216
        .word   1065140169
        .word   1065171628
        .word   1065200588
        .word   1065227043
        .word   1065250992
        .word   1065272429
        .word   1065291352
        .word   1065307757
        .word   1065321643
        .word   1065333007
        .word   1065341847
        .word   1065348163
        .word   1065351953
        .word   1065353216
        .word   1065351953
        .word   1065348163
        .word   1065341847
        .word   1065333007
        .word   1065321643
        .word   1065307757
        .word   1065291352
        .word   1065272429
        .word   1065250992
        .word   1065227043
        .word   1065200588
        .word   1065171628
        .word   1065140169
        .word   1065106216
        .word   1065069773
        .word   1065030846
        .word   1064989442
        .word   1064945565
        .word   1064899224
        .word   1064850424
        .word   1064799173
        .word   1064745479
        .word   1064689350
        .word   1064630795
        .word   1064569821
        .word   1064506439
        .word   1064440658
        .word   1064372488
        .word   1064301939
        .word   1064229022
        .word   1064153747
        .word   1064076126
        .word   1063996172
        .word   1063913895
        .word   1063829308
        .word   1063742424
        .word   1063653256
        .word   1063561817
        .word   1063468122
        .word   1063372183
        .word   1063274017
        .word   1063173637
        .word   1063071059
        .word   1062966298
        .word   1062859370
        .word   1062750291
        .word   1062639077
        .word   1062525745
        .word   1062410313
        .word   1062292797
        .word   1062173216
        .word   1062051586
        .word   1061927928
        .word   1061802258
        .word   1061674597
        .word   1061544963
        .word   1061413377
        .word   1061279856
        .word   1061144423
        .word   1061007097
        .word   1060867899
        .word   1060726850
        .word   1060583971
        .word   1060439283
        .word   1060292809
        .word   1060144571
        .word   1059994590
        .word   1059842890
        .word   1059689493
        .word   1059534422
        .word   1059377701
        .word   1059219353
        .word   1059059403
        .word   1058897873
        .word   1058734790
        .word   1058570176
        .word   1058404057
        .word   1058236458
        .word   1058067405
        .word   1057896922
        .word   1057725035
        .word   1057551771
        .word   1057377154
        .word   1057201213
        .word   1057023972
        .word   1056726311
        .word   1056366795
        .word   1056004842
        .word   1055640507
        .word   1055273845
        .word   1054904911
        .word   1054533760
        .word   1054160449
        .word   1053785034
        .word   1053407571
        .word   1053028117
        .word   1052646729
        .word   1052263466
        .word   1051878383
        .word   1051491540
        .word   1051102994
        .word   1050712805
        .word   1050321030
        .word   1049927730
        .word   1049532962
        .word   1049136787
        .word   1048739264
        .word   1048104908
        .word   1047304831
        .word   1046502419
        .word   1045697793
        .word   1044891074
        .word   1044082383
        .word   1043271842
        .word   1042459573
        .word   1041645699
        .word   1040830343
        .word   1039839859
        .word   1038203951
        .word   1036565814
        .word   1034925696
        .word   1033283844
        .word   1031482229
        .word   1028193070
        .word   1024901931
        .word   1019808433
        .word   1011420818
        .word   0
        .word   3158904466
        .word   3167292081
        .word   3172385579
        .word   3175676718
        .word   3178965877
        .word   3180767492
        .word   3182409344
        .word   3184049462
        .word   3185687599
        .word   3187323507
        .word   3188313991
        .word   3189129347
        .word   3189943221
        .word   3190755490
        .word   3191566031
        .word   3192374722
        .word   3193181441
        .word   3193986067
        .word   3194788479
        .word   3195588556
        .word   3196222912
        .word   3196620435
        .word   3197016610
        .word   3197411378
        .word   3197804678
        .word   3198196453
        .word   3198586642
        .word   3198975188
        .word   3199362031
        .word   3199747114
        .word   3200130377
        .word   3200511765
        .word   3200891219
        .word   3201268682
        .word   3201644097
        .word   3202017408
        .word   3202388559
        .word   3202757493
        .word   3203124155
        .word   3203488490
        .word   3203850443
        .word   3204209959
        .word   3204507620
        .word   3204684861
        .word   3204860802
        .word   3205035419
        .word   3205208683
        .word   3205380570
        .word   3205551053
        .word   3205720106
        .word   3205887705
        .word   3206053824
        .word   3206218438
        .word   3206381521
        .word   3206543051
        .word   3206703001
        .word   3206861349
        .word   3207018070
        .word   3207173141
        .word   3207326538
        .word   3207478238
        .word   3207628219
        .word   3207776457
        .word   3207922931
        .word   3208067619
        .word   3208210498
        .word   3208351547
        .word   3208490745
        .word   3208628071
        .word   3208763504
        .word   3208897025
        .word   3209028611
        .word   3209158245
        .word   3209285906
        .word   3209411576
        .word   3209535234
        .word   3209656864
        .word   3209776445
        .word   3209893961
        .word   3210009393
        .word   3210122725
        .word   3210233939
        .word   3210343018
        .word   3210449946
        .word   3210554707
        .word   3210657285
        .word   3210757665
        .word   3210855831
        .word   3210951770
        .word   3211045465
        .word   3211136904
        .word   3211226072
        .word   3211312956
        .word   3211397543
        .word   3211479820
        .word   3211559774
        .word   3211637395
        .word   3211712670
        .word   3211785587
        .word   3211856136
        .word   3211924306
        .word   3211990087
        .word   3212053469
        .word   3212114443
        .word   3212172998
        .word   3212229127
        .word   3212282821
        .word   3212334072
        .word   3212382872
        .word   3212429213
        .word   3212473090
        .word   3212514494
        .word   3212553421
        .word   3212589864
        .word   3212623817
        .word   3212655276
        .word   3212684236
        .word   3212710691
        .word   3212734640
        .word   3212756077
        .word   3212775000
        .word   3212791405
        .word   3212805291
        .word   3212816655
        .word   3212825495
        .word   3212831811
        .word   3212835601
        .word   3212836864
        .word   3212835601
        .word   3212831811
        .word   3212825495
        .word   3212816655
        .word   3212805291
        .word   3212791405
        .word   3212775000
        .word   3212756077
        .word   3212734640
        .word   3212710691
        .word   3212684236
        .word   3212655276
        .word   3212623817
        .word   3212589864
        .word   3212553421
        .word   3212514494
        .word   3212473090
        .word   3212429213
        .word   3212382872
        .word   3212334072
        .word   3212282821
        .word   3212229127
        .word   3212172998
        .word   3212114443
        .word   3212053469
        .word   3211990087
        .word   3211924306
        .word   3211856136
        .word   3211785587
        .word   3211712670
        .word   3211637395
        .word   3211559774
        .word   3211479820
        .word   3211397543
        .word   3211312956
        .word   3211226072
        .word   3211136904
        .word   3211045465
        .word   3210951770
        .word   3210855831
        .word   3210757665
        .word   3210657285
        .word   3210554707
        .word   3210449946
        .word   3210343018
        .word   3210233939
        .word   3210122725
        .word   3210009393
        .word   3209893961
        .word   3209776445
        .word   3209656864
        .word   3209535234
        .word   3209411576
        .word   3209285906
        .word   3209158245
        .word   3209028611
        .word   3208897025
        .word   3208763504
        .word   3208628071
        .word   3208490745
        .word   3208351547
        .word   3208210498
        .word   3208067619
        .word   3207922931
        .word   3207776457
        .word   3207628219
        .word   3207478238
        .word   3207326538
        .word   3207173141
        .word   3207018070
        .word   3206861349
        .word   3206703001
        .word   3206543051
        .word   3206381521
        .word   3206218438
        .word   3206053824
        .word   3205887705
        .word   3205720106
        .word   3205551053
        .word   3205380570
        .word   3205208683
        .word   3205035419
        .word   3204860802
        .word   3204684861
        .word   3204507620
        .word   3204209959
        .word   3203850443
        .word   3203488490
        .word   3203124155
        .word   3202757493
        .word   3202388559
        .word   3202017408
        .word   3201644097
        .word   3201268682
        .word   3200891219
        .word   3200511765
        .word   3200130377
        .word   3199747114
        .word   3199362031
        .word   3198975188
        .word   3198586642
        .word   3198196453
        .word   3197804678
        .word   3197411378
        .word   3197016610
        .word   3196620435
        .word   3196222912
        .word   3195588556
        .word   3194788479
        .word   3193986067
        .word   3193181441
        .word   3192374722
        .word   3191566031
        .word   3190755490
        .word   3189943221
        .word   3189129347
        .word   3188313991
        .word   3187323507
        .word   3185687599
        .word   3184049462
        .word   3182409344
        .word   3180767492
        .word   3178965877
        .word   3175676718
        .word   3172385579
        .word   3167292081
        .word   3158904466
        .word   2147483648
.LC0:
        .word   1042479491
.LC1:
        .word   1140850688
.LC2:
        .word   1065353216

.atan:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.reflect:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.pow:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.normalize1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.normalize2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.normalize3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.normalize4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.cos:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.log2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.exp:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.inversesqrt:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.asin:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.length1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.length2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.length3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.length4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.fract:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.cross:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.log:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.faceforward:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.acos:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.radians:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.degrees:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.exp2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.tan:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.atan2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.refract:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.distance:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.floor:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.step:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.dot1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.dot2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.dot3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.dot4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.any1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.any2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.any3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.any4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.all1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.all2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.all3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.all4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0
