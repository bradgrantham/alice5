// Created by altera_lib_lpm.pl from fp_add_sub.v

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
module  fp_add_sub_altpriority_encoder_bv7
	(
	data,
	q) ;
	input   [7:0]  data;
	output   [2:0]  q;

	wire  [1:0]   wire_altpriority_encoder15_q;
	wire  [1:0]   wire_altpriority_encoder16_q;
	wire  wire_altpriority_encoder16_zero;

	fp_add_sub_altpriority_encoder_6v7   altpriority_encoder15
	(
	.data(data[3:0]),
	.q(wire_altpriority_encoder15_q));
	fp_add_sub_altpriority_encoder_6e8   altpriority_encoder16
	(
	.data(data[7:4]),
	.q(wire_altpriority_encoder16_q),
	.zero(wire_altpriority_encoder16_zero));
	assign
		q = {(~ wire_altpriority_encoder16_zero), (({2{wire_altpriority_encoder16_zero}} & wire_altpriority_encoder15_q) | ({2{(~ wire_altpriority_encoder16_zero)}} & wire_altpriority_encoder16_q))};
endmodule //fp_add_sub_altpriority_encoder_bv7

