// Created by altera_lib_lpm.pl from fp_add_sub.v


//altpriority_encoder CBX_AUTO_BLACKBOX="ALL" LSB_PRIORITY="NO" PIPELINE=0 WIDTH=16 WIDTHAD=4 data q zero
//VERSION_BEGIN 18.1 cbx_altpriority_encoder 2018:09:12:13:04:09:SJ cbx_mgl 2018:09:12:14:15:07:SJ  VERSION_END

//synthesis_resources =
//synopsys translate_off
`timescale 1 ps / 1 ps
//synopsys translate_on
/*verilator lint_off CASEX*/
/*verilator lint_off COMBDLY*/
/*verilator lint_off INITIALDLY*/
/*verilator lint_off LITENDIAN*/
/*verilator lint_off MULTIDRIVEN*/
/*verilator lint_off UNOPTFLAT*/
/*verilator lint_off BLKANDNBLK*/
module  fp_add_sub_altpriority_encoder_ue9
	(
	data,
	q,
	zero) ;
	input   [15:0]  data;
	output   [3:0]  q;
	output   zero;

	wire  [2:0]   wire_altpriority_encoder19_q;
	wire  wire_altpriority_encoder19_zero;
	wire  [2:0]   wire_altpriority_encoder20_q;
	wire  wire_altpriority_encoder20_zero;

	fp_add_sub_altpriority_encoder_be8   altpriority_encoder19
	(
	.data(data[7:0]),
	.q(wire_altpriority_encoder19_q),
	.zero(wire_altpriority_encoder19_zero));
	fp_add_sub_altpriority_encoder_be8   altpriority_encoder20
	(
	.data(data[15:8]),
	.q(wire_altpriority_encoder20_q),
	.zero(wire_altpriority_encoder20_zero));
	assign
		q = {(~ wire_altpriority_encoder20_zero), (({3{wire_altpriority_encoder20_zero}} & wire_altpriority_encoder19_q) | ({3{(~ wire_altpriority_encoder20_zero)}} & wire_altpriority_encoder20_q))},
		zero = (wire_altpriority_encoder19_zero & wire_altpriority_encoder20_zero);
endmodule //fp_add_sub_altpriority_encoder_ue9

