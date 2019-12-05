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
module  int_to_float_altpriority_encoder_6e8
	(
	data,
	q,
	zero) ;
	input   [3:0]  data;
	output   [1:0]  q;
	output   zero;

	wire  [0:0]   wire_altpriority_encoder15_q;
	wire  wire_altpriority_encoder15_zero;
	wire  [0:0]   wire_altpriority_encoder16_q;
	wire  wire_altpriority_encoder16_zero;

	int_to_float_altpriority_encoder_3e8   altpriority_encoder15
	(
	.data(data[1:0]),
	.q(wire_altpriority_encoder15_q),
	.zero(wire_altpriority_encoder15_zero));
	int_to_float_altpriority_encoder_3e8   altpriority_encoder16
	(
	.data(data[3:2]),
	.q(wire_altpriority_encoder16_q),
	.zero(wire_altpriority_encoder16_zero));
	assign
		q = {(~ wire_altpriority_encoder16_zero), ((wire_altpriority_encoder16_zero & wire_altpriority_encoder15_q) | ((~ wire_altpriority_encoder16_zero) & wire_altpriority_encoder16_q))},
		zero = (wire_altpriority_encoder15_zero & wire_altpriority_encoder16_zero);
endmodule //int_to_float_altpriority_encoder_6e8

