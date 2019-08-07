
; ------------------------------------------------------------------------
; library.s
;
; library of math functions

; some useful constants

.segment data
.one:
        .word   1065353216      ; 1.0

.point5:
        .word   1056964608      ; .5

.point25:
        .word   1048576000      ; 0x3e800000 = .25

.oneOverTwoPi:
        .word   1042479491      ; 1.0 / 2pi

.halfPi:
        .word   1070141403      ; 0x3fc90fdb = pi / 2

.pi:
        .word   1078530011      ; 0x40490fdb = pi

.segment text
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

.segment data
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
        
.atanTableSize:
        .word   1124073472      ; 0x43000000 = 128.0

atanTable_f32:
        .word   0 ; 0x00000000 = 0.000000
        .word   1006632619 ; 0x3BFFFEAB = 0.007812
        .word   1015020203 ; 0x3C7FFAAB = 0.015624
        .word   1019213569 ; 0x3CBFF701 = 0.023433
        .word   1023404718 ; 0x3CFFEAAE = 0.031240
        .word   1025502000 ; 0x3D1FEB30 = 0.039043
        .word   1027595276 ; 0x3D3FDC0C = 0.046841
        .word   1029687024 ; 0x3D5FC6F0 = 0.054633
        .word   1031776990 ; 0x3D7FAADE = 0.062419
        .word   1032831854 ; 0x3D8FC36E = 0.070197
        .word   1033874681 ; 0x3D9FACF9 = 0.077967
        .word   1034916243 ; 0x3DAF9193 = 0.085727
        .word   1035956417 ; 0x3DBF70C1 = 0.093477
        .word   1036995083 ; 0x3DCF4A0B = 0.101215
        .word   1038032118 ; 0x3DDF1CF6 = 0.108942
        .word   1039067404 ; 0x3DEEE90C = 0.116655
        .word   1040100821 ; 0x3DFEADD5 = 0.124355
        .word   1040659823 ; 0x3E07356F = 0.132040
        .word   1041174488 ; 0x3E0F0FD8 = 0.139709
        .word   1041688046 ; 0x3E16E5EE = 0.147361
        .word   1042200439 ; 0x3E1EB777 = 0.154997
        .word   1042711613 ; 0x3E26843D = 0.162614
        .word   1043221513 ; 0x3E2E4C09 = 0.170212
        .word   1043730084 ; 0x3E360EA4 = 0.177790
        .word   1044237274 ; 0x3E3DCBDA = 0.185348
        .word   1044743031 ; 0x3E458377 = 0.192884
        .word   1045247303 ; 0x3E4D3547 = 0.200399
        .word   1045750041 ; 0x3E54E119 = 0.207890
        .word   1046251195 ; 0x3E5C86BB = 0.215358
        .word   1046750716 ; 0x3E6425FC = 0.222801
        .word   1047248559 ; 0x3E6BBEAF = 0.230220
        .word   1047744676 ; 0x3E7350A4 = 0.237612
        .word   1048239024 ; 0x3E7ADBB0 = 0.244979
        .word   1048653778 ; 0x3E812FD2 = 0.252318
        .word   1048899117 ; 0x3E84EE2D = 0.259630
        .word   1049143506 ; 0x3E88A8D2 = 0.266913
        .word   1049386925 ; 0x3E8C5FAD = 0.274167
        .word   1049629355 ; 0x3E9012AB = 0.281392
        .word   1049870777 ; 0x3E93C1B9 = 0.288587
        .word   1050111172 ; 0x3E976CC4 = 0.295752
        .word   1050350522 ; 0x3E9B13BA = 0.302885
        .word   1050588809 ; 0x3E9EB689 = 0.309986
        .word   1050826018 ; 0x3EA25522 = 0.317056
        .word   1051062131 ; 0x3EA5EF73 = 0.324092
        .word   1051297133 ; 0x3EA9856D = 0.331096
        .word   1051531009 ; 0x3EAD1701 = 0.338066
        .word   1051763744 ; 0x3EB0A420 = 0.345002
        .word   1051995325 ; 0x3EB42CBD = 0.351904
        .word   1052225738 ; 0x3EB7B0CA = 0.358771
        .word   1052454970 ; 0x3EBB303A = 0.365602
        .word   1052683010 ; 0x3EBEAB02 = 0.372398
        .word   1052909846 ; 0x3EC22116 = 0.379159
        .word   1053135466 ; 0x3EC5926A = 0.385883
        .word   1053359860 ; 0x3EC8FEF4 = 0.392570
        .word   1053583018 ; 0x3ECC66AA = 0.399221
        .word   1053804931 ; 0x3ECFC983 = 0.405834
        .word   1054025590 ; 0x3ED32776 = 0.412410
        .word   1054244987 ; 0x3ED6807B = 0.418949
        .word   1054463113 ; 0x3ED9D489 = 0.425450
        .word   1054679962 ; 0x3EDD239A = 0.431912
        .word   1054895526 ; 0x3EE06DA6 = 0.438337
        .word   1055109801 ; 0x3EE3B2A9 = 0.444722
        .word   1055322778 ; 0x3EE6F29A = 0.451070
        .word   1055534455 ; 0x3EEA2D77 = 0.457378
        .word   1055744824 ; 0x3EED6338 = 0.463648
        .word   1055953884 ; 0x3EF093DC = 0.469878
        .word   1056161628 ; 0x3EF3BF5C = 0.476069
        .word   1056368055 ; 0x3EF6E5B7 = 0.482221
        .word   1056573160 ; 0x3EFA06E8 = 0.488334
        .word   1056776943 ; 0x3EFD22EF = 0.494407
        .word   1056972004 ; 0x3F001CE4 = 0.500441
        .word   1057072568 ; 0x3F01A5B8 = 0.506435
        .word   1057172469 ; 0x3F032BF5 = 0.512389
        .word   1057271704 ; 0x3F04AF98 = 0.518304
        .word   1057370275 ; 0x3F0630A3 = 0.524180
        .word   1057468180 ; 0x3F07AF14 = 0.530015
        .word   1057565421 ; 0x3F092AED = 0.535811
        .word   1057661997 ; 0x3F0AA42D = 0.541568
        .word   1057757908 ; 0x3F0C1AD4 = 0.547284
        .word   1057853156 ; 0x3F0D8EE4 = 0.552962
        .word   1057947741 ; 0x3F0F005D = 0.558599
        .word   1058041664 ; 0x3F106F40 = 0.564198
        .word   1058134927 ; 0x3F11DB8F = 0.569756
        .word   1058227530 ; 0x3F13454A = 0.575276
        .word   1058319475 ; 0x3F14AC73 = 0.580756
        .word   1058410763 ; 0x3F16110B = 0.586198
        .word   1058501396 ; 0x3F177314 = 0.591600
        .word   1058591376 ; 0x3F18D290 = 0.596963
        .word   1058680705 ; 0x3F1A2F81 = 0.602287
        .word   1058769384 ; 0x3F1B89E8 = 0.607573
        .word   1058857417 ; 0x3F1CE1C9 = 0.612820
        .word   1058944805 ; 0x3F1E3725 = 0.618029
        .word   1059031550 ; 0x3F1F89FE = 0.623199
        .word   1059117655 ; 0x3F20DA57 = 0.628332
        .word   1059203123 ; 0x3F222833 = 0.633426
        .word   1059287956 ; 0x3F237394 = 0.638482
        .word   1059372157 ; 0x3F24BC7D = 0.643501
        .word   1059455729 ; 0x3F2602F1 = 0.648482
        .word   1059538675 ; 0x3F2746F3 = 0.653426
        .word   1059620998 ; 0x3F288886 = 0.658333
        .word   1059702700 ; 0x3F29C7AC = 0.663203
        .word   1059783785 ; 0x3F2B0469 = 0.668036
        .word   1059864257 ; 0x3F2C3EC1 = 0.672833
        .word   1059944118 ; 0x3F2D76B6 = 0.677593
        .word   1060023372 ; 0x3F2EAC4C = 0.682317
        .word   1060102022 ; 0x3F2FDF86 = 0.687004
        .word   1060180072 ; 0x3F311068 = 0.691657
        .word   1060257526 ; 0x3F323EF6 = 0.696273
        .word   1060334386 ; 0x3F336B32 = 0.700854
        .word   1060410656 ; 0x3F349520 = 0.705400
        .word   1060486340 ; 0x3F35BCC4 = 0.709912
        .word   1060561442 ; 0x3F36E222 = 0.714388
        .word   1060635966 ; 0x3F38053E = 0.718830
        .word   1060709915 ; 0x3F39261B = 0.723238
        .word   1060783292 ; 0x3F3A44BC = 0.727611
        .word   1060856103 ; 0x3F3B6127 = 0.731951
        .word   1060928350 ; 0x3F3C7B5E = 0.736257
        .word   1061000037 ; 0x3F3D9365 = 0.740530
        .word   1061071169 ; 0x3F3EA941 = 0.744770
        .word   1061141750 ; 0x3F3FBCF6 = 0.748977
        .word   1061211782 ; 0x3F40CE86 = 0.753151
        .word   1061281270 ; 0x3F41DDF6 = 0.757293
        .word   1061350219 ; 0x3F42EB4B = 0.761403
        .word   1061418631 ; 0x3F43F687 = 0.765480
        .word   1061486512 ; 0x3F44FFB0 = 0.769526
        .word   1061553864 ; 0x3F4606C8 = 0.773541
        .word   1061620693 ; 0x3F470BD5 = 0.777524
        .word   1061687002 ; 0x3F480EDA = 0.781477
        .word   1061752795 ; 0x3F490FDB = 0.785398

.segment text
.atan_0_1:

        fsw     fa0, -4(sp)
        fsw     fa1, -8(sp)
        fsw     fa2, -12(sp)
        fsw     fa3, -16(sp)
        fsw     fa4, -20(sp)
        fsw     fa5, -24(sp)
        sw     a0, -28(sp)
        sw     a1, -32(sp)
        sw     a2, -36(sp)

        ; atan_0_1(float z) { }
        flw     fa0, 0(sp)              ; fa0 = z

        ; int i = z * TABLE_POW2;
	lui	a1,%hi(.atanTableSize)  ; a1 = hi part of address of TABLE_POW2
	flw	fa1,%lo(.atanTableSize)(a1)      ; fa1 = TABLE_POW2
        ; a1 is available after this line
        fmul.s    fa2, fa0, fa1         ; fa2 = z * TABLE_POW2
	fcvt.w.s        a0,fa2,rdn      ; a0 = ifloor(z * TABLE_POW2)
        ; fa1 is available after this line
        ; fa0 is available after this line

        ; float a = z * TABLE_POW2 - i;
        ; fa3 = a
        fcvt.s.w fa0,a0,rne             ; fa0 = float(a0)
        fsub.s fa3,fa2,fa0            ; fa3 = z * TABLE_POW2 - float(floori(z * TABLE_POW2))
        ; fa0 is available after this line
        ; fa2 is available after this line

        ; float lower = atanTable[i];
        lui     a1,%hi(atanTable_f32)   ; a1 = hi part of address of atanTable
        addi    a2,a1,%lo(atanTable_f32); a2 = atanTable
        ; a1 is available after this line

        slli    a1, a0, 2               ; a1 = ifloor(z * TABLE_POW2) * 4
        ; a0 is available after this line
        add     a0, a1, a2              ; a0 = ifloor(z * TABLE_POW2) * 4 + atanTable
        ; a1 is available after this line
        flw     fa4, 0(a0)              ; fa4 = lower = atanTable[z * TABLE_POW2]
        ; float higher = atanTable[i + 1];
        flw     fa5, 4(a0)              ; fa5 = higher = atanTable[z * TABLE_POW2 + 1]
        ; a0 is available after this line

        ; float f = lower * (1 - a) + higher * a;
	lui	a0,%hi(.one)            ; a0 = hi part of .one
	flw	fa0,%lo(.one)(a0)       ; fa0 = 1.0
        fsub.s  fa1,fa0,fa3             ; fa1 = 1.0 - a
        fmul.s  fa0,fa4,fa1             ; fa0 = lower * (1.0 - a)
        fmadd.s fa1,fa5,fa3,fa0         ; fa1 = higher * a + lower * (1.0 - a)

        ; return f;
        fsw     fa1, 0(sp)

        ; restore registers
        flw     fa0, -4(sp)
        flw     fa1, -8(sp)
        flw     fa2, -12(sp)
        flw     fa3, -16(sp)
        flw     fa4, -20(sp)
        flw     fa5, -24(sp)
        lw     a0, -28(sp)
        lw     a1, -32(sp)
        lw     a2, -36(sp)

        jalr x0, ra, 0

.atan:
        ; save registers
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        fsw     fa0, -12(sp)
        fsw     fa1, -16(sp)
        fsw     fa2, -20(sp)
        fsw     fa3, -24(sp)
        fsw     fa4, -28(sp)
        fsw     fa5, -32(sp)
        fsw     fa6, -36(sp)

        ; load parameter
        ; begin using fa0
        flw     fa0, 0(sp)      ; float atan(float y_x) {}

        addi    sp,sp,-32        ; push saved registers

        ; begin using fa2
        fsgnjx.s fa2, fa0, fa0     ; float fa2 = fabs_y_x = fabs(y_x)

        ; if(fabsf(y_x) > 1.0) {
	lui	a0,%hi(.one)
	flw	fa1,%lo(.one)(a0)       ; fa1 = 1.0
        ; begin using a1
        fle.s   a1,fa2,fa1              ; a1 = (fabs_y_x <= 1.0)
        bne     a1,zero,.atan_small_y_x ; if(fabs_y_x <= 1.0) goto atan_small_y_x
        ; a1 is available after this line

        ;     return copysign(M_PI / 2, y_x) + brad_atan_0_1(1.0 / fabs_y_x);
        ;     return copysign(M_PI / 2.0, y_x) + -copysign(brad_atan_0_1(1.0 / fabs(y_x)), y_x);
        ; begin using fa3
        fdiv.s  fa3, fa1, fa2

        addi    sp, sp, -8      ; Make room on stack
        sw      ra, 4(sp)       ; Save return address
        fsw     fa3, 0(sp)      ; Store parameter
        ; fa3 is available after this line

        jal     ra, .atan_0_1   

        ; begin using fa3
        flw     fa3, 0(sp)      ; Pop result
        lw      ra, 4(sp)       ; Restore return address
        addi    sp, sp, 8       ; Restore stack

	lui	a0,%hi(.halfPi)
        ; begin using fa4
	flw	fa4,%lo(.halfPi)(a0)
        ; begin using fa5
        fsgnj.s   fa5, fa4, fa0
        ; fa4 is available after this line

        ; begin using fa6
        fsgnjn.s fa6, fa3, fa0
        ; fa3 is available after this line

        ; begin using fa4
        fadd.s  fa4, fa5, fa6
        ; fa6 is available after this line
        ; fa5 is available after this line

        jal zero, .atan_finish   ; goto .atan_finish

.atan_small_y_x: ; } else {

        addi    sp, sp, -8      ; Make room on stack
        sw      ra, 4(sp)       ; Save return address
        fsw     fa2, 0(sp)      ; Store parameter
        ; fa2 is available after this line

        jal     ra, .atan_0_1   

        ; begin using fa2
        flw     fa2, 0(sp)      ; Pop result
        lw      ra, 4(sp)       ; Restore return address
        addi    sp, sp, 8       ; Restore stack

        ; begin using fa4
        fsgnj.s   fa4, fa2, fa0
        ; fa2 is available after this line
        ; fa0 is available after this line
        
.atan_finish:

        addi    sp,sp,32        ; pop saved registers

        ; store return value
        fsw     fa4, 0(sp)
        ; fa4 is available after this line

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        flw     fa0, -12(sp)
        flw     fa1, -16(sp)
        flw     fa2, -20(sp)
        flw     fa3, -24(sp)
        flw     fa4, -28(sp)
        flw     fa5, -32(sp)
        flw     fa6, -36(sp)

        ; return
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

.max:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.min:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.clamp:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.mix:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.smoothstep:
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
        ; cos(x) = sin(x + pi / 2), so copy .sin and add 1/4 in the middle after x has been
        ; normalized.
        ; Making .cos a wrapper around .sin adding pi/2 to the parameter made .cos 12 insns
        ; longer than .sin and added 12 insn words to the library.
        ; Copying .sin and adding pi/2 in the middle makes .cos 2 insns longer and adds 57 words
        ; to the library, so make a judgement call that adding 46 more words but only making
        ; .cos 3.6% slower and completely removing the cosine table is a good tradeoff.

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

        ; accomplish adding pi / 2 by adding 1/4 to the normalized angle in the fmadd below
	lui	a5,%hi(.point25)
	flw	fa3,%lo(.point25)(a5)

        ; fa2<u> = fa0<x> * fa1<1 / (2 * pi)> + fa3(1/4)
	lui	a5,%hi(.oneOverTwoPi)
	flw	fa1,%lo(.oneOverTwoPi)(a5)
        fmadd.s   fa2,fa0,fa1,fa3

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


.log2:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.exp:
        addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        jalr x0, ra, 0

.mod:
        ; save registers
        fsw     fa0,-4(sp)
        fsw     fa1,-8(sp)
        fsw     fa2,-12(sp)
        fsw     fa3,-16(sp)
        fsw     fa4,-20(sp)
        sw     a0,-24(sp)

        flw     fa0, 0(sp)              ; fa0 = x = popf()
        flw     fa1, 4(sp)              ; fa1 = y = popf()
        fdiv.s  fa2, fa0, fa1, rdn      ; fa2 = t1 = x/y;
	fcvt.w.s a0,fa2,rdn             ; a0 = i = floori(t1)
        fcvt.s.w fa4,a0,rne             ; fa4 = q = floorf(i)
        fmul.s  fa3, fa4, fa1           ; fa3 = t2 = q*y
        fsub.s  fa1, fa0, fa3           ; fa1 = r = x - t2 = x - q*y

        fsw     fa1, 4(sp)             ; pushf(r)

        ; restore registers
        flw     fa0,-4(sp)
        flw     fa1,-8(sp)
        flw     fa2,-12(sp)
        flw     fa3,-16(sp)
        flw     fa4,-20(sp)
        lw     a0,-24(sp)

        ; set stack pointer as if pops and pushes had occured
        addi    sp, sp, 4

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
        ; save registers
        fsw     fa0,-4(sp)
        fsw     fa1,-8(sp)
        fsw     fa2,-12(sp)
        fsw     fa3,-16(sp)

        flw     fa0, 0(sp)              ; fa0 = x = popf()
        fmul.s  fa1, fa0, fa0           ; fa1 = x * x
        flw     fa3, 4(sp)              ; fa3 = y = popf()
        fmadd.s fa2, fa3, fa3, fa1      ; fa2 = x * x + y * y

        fsqrt.s fa0, fa2                ; fa0 = d = sqrtf(x * x + y * y + z * z + w * w)

        fsw     fa0, 4(sp)             ; pushf(d) 

        ; restore registers
        flw     fa0,-4(sp)
        flw     fa1,-8(sp)
        flw     fa2,-12(sp)
        flw     fa3,-16(sp)

        ; set stack pointer as if pops and pushes had occured
        addi    sp, sp, 4

        jalr x0, ra, 0

.length3:
        ; save registers
        fsw     fa0,-4(sp)
        fsw     fa1,-8(sp)
        fsw     fa2,-12(sp)
        fsw     fa3,-16(sp)

        flw     fa0, 0(sp)              ; fa0 = x = popf()
        fmul.s  fa1, fa0, fa0           ; fa1 = x * x
        flw     fa3, 4(sp)              ; fa3 = y = popf()
        fmadd.s fa2, fa3, fa3, fa1      ; fa2 = x * x + y * y
        flw     fa0, 8(sp)              ; fa0 = z = popf()
        fmadd.s fa1, fa0, fa0, fa2      ; fa1 = x * x + y * y + z * z

        fsqrt.s fa3, fa1                ; fa0 = d = sqrtf(x * x + y * y + z * z + w * w)

        fsw     fa3, 8(sp)             ; pushf(d) 

        ; restore registers
        flw     fa0,-4(sp)
        flw     fa1,-8(sp)
        flw     fa2,-12(sp)
        flw     fa3,-16(sp)

        ; set stack pointer as if pops and pushes had occured
        addi    sp, sp, 8

        jalr x0, ra, 0

.length4:
        ; save registers
        fsw     fa0,-4(sp)
        fsw     fa1,-8(sp)
        fsw     fa2,-12(sp)
        fsw     fa3,-16(sp)

        flw     fa0, 0(sp)              ; fa0 = x = popf()
        fmul.s  fa1, fa0, fa0           ; fa1 = x * x
        flw     fa3, 4(sp)              ; fa3 = y = popf()
        fmadd.s fa2, fa3, fa3, fa1      ; fa2 = x * x + y * y
        flw     fa0, 8(sp)              ; fa0 = z = popf()
        fmadd.s fa1, fa0, fa0, fa2      ; fa1 = x * x + y * y + z * z
        flw     fa3, 12(sp)             ; fa3 = w = popf()
        fmadd.s fa2, fa3, fa3, fa1      ; fa2 = x * x + y * y + z * z + w * w

        fsqrt.s fa0, fa2                ; fa0 = d = sqrtf(x * x + y * y + z * z + w * w)

        fsw     fa0, 12(sp)             ; pushf(d) 

        ; restore registers
        flw     fa0,-4(sp)
        flw     fa1,-8(sp)
        flw     fa2,-12(sp)
        flw     fa3,-16(sp)

        ; set stack pointer as if pops and pushes had occured
        addi    sp, sp, 12

        jalr    x0, ra, 0                  ; return;

.fract:
        ; save registers
        sw      a0, -4(sp)
        fsw     fa0, -8(sp)
        fsw     fa1, -12(sp)
        fsw     fa2, -16(sp)

        flw      fa0, 0(sp)      ; a0 = x

        ; a0<wholei> = ifloorf(fa0<x>)
	fcvt.w.s a0,fa0,rdn

        ; fa1<fract> = fa0<x> - fa1<float(wholei)>
        fcvt.s.w fa1,a0,rtz
	fsub.s	fa2,fa0,fa1

        fsw      fa2, 0(sp)     ; return(x)

        ; restore registers
        lw      a0, -4(sp)
        flw     fa0, -8(sp)
        flw     fa1, -12(sp)
        flw     fa2, -16(sp)

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
        ; save registers
        fsw     fa0, -4(sp)
        fsw     fa1, -8(sp)
        fsw     fa2, -12(sp)
        fsw     fa3, -16(sp)
        fsw     fa4, -20(sp)
        fsw     fa5, -24(sp)
        sw      a0, -28(sp)
        sw      a1, -32(sp)

        flw     fa1, 0(sp)      ; fa1 = y
        flw     fa0, 4(sp)      ; fa0 = x
        addi    sp,sp,-32        ; push saved registers

        fdiv.s          fa3, fa1, fa0   ; z = y / x

        fcvt.s.w        fa2,zero,rtz    ; fa2 = 0.0
        flt.s           a1,fa0,fa2      ; a1 = (x < 0)
        bne             a1,zero,.atan2_neg_x     ; if(x < 0) goto .atan2_neg_x;

        addi    sp, sp, -8      ; Make room on stack
        sw      ra, 4(sp)       ; Save return address
        fsw     fa3, 0(sp)      ; Store parameter

        jal     ra, .atan       ; fa0 = a = atan(z)     ; XXX OPT could restore regs and jump to .atan

        flw     fa0, 0(sp)      ; Pop result
        lw      ra, 4(sp)       ; Restore return address
        addi    sp, sp, 8       ; Restore stack

        jal zero, .atan2_finish   ; goto .atan2_finish

.atan2_neg_x:
        ; x < 0

        fsgnjn.s fa4, fa3, fa1  ; float z2 = copysign(z, -y);

        addi    sp, sp, -8      ; Make room on stack
        sw      ra, 4(sp)       ; Save return address
        fsw     fa4, 0(sp)      ; Store parameter

        jal     ra, .atan       ; fa0 = a = atan(z2)

        flw     fa4, 0(sp)      ; Pop result
        lw      ra, 4(sp)       ; Restore return address
        addi    sp, sp, 8       ; Restore stack

        fsgnjn.s fa2, fa4, fa1  ; float a = copysign(brad_atan(z2), -y);

	lui	a0,%hi(.pi)
	flw	fa5,%lo(.pi)(a0)        ; float pi = 3.14159

        fsgnj.s fa3, fa5, fa1   ; sign_y_pi = copysign(pi, y)
        fadd.s  fa0, fa3, fa2   ; v = copysign(M_PI, y) + a;

.atan2_finish:

        addi    sp,sp,32        ; pop saved registers
        fsw     fa0, 4(sp)      ; store return value ("x")

        ; restore registers
        flw     fa0, -4(sp)
        flw     fa1, -8(sp)
        flw     fa2, -12(sp)
        flw     fa3, -16(sp)
        flw     fa4, -20(sp)
        flw     fa5, -24(sp)
        lw      a0, -28(sp)
        lw      a1, -32(sp)

        addi    sp, sp, 4       ; We took two parameters and return one, so fix up stack

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
        ; save registers
        sw      a0, -4(sp)
        fsw     fa0, -8(sp)
        fsw     fa1, -12(sp)

        flw      fa0, 0(sp)      ; a0 = x

        ; a0<wholei> = ifloorf(fa0<x>)
	fcvt.w.s a0,fa0,rdn

        ; fa1<fract> = fa0<x> - fa1<float(wholei)>
        fcvt.s.w fa1,a0,rtz

        fsw      fa1, 0(sp)     ; return(fract)

        ; restore registers
        lw      a0, -4(sp)
        flw     fa0, -8(sp)
        flw     fa1, -12(sp)

        jalr x0, ra, 0

.step:
        sw      a0, -4(sp)
        fsw     fa0, -8(sp)
        fsw     fa1, -12(sp)
        fsw     fa2, -16(sp)

        ; float edge = popf();
        flw      fa0, 0(sp)      ; fa0 = edge = pop()

        ; float x = popf();
        flw      fa1, 4(sp)      ; fa1 = x = pop()

        ; float y = (x < edge) ? 0.0f : 1.0f;
        fle.s     a0, fa0, fa1

        fcvt.s.w fa2,a0,rtz

        ; pushf(y);
        fsw     fa2, 4(sp)

        lw      a0, -4(sp)
        flw     fa0, -8(sp)
        flw     fa1, -12(sp)
        flw     fa2, -16(sp)

        addi    sp, sp, 4       ; We took two parameters and return one, so fix up stack

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
        ; addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        ; any(a) = a so just return
        jalr x0, ra, 0

.any2:
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        or a2, a0, a1  ; c = a || b
        sw a2, 4(sp)    ; return(c)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)

        addi    sp, sp, 4       ; We took two parameters and return one, so fix up stack
        jalr x0, ra, 0

.any3:
        ; save registers
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)
        sw      a3, -16(sp)
        sw      a4, -20(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        lw a2, 8(sp)    ; c
        or a4, a0, a1  ; e = a || b
        or a0, a2, a4  ; f = c || e
        sw a0, 8(sp)    ; return(f)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)
        lw      a3, -16(sp)
        lw      a4, -20(sp)

        addi    sp, sp, 8       ; We took three parameters and return one, so fix up stack
        jalr x0, ra, 0

.any4:
        ; save registers
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)
        sw      a3, -16(sp)
        sw      a4, -20(sp)
        sw      a5, -24(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        lw a2, 8(sp)    ; c
        lw a3, 12(sp)   ; d
        or a4, a0, a1  ; e = a || b
        or a5, a2, a3  ; f = c || d
        or a0, a4, a5  ; g = e || f
        sw a0, 12(sp)   ; return(g)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)
        lw      a3, -16(sp)
        lw      a4, -20(sp)
        lw      a5, -24(sp)

        addi    sp, sp, 12      ; We took four parameters and return one, so fix up stack
        jalr x0, ra, 0

.all1:
        ; addi x0, x0, 0  ; NOP caught by gpuemu; functionality will be proxied
        ; all(a) = a so just return
        jalr x0, ra, 0

.all2:
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        and a2, a0, a1  ; c = a && b
        sw a2, 4(sp)    ; return(c)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)

        addi    sp, sp, 4       ; We took two parameters and return one, so fix up stack
        jalr x0, ra, 0

.all3:
        ; save registers
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)
        sw      a3, -16(sp)
        sw      a4, -20(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        lw a2, 8(sp)    ; c
        and a4, a0, a1  ; e = a && b
        and a0, a2, a4  ; f = c && e
        sw a0, 8(sp)    ; return(f)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)
        lw      a3, -16(sp)
        lw      a4, -20(sp)

        addi    sp, sp, 8       ; We took three parameters and return one, so fix up stack
        jalr x0, ra, 0

.all4:
        ; save registers
        sw      a0, -4(sp)
        sw      a1, -8(sp)
        sw      a2, -12(sp)
        sw      a3, -16(sp)
        sw      a4, -20(sp)
        sw      a5, -24(sp)

        lw a0, 0(sp)    ; a
        lw a1, 4(sp)    ; b
        lw a2, 8(sp)    ; c
        lw a3, 12(sp)   ; d
        and a4, a0, a1  ; e = a && b
        and a5, a2, a3  ; f = c && d
        and a0, a4, a5  ; g = e && f
        sw a0, 12(sp)   ; return(g)

        ; restore registers
        lw      a0, -4(sp)
        lw      a1, -8(sp)
        lw      a2, -12(sp)
        lw      a3, -16(sp)
        lw      a4, -20(sp)
        lw      a5, -24(sp)

        addi    sp, sp, 12      ; We took four parameters and return one, so fix up stack
        jalr x0, ra, 0

