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
module  fp_add_sub_altpriority_encoder_6v7
	(
	data,
	q) ;
	input   [3:0]  data;
	output   [1:0]  q;

	wire  [0:0]   wire_altpriority_encoder17_q;
	wire  [0:0]   wire_altpriority_encoder18_q;
	wire  wire_altpriority_encoder18_zero;

	fp_add_sub_altpriority_encoder_3v7   altpriority_encoder17
	(
	.data(data[1:0]),
	.q(wire_altpriority_encoder17_q));
	fp_add_sub_altpriority_encoder_3e8   altpriority_encoder18
	(
	.data(data[3:2]),
	.q(wire_altpriority_encoder18_q),
	.zero(wire_altpriority_encoder18_zero));
	assign
		q = {(~ wire_altpriority_encoder18_zero), ((wire_altpriority_encoder18_zero & wire_altpriority_encoder17_q) | ((~ wire_altpriority_encoder18_zero) & wire_altpriority_encoder18_q))};
endmodule //fp_add_sub_altpriority_encoder_6v7

