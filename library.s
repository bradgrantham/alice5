; library of math functions

; some useful constants
.one:
        .word   1065353216      ; 1.0
.point5:
        .word   1056964608      ; .5
.oneOverTwoPi:
        .word   1042479491      ; 1.0 / 2pi

.sin:
        ; save registers here, e.g.
        sw      a0,-4(sp)
        sw      a1,-8(sp)
        sw      a2,-12(sp)
        sw      a3,-16(sp)
        sw      a4,-20(sp)
        sw      a5,-24(sp)
        fsw     fa0,-28(sp)
        fsw     fa1,-32(sp)
        fsw     fa2,-36(sp)
        fsw     fa3,-40(sp)
        fsw     fa4,-44(sp)
        fsw     fa5,-48(sp)
        fsw     fa6,-52(sp)

        flw     fa0, 0(sp)       ; load first parameter ("x"), doesn't have to be into fa0

        ; .oneOverTwoPi is 1/(2pi)
        ; .sinTableSize is 512.0
        ; .one is 1.0
        ; sinTable_f32 is 513 long

        ; fa2<u> = fa0<x> * fa1<1 / (2 * pi)>
	lui	a5,%hi(.oneOverTwoPi)
	flw	fa1,%lo(.oneOverTwoPi)(a5)
	fmul.s	fa2,fa0,fa1

        ; fa3<indexf> = fa2<u> * fa1<tablesize>
	lui	a5,%hi(.sinTableSize)
	flw	fa1,%lo(.sinTableSize)(a5)
	fmul.s	fa3,fa2,fa1

        ; a1<index> = ifloorf(fa3<indexf>)
	fcvt.w.s a1,fa3,rdn

        ; fa6<beta> = fa3<indexf> - fa4<float(index)>
        fcvt.s.w fa4,a1,rtz
	fsub.s	fa6,fa3,fa4

        ; fa4<alpha> = fa5<1.0f> - fa6<beta>
	lui	a5,%hi(.one)
	flw	fa5,%lo(.one)(a5)
        fsub.s fa4,fa5,fa6

        ; a2<lower> = a1<index> & imm<tablemask>
	andi	a2,a1,511

        ; a3<upper> = a2<lower> + imm<1>
        addi     a3,a2,1

        ; ; fa0<result> = table[a2<lower>] * fa4<alpha> + table[a3<upper>] * fa6<beta>
        ; a1 = table + a2 * 4
        lui     a5,%hi(sinTable_f32)
        addi    a5,a5,%lo(sinTable_f32)

        slli    a4,a2,2
        add    a1,a5,a4

        ; fa1 = *a1
        flw     fa1,0(a1)

        ; a4 = table + a3 * 4
        lui     a5,%hi(sinTable_f32)
        addi    a5,a5,%lo(sinTable_f32)
        slli    a1,a3,2
        add    a4,a5,a1

        ; fa2 = *a4
        flw     fa2,0(a4)

        ; fa0 = fa2 * fa6
        fmul.s    fa0,fa2,fa6

        ; fa0 = fa1 * fa4 + fa0
        fmadd.s   fa0,fa1,fa4,fa0

        ; XXX debugging - multiply by .5
	; lui	a5,%hi(.point5)
	; flw	fa1,%lo(.point5)(a5)
        ; fmul.s    fa0,fa0,fa1

        fsw     fa0, 0(sp)      ; store return value ("x"), doesn't have to be fa0
        ; restore registers here, e.g.
        lw      a0,-4(sp)
        lw      a1,-8(sp)
        lw      a2,-12(sp)
        lw      a3,-16(sp)
        lw      a4,-20(sp)
        lw      a5,-24(sp)
        flw     fa0,-28(sp)
        flw     fa1,-32(sp)
        flw     fa2,-36(sp)
        flw     fa3,-40(sp)
        flw     fa4,-44(sp)
        flw     fa5,-48(sp)
        flw     fa6,-52(sp)

        jalr x0, ra, 0                

.sinTableSize:
        .word   1140850688      ; 0x44000000 = 512.0
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
        
.atan:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.reflect1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.reflect2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.reflect3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.reflect4:
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
        ; save registers here, e.g.
        sw      a0,-4(sp)
        sw      a1,-8(sp)
        sw      a2,-12(sp)
        sw      a3,-16(sp)
        sw      a4,-20(sp)
        sw      a5,-24(sp)
        fsw     fa0,-28(sp)
        fsw     fa1,-32(sp)
        fsw     fa2,-36(sp)
        fsw     fa3,-40(sp)
        fsw     fa4,-44(sp)
        fsw     fa5,-48(sp)
        fsw     fa6,-52(sp)

        flw     fa0, 0(sp)       ; load first parameter ("x"), doesn't have to be into fa0

        ; .oneOverTwoPi is 1/(2pi)
        ; .cosTableSize is 512.0
        ; .one is 1.0
        ; cosTable_f32 is 513 long

        ; fa2<u> = fa0<x> * fa1<1 / (2 * pi)>
	lui	a5,%hi(.oneOverTwoPi)
	flw	fa1,%lo(.oneOverTwoPi)(a5)
	fmul.s	fa2,fa0,fa1

        ; fa3<indexf> = fa2<u> * fa1<tablesize>
	lui	a5,%hi(.cosTableSize)
	flw	fa1,%lo(.cosTableSize)(a5)
	fmul.s	fa3,fa2,fa1

        ; a1<index> = ifloorf(fa3<indexf>)
	fcvt.w.s a1,fa3,rdn

        ; fa6<beta> = fa3<indexf> - fa4<float(index)>
        fcvt.s.w fa4,a1,rtz
	fsub.s	fa6,fa3,fa4

        ; fa4<alpha> = fa5<1.0f> - fa6<beta>
	lui	a5,%hi(.one)
	flw	fa5,%lo(.one)(a5)
        fsub.s fa4,fa5,fa6

        ; a2<lower> = a1<index> & imm<tablemask>
	andi	a2,a1,511

        ; a3<upper> = a2<lower> + imm<1>
        addi     a3,a2,1

        ; ; fa0<result> = table[a2<lower>] * fa4<alpha> + table[a3<upper>] * fa6<beta>
        ; a1 = table + a2 * 4
        lui     a5,%hi(cosTable_f32)
        addi    a5,a5,%lo(cosTable_f32)

        slli    a4,a2,2
        add    a1,a5,a4

        ; fa1 = *a1
        flw     fa1,0(a1)

        ; a4 = table + a3 * 4
        lui     a5,%hi(cosTable_f32)
        addi    a5,a5,%lo(cosTable_f32)
        slli    a1,a3,2
        add    a4,a5,a1

        ; fa2 = *a4
        flw     fa2,0(a4)

        ; fa0 = fa2 * fa6
        fmul.s    fa0,fa2,fa6

        ; fa0 = fa1 * fa4 + fa0
        fmadd.s   fa0,fa1,fa4,fa0

        ; XXX debugging - multiply by .5
	; lui	a5,%hi(.point5)
	; flw	fa1,%lo(.point5)(a5)
        ; fmul.s    fa0,fa0,fa1

        fsw     fa0, 0(sp)      ; store return value ("x"), doesn't have to be fa0
        ; restore registers here, e.g.
        lw      a0,-4(sp)
        lw      a1,-8(sp)
        lw      a2,-12(sp)
        lw      a3,-16(sp)
        lw      a4,-20(sp)
        lw      a5,-24(sp)
        flw     fa0,-28(sp)
        flw     fa1,-32(sp)
        flw     fa2,-36(sp)
        flw     fa3,-40(sp)
        flw     fa4,-44(sp)
        flw     fa5,-48(sp)
        flw     fa6,-52(sp)

        jalr x0, ra, 0                

.cosTableSize:
        .word   1140850688      ; 0x44000000 = 512.0
cosTable_f32:
        .word   1065353216 ; 0x3F800000 = 1.000000
        .word   1065351953 ; 0x3F7FFB11 = 0.999925
        .word   1065348163 ; 0x3F7FEC43 = 0.999699
        .word   1065341847 ; 0x3F7FD397 = 0.999322
        .word   1065333007 ; 0x3F7FB10F = 0.998795
        .word   1065321643 ; 0x3F7F84AB = 0.998118
        .word   1065307757 ; 0x3F7F4E6D = 0.997290
        .word   1065291352 ; 0x3F7F0E58 = 0.996313
        .word   1065272429 ; 0x3F7EC46D = 0.995185
        .word   1065250992 ; 0x3F7E70B0 = 0.993907
        .word   1065227044 ; 0x3F7E1324 = 0.992480
        .word   1065200588 ; 0x3F7DABCC = 0.990903
        .word   1065171628 ; 0x3F7D3AAC = 0.989177
        .word   1065140169 ; 0x3F7CBFC9 = 0.987301
        .word   1065106216 ; 0x3F7C3B28 = 0.985278
        .word   1065069773 ; 0x3F7BACCD = 0.983105
        .word   1065030846 ; 0x3F7B14BE = 0.980785
        .word   1064989442 ; 0x3F7A7302 = 0.978317
        .word   1064945565 ; 0x3F79C79D = 0.975702
        .word   1064899224 ; 0x3F791298 = 0.972940
        .word   1064850424 ; 0x3F7853F8 = 0.970031
        .word   1064799173 ; 0x3F778BC5 = 0.966976
        .word   1064745479 ; 0x3F76BA07 = 0.963776
        .word   1064689350 ; 0x3F75DEC6 = 0.960431
        .word   1064630795 ; 0x3F74FA0B = 0.956940
        .word   1064569821 ; 0x3F740BDD = 0.953306
        .word   1064506439 ; 0x3F731447 = 0.949528
        .word   1064440658 ; 0x3F721352 = 0.945607
        .word   1064372488 ; 0x3F710908 = 0.941544
        .word   1064301939 ; 0x3F6FF573 = 0.937339
        .word   1064229022 ; 0x3F6ED89E = 0.932993
        .word   1064153747 ; 0x3F6DB293 = 0.928506
        .word   1064076126 ; 0x3F6C835E = 0.923880
        .word   1063996172 ; 0x3F6B4B0C = 0.919114
        .word   1063913895 ; 0x3F6A09A7 = 0.914210
        .word   1063829308 ; 0x3F68BF3C = 0.909168
        .word   1063742424 ; 0x3F676BD8 = 0.903989
        .word   1063653256 ; 0x3F660F88 = 0.898674
        .word   1063561817 ; 0x3F64AA59 = 0.893224
        .word   1063468122 ; 0x3F633C5A = 0.887640
        .word   1063372184 ; 0x3F61C598 = 0.881921
        .word   1063274017 ; 0x3F604621 = 0.876070
        .word   1063173638 ; 0x3F5EBE06 = 0.870087
        .word   1063071059 ; 0x3F5D2D53 = 0.863973
        .word   1062966298 ; 0x3F5B941A = 0.857729
        .word   1062859370 ; 0x3F59F26A = 0.851355
        .word   1062750291 ; 0x3F584853 = 0.844854
        .word   1062639077 ; 0x3F5695E5 = 0.838225
        .word   1062525745 ; 0x3F54DB31 = 0.831470
        .word   1062410313 ; 0x3F531849 = 0.824589
        .word   1062292797 ; 0x3F514D3D = 0.817585
        .word   1062173216 ; 0x3F4F7A20 = 0.810457
        .word   1062051586 ; 0x3F4D9F02 = 0.803208
        .word   1061927928 ; 0x3F4BBBF8 = 0.795837
        .word   1061802259 ; 0x3F49D113 = 0.788346
        .word   1061674597 ; 0x3F47DE65 = 0.780737
        .word   1061544964 ; 0x3F45E404 = 0.773010
        .word   1061413376 ; 0x3F43E200 = 0.765167
        .word   1061279856 ; 0x3F41D870 = 0.757209
        .word   1061144423 ; 0x3F3FC767 = 0.749136
        .word   1061007097 ; 0x3F3DAEF9 = 0.740951
        .word   1060867899 ; 0x3F3B8F3B = 0.732654
        .word   1060726850 ; 0x3F396842 = 0.724247
        .word   1060583971 ; 0x3F373A23 = 0.715731
        .word   1060439283 ; 0x3F3504F3 = 0.707107
        .word   1060292809 ; 0x3F32C8C9 = 0.698376
        .word   1060144571 ; 0x3F3085BB = 0.689541
        .word   1059994590 ; 0x3F2E3BDE = 0.680601
        .word   1059842890 ; 0x3F2BEB4A = 0.671559
        .word   1059689492 ; 0x3F299414 = 0.662416
        .word   1059534422 ; 0x3F273656 = 0.653173
        .word   1059377701 ; 0x3F24D225 = 0.643832
        .word   1059219353 ; 0x3F226799 = 0.634393
        .word   1059059403 ; 0x3F1FF6CB = 0.624860
        .word   1058897873 ; 0x3F1D7FD1 = 0.615232
        .word   1058734790 ; 0x3F1B02C6 = 0.605511
        .word   1058570176 ; 0x3F187FC0 = 0.595699
        .word   1058404057 ; 0x3F15F6D9 = 0.585798
        .word   1058236459 ; 0x3F13682B = 0.575808
        .word   1058067405 ; 0x3F10D3CD = 0.565732
        .word   1057896922 ; 0x3F0E39DA = 0.555570
        .word   1057725035 ; 0x3F0B9A6B = 0.545325
        .word   1057551771 ; 0x3F08F59B = 0.534998
        .word   1057377154 ; 0x3F064B82 = 0.524590
        .word   1057201214 ; 0x3F039C3E = 0.514103
        .word   1057023973 ; 0x3F00E7E5 = 0.503538
        .word   1056726311 ; 0x3EFC5D27 = 0.492898
        .word   1056366793 ; 0x3EF6E0C9 = 0.482184
        .word   1056004843 ; 0x3EF15AEB = 0.471397
        .word   1055640507 ; 0x3EEBCBBB = 0.460539
        .word   1055273844 ; 0x3EE63374 = 0.449611
        .word   1054904912 ; 0x3EE09250 = 0.438616
        .word   1054533761 ; 0x3EDAE881 = 0.427555
        .word   1054160449 ; 0x3ED53641 = 0.416430
        .word   1053785033 ; 0x3ECF7BC9 = 0.405241
        .word   1053407572 ; 0x3EC9B954 = 0.393992
        .word   1053028117 ; 0x3EC3EF15 = 0.382683
        .word   1052646728 ; 0x3EBE1D48 = 0.371317
        .word   1052263467 ; 0x3EB8442B = 0.359895
        .word   1051878383 ; 0x3EB263EF = 0.348419
        .word   1051491539 ; 0x3EAC7CD3 = 0.336890
        .word   1051102992 ; 0x3EA68F10 = 0.325310
        .word   1050712805 ; 0x3EA09AE5 = 0.313682
        .word   1050321030 ; 0x3E9AA086 = 0.302006
        .word   1049927728 ; 0x3E94A030 = 0.290285
        .word   1049532963 ; 0x3E8E9A23 = 0.278520
        .word   1049136787 ; 0x3E888E93 = 0.266713
        .word   1048739264 ; 0x3E827DC0 = 0.254866
        .word   1048104912 ; 0x3E78CFD0 = 0.242980
        .word   1047304833 ; 0x3E6C9A81 = 0.231058
        .word   1046502418 ; 0x3E605C12 = 0.219101
        .word   1045697790 ; 0x3E5414FE = 0.207111
        .word   1044891076 ; 0x3E47C5C4 = 0.195090
        .word   1044082383 ; 0x3E3B6ECF = 0.183040
        .word   1043271840 ; 0x3E2F10A0 = 0.170962
        .word   1042459577 ; 0x3E22ABB9 = 0.158858
        .word   1041645701 ; 0x3E164085 = 0.146730
        .word   1040830341 ; 0x3E09CF85 = 0.134581
        .word   1039839852 ; 0x3DFAB26C = 0.122411
        .word   1038203954 ; 0x3DE1BC32 = 0.110222
        .word   1036565813 ; 0x3DC8BD35 = 0.098017
        .word   1034925691 ; 0x3DAFB67B = 0.085797
        .word   1033283851 ; 0x3D96A90B = 0.073565
        .word   1031482231 ; 0x3D7B2B77 = 0.061321
        .word   1028193065 ; 0x3D48FB29 = 0.049068
        .word   1024901916 ; 0x3D16C31C = 0.036807
        .word   1019808446 ; 0x3CC90ABE = 0.024541
        .word   1011420806 ; 0x3C490E86 = 0.012272
        .word   3007036718 ; 0xB33BBD2E = -0.000000
        .word   3158904420 ; 0xBC490E64 = -0.012271
        .word   3167292077 ; 0xBCC90AAD = -0.024541
        .word   3172385587 ; 0xBD16C333 = -0.036807
        .word   3175676705 ; 0xBD48FB21 = -0.049068
        .word   3178965870 ; 0xBD7B2B6E = -0.061321
        .word   3180767494 ; 0xBD96A906 = -0.073565
        .word   3182409351 ; 0xBDAFB687 = -0.085797
        .word   3184049457 ; 0xBDC8BD31 = -0.098017
        .word   3185687598 ; 0xBDE1BC2E = -0.110222
        .word   3187323511 ; 0xBDFAB277 = -0.122411
        .word   3188313987 ; 0xBE09CF83 = -0.134581
        .word   3189129346 ; 0xBE164082 = -0.146730
        .word   3189943223 ; 0xBE22ABB7 = -0.158858
        .word   3190755494 ; 0xBE2F10A6 = -0.170962
        .word   3191566029 ; 0xBE3B6ECD = -0.183040
        .word   3192374722 ; 0xBE47C5C2 = -0.195090
        .word   3193181444 ; 0xBE541504 = -0.207111
        .word   3193986064 ; 0xBE605C10 = -0.219101
        .word   3194788478 ; 0xBE6C9A7E = -0.231058
        .word   3195588557 ; 0xBE78CFCD = -0.242980
        .word   3196222914 ; 0xBE827DC2 = -0.254866
        .word   3196620434 ; 0xBE888E92 = -0.266713
        .word   3197016610 ; 0xBE8E9A22 = -0.278520
        .word   3197411379 ; 0xBE94A033 = -0.290285
        .word   3197804677 ; 0xBE9AA085 = -0.302006
        .word   3198196452 ; 0xBEA09AE4 = -0.313682
        .word   3198586643 ; 0xBEA68F13 = -0.325310
        .word   3198975186 ; 0xBEAC7CD2 = -0.336890
        .word   3199362030 ; 0xBEB263EE = -0.348419
        .word   3199747114 ; 0xBEB8442A = -0.359895
        .word   3200130379 ; 0xBEBE1D4B = -0.371317
        .word   3200511764 ; 0xBEC3EF14 = -0.382683
        .word   3200891219 ; 0xBEC9B953 = -0.393992
        .word   3201268683 ; 0xBECF7BCB = -0.405241
        .word   3201644096 ; 0xBED53640 = -0.416430
        .word   3202017408 ; 0xBEDAE880 = -0.427555
        .word   3202388559 ; 0xBEE0924F = -0.438616
        .word   3202757494 ; 0xBEE63376 = -0.449611
        .word   3203124158 ; 0xBEEBCBBE = -0.460539
        .word   3203488486 ; 0xBEF15AE6 = -0.471397
        .word   3203850440 ; 0xBEF6E0C8 = -0.482184
        .word   3204209958 ; 0xBEFC5D26 = -0.492898
        .word   3204507620 ; 0xBF00E7E4 = -0.503538
        .word   3204684861 ; 0xBF039C3D = -0.514103
        .word   3204860803 ; 0xBF064B83 = -0.524590
        .word   3205035420 ; 0xBF08F59C = -0.534998
        .word   3205208682 ; 0xBF0B9A6A = -0.545325
        .word   3205380569 ; 0xBF0E39D9 = -0.555570
        .word   3205551052 ; 0xBF10D3CC = -0.565732
        .word   3205720106 ; 0xBF13682A = -0.575808
        .word   3205887706 ; 0xBF15F6DA = -0.585798
        .word   3206053825 ; 0xBF187FC1 = -0.595699
        .word   3206218439 ; 0xBF1B02C7 = -0.605511
        .word   3206381520 ; 0xBF1D7FD0 = -0.615232
        .word   3206543050 ; 0xBF1FF6CA = -0.624859
        .word   3206703001 ; 0xBF226799 = -0.634393
        .word   3206861349 ; 0xBF24D225 = -0.643832
        .word   3207018070 ; 0xBF273656 = -0.653173
        .word   3207173141 ; 0xBF299415 = -0.662416
        .word   3207326539 ; 0xBF2BEB4B = -0.671559
        .word   3207478237 ; 0xBF2E3BDD = -0.680601
        .word   3207628218 ; 0xBF3085BA = -0.689541
        .word   3207776457 ; 0xBF32C8C9 = -0.698376
        .word   3207922931 ; 0xBF3504F3 = -0.707107
        .word   3208067619 ; 0xBF373A23 = -0.715731
        .word   3208210499 ; 0xBF396843 = -0.724247
        .word   3208351548 ; 0xBF3B8F3C = -0.732654
        .word   3208490744 ; 0xBF3DAEF8 = -0.740951
        .word   3208628070 ; 0xBF3FC766 = -0.749136
        .word   3208763504 ; 0xBF41D870 = -0.757209
        .word   3208897025 ; 0xBF43E201 = -0.765167
        .word   3209028612 ; 0xBF45E404 = -0.773010
        .word   3209158246 ; 0xBF47DE66 = -0.780737
        .word   3209285907 ; 0xBF49D113 = -0.788346
        .word   3209411575 ; 0xBF4BBBF7 = -0.795837
        .word   3209535234 ; 0xBF4D9F02 = -0.803208
        .word   3209656863 ; 0xBF4F7A1F = -0.810457
        .word   3209776445 ; 0xBF514D3D = -0.817585
        .word   3209893961 ; 0xBF531849 = -0.824589
        .word   3210009394 ; 0xBF54DB32 = -0.831470
        .word   3210122724 ; 0xBF5695E4 = -0.838225
        .word   3210233938 ; 0xBF584852 = -0.844854
        .word   3210343018 ; 0xBF59F26A = -0.851355
        .word   3210449946 ; 0xBF5B941A = -0.857729
        .word   3210554707 ; 0xBF5D2D53 = -0.863973
        .word   3210657286 ; 0xBF5EBE06 = -0.870087
        .word   3210757666 ; 0xBF604622 = -0.876070
        .word   3210855831 ; 0xBF61C597 = -0.881921
        .word   3210951769 ; 0xBF633C59 = -0.887640
        .word   3211045465 ; 0xBF64AA59 = -0.893224
        .word   3211136904 ; 0xBF660F88 = -0.898674
        .word   3211226072 ; 0xBF676BD8 = -0.903989
        .word   3211312956 ; 0xBF68BF3C = -0.909168
        .word   3211397543 ; 0xBF6A09A7 = -0.914210
        .word   3211479819 ; 0xBF6B4B0B = -0.919114
        .word   3211559774 ; 0xBF6C835E = -0.923880
        .word   3211637395 ; 0xBF6DB293 = -0.928506
        .word   3211712670 ; 0xBF6ED89E = -0.932993
        .word   3211785587 ; 0xBF6FF573 = -0.937339
        .word   3211856137 ; 0xBF710909 = -0.941544
        .word   3211924307 ; 0xBF721353 = -0.945607
        .word   3211990087 ; 0xBF731447 = -0.949528
        .word   3212053469 ; 0xBF740BDD = -0.953306
        .word   3212114443 ; 0xBF74FA0B = -0.956940
        .word   3212172998 ; 0xBF75DEC6 = -0.960431
        .word   3212229127 ; 0xBF76BA07 = -0.963776
        .word   3212282821 ; 0xBF778BC5 = -0.966976
        .word   3212334072 ; 0xBF7853F8 = -0.970031
        .word   3212382871 ; 0xBF791297 = -0.972940
        .word   3212429213 ; 0xBF79C79D = -0.975702
        .word   3212473090 ; 0xBF7A7302 = -0.978317
        .word   3212514495 ; 0xBF7B14BF = -0.980785
        .word   3212553421 ; 0xBF7BACCD = -0.983105
        .word   3212589864 ; 0xBF7C3B28 = -0.985278
        .word   3212623817 ; 0xBF7CBFC9 = -0.987301
        .word   3212655276 ; 0xBF7D3AAC = -0.989177
        .word   3212684235 ; 0xBF7DABCB = -0.990903
        .word   3212710691 ; 0xBF7E1323 = -0.992480
        .word   3212734640 ; 0xBF7E70B0 = -0.993907
        .word   3212756077 ; 0xBF7EC46D = -0.995185
        .word   3212775000 ; 0xBF7F0E58 = -0.996313
        .word   3212791406 ; 0xBF7F4E6E = -0.997290
        .word   3212805291 ; 0xBF7F84AB = -0.998118
        .word   3212816655 ; 0xBF7FB10F = -0.998795
        .word   3212825495 ; 0xBF7FD397 = -0.999322
        .word   3212831811 ; 0xBF7FEC43 = -0.999699
        .word   3212835601 ; 0xBF7FFB11 = -0.999925
        .word   3212836864 ; 0xBF800000 = -1.000000
        .word   3212835601 ; 0xBF7FFB11 = -0.999925
        .word   3212831811 ; 0xBF7FEC43 = -0.999699
        .word   3212825496 ; 0xBF7FD398 = -0.999322
        .word   3212816655 ; 0xBF7FB10F = -0.998795
        .word   3212805291 ; 0xBF7F84AB = -0.998118
        .word   3212791405 ; 0xBF7F4E6D = -0.997290
        .word   3212775000 ; 0xBF7F0E58 = -0.996313
        .word   3212756077 ; 0xBF7EC46D = -0.995185
        .word   3212734640 ; 0xBF7E70B0 = -0.993907
        .word   3212710692 ; 0xBF7E1324 = -0.992480
        .word   3212684236 ; 0xBF7DABCC = -0.990903
        .word   3212655276 ; 0xBF7D3AAC = -0.989177
        .word   3212623817 ; 0xBF7CBFC9 = -0.987301
        .word   3212589864 ; 0xBF7C3B28 = -0.985278
        .word   3212553421 ; 0xBF7BACCD = -0.983105
        .word   3212514495 ; 0xBF7B14BF = -0.980785
        .word   3212473090 ; 0xBF7A7302 = -0.978317
        .word   3212429213 ; 0xBF79C79D = -0.975702
        .word   3212382872 ; 0xBF791298 = -0.972940
        .word   3212334072 ; 0xBF7853F8 = -0.970031
        .word   3212282821 ; 0xBF778BC5 = -0.966976
        .word   3212229128 ; 0xBF76BA08 = -0.963776
        .word   3212172999 ; 0xBF75DEC7 = -0.960431
        .word   3212114443 ; 0xBF74FA0B = -0.956940
        .word   3212053469 ; 0xBF740BDD = -0.953306
        .word   3211990087 ; 0xBF731447 = -0.949528
        .word   3211924306 ; 0xBF721352 = -0.945607
        .word   3211856136 ; 0xBF710908 = -0.941544
        .word   3211785588 ; 0xBF6FF574 = -0.937339
        .word   3211712670 ; 0xBF6ED89E = -0.932993
        .word   3211637395 ; 0xBF6DB293 = -0.928506
        .word   3211559774 ; 0xBF6C835E = -0.923880
        .word   3211479819 ; 0xBF6B4B0B = -0.919114
        .word   3211397542 ; 0xBF6A09A6 = -0.914210
        .word   3211312955 ; 0xBF68BF3B = -0.909168
        .word   3211226072 ; 0xBF676BD8 = -0.903989
        .word   3211136904 ; 0xBF660F88 = -0.898674
        .word   3211045465 ; 0xBF64AA59 = -0.893224
        .word   3210951770 ; 0xBF633C5A = -0.887640
        .word   3210855831 ; 0xBF61C597 = -0.881921
        .word   3210757665 ; 0xBF604621 = -0.876070
        .word   3210657284 ; 0xBF5EBE04 = -0.870087
        .word   3210554708 ; 0xBF5D2D54 = -0.863973
        .word   3210449947 ; 0xBF5B941B = -0.857729
        .word   3210343018 ; 0xBF59F26A = -0.851355
        .word   3210233939 ; 0xBF584853 = -0.844854
        .word   3210122724 ; 0xBF5695E4 = -0.838225
        .word   3210009392 ; 0xBF54DB30 = -0.831470
        .word   3209893962 ; 0xBF53184A = -0.824589
        .word   3209776446 ; 0xBF514D3E = -0.817585
        .word   3209656864 ; 0xBF4F7A20 = -0.810457
        .word   3209535234 ; 0xBF4D9F02 = -0.803208
        .word   3209411575 ; 0xBF4BBBF7 = -0.795837
        .word   3209285906 ; 0xBF49D112 = -0.788346
        .word   3209158244 ; 0xBF47DE64 = -0.780737
        .word   3209028613 ; 0xBF45E405 = -0.773011
        .word   3208897025 ; 0xBF43E201 = -0.765167
        .word   3208763505 ; 0xBF41D871 = -0.757209
        .word   3208628071 ; 0xBF3FC767 = -0.749136
        .word   3208490745 ; 0xBF3DAEF9 = -0.740951
        .word   3208351546 ; 0xBF3B8F3A = -0.732654
        .word   3208210497 ; 0xBF396841 = -0.724247
        .word   3208067620 ; 0xBF373A24 = -0.715731
        .word   3207922932 ; 0xBF3504F4 = -0.707107
        .word   3207776458 ; 0xBF32C8CA = -0.698376
        .word   3207628219 ; 0xBF3085BB = -0.689541
        .word   3207478238 ; 0xBF2E3BDE = -0.680601
        .word   3207326537 ; 0xBF2BEB49 = -0.671559
        .word   3207173139 ; 0xBF299413 = -0.662416
        .word   3207018071 ; 0xBF273657 = -0.653173
        .word   3206861350 ; 0xBF24D226 = -0.643832
        .word   3206703002 ; 0xBF22679A = -0.634393
        .word   3206543051 ; 0xBF1FF6CB = -0.624860
        .word   3206381521 ; 0xBF1D7FD1 = -0.615232
        .word   3206218437 ; 0xBF1B02C5 = -0.605511
        .word   3206053822 ; 0xBF187FBE = -0.595699
        .word   3205887703 ; 0xBF15F6D7 = -0.585798
        .word   3205720104 ; 0xBF136828 = -0.575808
        .word   3205551050 ; 0xBF10D3CA = -0.565732
        .word   3205380573 ; 0xBF0E39DD = -0.555570
        .word   3205208686 ; 0xBF0B9A6E = -0.545325
        .word   3205035421 ; 0xBF08F59D = -0.534998
        .word   3204860804 ; 0xBF064B84 = -0.524590
        .word   3204684862 ; 0xBF039C3E = -0.514103
        .word   3204507621 ; 0xBF00E7E5 = -0.503538
        .word   3204209959 ; 0xBEFC5D27 = -0.492898
        .word   3203850442 ; 0xBEF6E0CA = -0.482184
        .word   3203488488 ; 0xBEF15AE8 = -0.471397
        .word   3203124152 ; 0xBEEBCBB8 = -0.460539
        .word   3202757489 ; 0xBEE63371 = -0.449611
        .word   3202388554 ; 0xBEE0924A = -0.438616
        .word   3202017403 ; 0xBEDAE87B = -0.427555
        .word   3201644091 ; 0xBED5363B = -0.416429
        .word   3201268689 ; 0xBECF7BD1 = -0.405242
        .word   3200891225 ; 0xBEC9B959 = -0.393992
        .word   3200511770 ; 0xBEC3EF1A = -0.382684
        .word   3200130381 ; 0xBEBE1D4D = -0.371317
        .word   3199747116 ; 0xBEB8442C = -0.359895
        .word   3199362032 ; 0xBEB263F0 = -0.348419
        .word   3198975188 ; 0xBEAC7CD4 = -0.336890
        .word   3198586641 ; 0xBEA68F11 = -0.325310
        .word   3198196451 ; 0xBEA09AE3 = -0.313682
        .word   3197804675 ; 0xBE9AA083 = -0.302006
        .word   3197411373 ; 0xBE94A02D = -0.290285
        .word   3197016605 ; 0xBE8E9A1D = -0.278520
        .word   3196620429 ; 0xBE888E8D = -0.266713
        .word   3196222905 ; 0xBE827DB9 = -0.254865
        .word   3195588569 ; 0xBE78CFD9 = -0.242980
        .word   3194788490 ; 0xBE6C9A8A = -0.231058
        .word   3193986076 ; 0xBE605C1C = -0.219101
        .word   3193181448 ; 0xBE541508 = -0.207111
        .word   3192374726 ; 0xBE47C5C6 = -0.195090
        .word   3191566033 ; 0xBE3B6ED1 = -0.183040
        .word   3190755490 ; 0xBE2F10A2 = -0.170962
        .word   3189943219 ; 0xBE22ABB3 = -0.158858
        .word   3189129343 ; 0xBE16407F = -0.146730
        .word   3188313983 ; 0xBE09CF7F = -0.134581
        .word   3187323488 ; 0xBDFAB260 = -0.122411
        .word   3185687575 ; 0xBDE1BC17 = -0.110222
        .word   3184049434 ; 0xBDC8BD1A = -0.098017
        .word   3182409375 ; 0xBDAFB69F = -0.085798
        .word   3180767519 ; 0xBD96A91F = -0.073565
        .word   3178965919 ; 0xBD7B2B9F = -0.061321
        .word   3175676754 ; 0xBD48FB52 = -0.049068
        .word   3172385604 ; 0xBD16C344 = -0.036807
        .word   3167292111 ; 0xBCC90ACF = -0.024541
        .word   3158904488 ; 0xBC490EA8 = -0.012272
        .word   843898414 ; 0x324CDE2E = 0.000000
        .word   1011420866 ; 0x3C490EC2 = 0.012272
        .word   1019808475 ; 0x3CC90ADB = 0.024541
        .word   1024901963 ; 0x3D16C34B = 0.036807
        .word   1028193112 ; 0x3D48FB58 = 0.049068
        .word   1031482278 ; 0x3D7B2BA6 = 0.061321
        .word   1033283874 ; 0x3D96A922 = 0.073565
        .word   1034925667 ; 0x3DAFB663 = 0.085797
        .word   1036565789 ; 0x3DC8BD1D = 0.098017
        .word   1038203930 ; 0x3DE1BC1A = 0.110222
        .word   1039839843 ; 0x3DFAB263 = 0.122411
        .word   1040830337 ; 0x3E09CF81 = 0.134581
        .word   1041645696 ; 0x3E164080 = 0.146730
        .word   1042459573 ; 0x3E22ABB5 = 0.158858
        .word   1043271844 ; 0x3E2F10A4 = 0.170962
        .word   1044082387 ; 0x3E3B6ED3 = 0.183040
        .word   1044891080 ; 0x3E47C5C8 = 0.195090
        .word   1045697801 ; 0x3E541509 = 0.207111
        .word   1046502430 ; 0x3E605C1E = 0.219101
        .word   1047304844 ; 0x3E6C9A8C = 0.231058
        .word   1048104923 ; 0x3E78CFDB = 0.242980
        .word   1048739258 ; 0x3E827DBA = 0.254865
        .word   1049136781 ; 0x3E888E8D = 0.266713
        .word   1049532957 ; 0x3E8E9A1D = 0.278520
        .word   1049927726 ; 0x3E94A02E = 0.290285
        .word   1050321028 ; 0x3E9AA084 = 0.302006
        .word   1050712803 ; 0x3EA09AE3 = 0.313682
        .word   1051102994 ; 0x3EA68F12 = 0.325310
        .word   1051491541 ; 0x3EAC7CD5 = 0.336890
        .word   1051878385 ; 0x3EB263F1 = 0.348419
        .word   1052263469 ; 0x3EB8442D = 0.359895
        .word   1052646734 ; 0x3EBE1D4E = 0.371317
        .word   1053028123 ; 0x3EC3EF1B = 0.382684
        .word   1053407577 ; 0x3EC9B959 = 0.393992
        .word   1053785027 ; 0x3ECF7BC3 = 0.405241
        .word   1054160443 ; 0x3ED5363B = 0.416429
        .word   1054533755 ; 0x3EDAE87B = 0.427555
        .word   1054904907 ; 0x3EE0924B = 0.438616
        .word   1055273842 ; 0x3EE63372 = 0.449611
        .word   1055640505 ; 0x3EEBCBB9 = 0.460539
        .word   1056004841 ; 0x3EF15AE9 = 0.471397
        .word   1056366795 ; 0x3EF6E0CB = 0.482184
        .word   1056726312 ; 0x3EFC5D28 = 0.492898
        .word   1057023973 ; 0x3F00E7E5 = 0.503538
        .word   1057201214 ; 0x3F039C3E = 0.514103
        .word   1057377157 ; 0x3F064B85 = 0.524590
        .word   1057551773 ; 0x3F08F59D = 0.534998
        .word   1057725038 ; 0x3F0B9A6E = 0.545325
        .word   1057896919 ; 0x3F0E39D7 = 0.555570
        .word   1058067402 ; 0x3F10D3CA = 0.565732
        .word   1058236456 ; 0x3F136828 = 0.575808
        .word   1058404056 ; 0x3F15F6D8 = 0.585798
        .word   1058570175 ; 0x3F187FBF = 0.595699
        .word   1058734789 ; 0x3F1B02C5 = 0.605511
        .word   1058897873 ; 0x3F1D7FD1 = 0.615232
        .word   1059059403 ; 0x3F1FF6CB = 0.624860
        .word   1059219354 ; 0x3F22679A = 0.634393
        .word   1059377702 ; 0x3F24D226 = 0.643832
        .word   1059534423 ; 0x3F273657 = 0.653173
        .word   1059689495 ; 0x3F299417 = 0.662416
        .word   1059842892 ; 0x3F2BEB4C = 0.671559
        .word   1059994593 ; 0x3F2E3BE1 = 0.680601
        .word   1060144568 ; 0x3F3085B8 = 0.689540
        .word   1060292807 ; 0x3F32C8C7 = 0.698376
        .word   1060439281 ; 0x3F3504F1 = 0.707107
        .word   1060583969 ; 0x3F373A21 = 0.715731
        .word   1060726849 ; 0x3F396841 = 0.724247
        .word   1060867899 ; 0x3F3B8F3B = 0.732654
        .word   1061007097 ; 0x3F3DAEF9 = 0.740951
        .word   1061144423 ; 0x3F3FC767 = 0.749136
        .word   1061279857 ; 0x3F41D871 = 0.757209
        .word   1061413378 ; 0x3F43E202 = 0.765167
        .word   1061544965 ; 0x3F45E405 = 0.773011
        .word   1061674599 ; 0x3F47DE67 = 0.780737
        .word   1061802260 ; 0x3F49D114 = 0.788347
        .word   1061927930 ; 0x3F4BBBFA = 0.795837
        .word   1062051584 ; 0x3F4D9F00 = 0.803207
        .word   1062173214 ; 0x3F4F7A1E = 0.810457
        .word   1062292796 ; 0x3F514D3C = 0.817585
        .word   1062410312 ; 0x3F531848 = 0.824589
        .word   1062525745 ; 0x3F54DB31 = 0.831470
        .word   1062639077 ; 0x3F5695E5 = 0.838225
        .word   1062750291 ; 0x3F584853 = 0.844854
        .word   1062859370 ; 0x3F59F26A = 0.851355
        .word   1062966299 ; 0x3F5B941B = 0.857729
        .word   1063071060 ; 0x3F5D2D54 = 0.863973
        .word   1063173639 ; 0x3F5EBE07 = 0.870087
        .word   1063274019 ; 0x3F604623 = 0.876070
        .word   1063372185 ; 0x3F61C599 = 0.881921
        .word   1063468120 ; 0x3F633C58 = 0.887640
        .word   1063561816 ; 0x3F64AA58 = 0.893224
        .word   1063653254 ; 0x3F660F86 = 0.898674
        .word   1063742423 ; 0x3F676BD7 = 0.903989
        .word   1063829307 ; 0x3F68BF3B = 0.909168
        .word   1063913894 ; 0x3F6A09A6 = 0.914210
        .word   1063996171 ; 0x3F6B4B0B = 0.919114
        .word   1064076127 ; 0x3F6C835F = 0.923880
        .word   1064153747 ; 0x3F6DB293 = 0.928506
        .word   1064229022 ; 0x3F6ED89E = 0.932993
        .word   1064301940 ; 0x3F6FF574 = 0.937339
        .word   1064372489 ; 0x3F710909 = 0.941544
        .word   1064440659 ; 0x3F721353 = 0.945607
        .word   1064506441 ; 0x3F731449 = 0.949528
        .word   1064569820 ; 0x3F740BDC = 0.953306
        .word   1064630794 ; 0x3F74FA0A = 0.956940
        .word   1064689350 ; 0x3F75DEC6 = 0.960431
        .word   1064745479 ; 0x3F76BA07 = 0.963776
        .word   1064799173 ; 0x3F778BC5 = 0.966976
        .word   1064850424 ; 0x3F7853F8 = 0.970031
        .word   1064899224 ; 0x3F791298 = 0.972940
        .word   1064945565 ; 0x3F79C79D = 0.975702
        .word   1064989442 ; 0x3F7A7302 = 0.978317
        .word   1065030847 ; 0x3F7B14BF = 0.980785
        .word   1065069774 ; 0x3F7BACCE = 0.983106
        .word   1065106216 ; 0x3F7C3B28 = 0.985278
        .word   1065140170 ; 0x3F7CBFCA = 0.987301
        .word   1065171629 ; 0x3F7D3AAD = 0.989177
        .word   1065200587 ; 0x3F7DABCB = 0.990903
        .word   1065227043 ; 0x3F7E1323 = 0.992480
        .word   1065250992 ; 0x3F7E70B0 = 0.993907
        .word   1065272429 ; 0x3F7EC46D = 0.995185
        .word   1065291352 ; 0x3F7F0E58 = 0.996313
        .word   1065307757 ; 0x3F7F4E6D = 0.997290
        .word   1065321643 ; 0x3F7F84AB = 0.998118
        .word   1065333007 ; 0x3F7FB10F = 0.998795
        .word   1065341848 ; 0x3F7FD398 = 0.999322
        .word   1065348163 ; 0x3F7FEC43 = 0.999699
        .word   1065351953 ; 0x3F7FFB11 = 0.999925
        .word   1065353216 ; 0x3F800000 = 1.000000

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

.faceforward1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.faceforward2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.faceforward3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.faceforward4:
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

.refract1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.refract2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.refract3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.refract4:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.distance1:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.distance2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.distance3:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.distance4:
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
