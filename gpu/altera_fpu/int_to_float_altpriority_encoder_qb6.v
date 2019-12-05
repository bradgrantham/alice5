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
module  int_to_float_altpriority_encoder_qb6
	(
	data,
	q) ;
	input   [31:0]  data;
	output   [4:0]  q;

	wire  [3:0]   wire_altpriority_encoder10_q;
	wire  wire_altpriority_encoder10_zero;
	wire  [3:0]   wire_altpriority_encoder9_q;

	int_to_float_altpriority_encoder_rf8   altpriority_encoder10
	(
	.data(data[31:16]),
	.q(wire_altpriority_encoder10_q),
	.zero(wire_altpriority_encoder10_zero));
	int_to_float_altpriority_encoder_r08   altpriority_encoder9
	(
	.data(data[15:0]),
	.q(wire_altpriority_encoder9_q));
	assign
		q = {(~ wire_altpriority_encoder10_zero), (({4{wire_altpriority_encoder10_zero}} & wire_altpriority_encoder9_q) | ({4{(~ wire_altpriority_encoder10_zero)}} & wire_altpriority_encoder10_q))};
endmodule //int_to_float_altpriority_encoder_qb6

