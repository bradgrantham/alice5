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
module  int_to_float_altpriority_encoder_6v7
	(
	data,
	q) ;
	input   [3:0]  data;
	output   [1:0]  q;

	wire  [0:0]   wire_altpriority_encoder21_q;
	wire  [0:0]   wire_altpriority_encoder22_q;
	wire  wire_altpriority_encoder22_zero;

	int_to_float_altpriority_encoder_3v7   altpriority_encoder21
	(
	.data(data[1:0]),
	.q(wire_altpriority_encoder21_q));
	int_to_float_altpriority_encoder_3e8   altpriority_encoder22
	(
	.data(data[3:2]),
	.q(wire_altpriority_encoder22_q),
	.zero(wire_altpriority_encoder22_zero));
	assign
		q = {(~ wire_altpriority_encoder22_zero), ((wire_altpriority_encoder22_zero & wire_altpriority_encoder21_q) | ((~ wire_altpriority_encoder22_zero) & wire_altpriority_encoder22_q))};
endmodule //int_to_float_altpriority_encoder_6v7

