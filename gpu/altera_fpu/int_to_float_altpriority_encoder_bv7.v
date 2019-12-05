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
module  int_to_float_altpriority_encoder_bv7
	(
	data,
	q) ;
	input   [7:0]  data;
	output   [2:0]  q;

	wire  [1:0]   wire_altpriority_encoder19_q;
	wire  [1:0]   wire_altpriority_encoder20_q;
	wire  wire_altpriority_encoder20_zero;

	int_to_float_altpriority_encoder_6v7   altpriority_encoder19
	(
	.data(data[3:0]),
	.q(wire_altpriority_encoder19_q));
	int_to_float_altpriority_encoder_6e8   altpriority_encoder20
	(
	.data(data[7:4]),
	.q(wire_altpriority_encoder20_q),
	.zero(wire_altpriority_encoder20_zero));
	assign
		q = {(~ wire_altpriority_encoder20_zero), (({2{wire_altpriority_encoder20_zero}} & wire_altpriority_encoder19_q) | ({2{(~ wire_altpriority_encoder20_zero)}} & wire_altpriority_encoder20_q))};
endmodule //int_to_float_altpriority_encoder_bv7

