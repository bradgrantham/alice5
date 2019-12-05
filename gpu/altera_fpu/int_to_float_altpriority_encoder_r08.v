// Created by altera_lib_lpm.pl from int_to_float.v

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
module  int_to_float_altpriority_encoder_r08
	(
	data,
	q) ;
	input   [15:0]  data;
	output   [3:0]  q;

	wire  [2:0]   wire_altpriority_encoder17_q;
	wire  [2:0]   wire_altpriority_encoder18_q;
	wire  wire_altpriority_encoder18_zero;

	int_to_float_altpriority_encoder_bv7   altpriority_encoder17
	(
	.data(data[7:0]),
	.q(wire_altpriority_encoder17_q));
	int_to_float_altpriority_encoder_be8   altpriority_encoder18
	(
	.data(data[15:8]),
	.q(wire_altpriority_encoder18_q),
	.zero(wire_altpriority_encoder18_zero));
	assign
		q = {(~ wire_altpriority_encoder18_zero), (({3{wire_altpriority_encoder18_zero}} & wire_altpriority_encoder17_q) | ({3{(~ wire_altpriority_encoder18_zero)}} & wire_altpriority_encoder18_q))};
endmodule //int_to_float_altpriority_encoder_r08

