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
module  fp_add_sub_altpriority_encoder_be8
	(
	data,
	q,
	zero) ;
	input   [7:0]  data;
	output   [2:0]  q;
	output   zero;

	wire  [1:0]   wire_altpriority_encoder11_q;
	wire  wire_altpriority_encoder11_zero;
	wire  [1:0]   wire_altpriority_encoder12_q;
	wire  wire_altpriority_encoder12_zero;

	fp_add_sub_altpriority_encoder_6e8   altpriority_encoder11
	(
	.data(data[3:0]),
	.q(wire_altpriority_encoder11_q),
	.zero(wire_altpriority_encoder11_zero));
	fp_add_sub_altpriority_encoder_6e8   altpriority_encoder12
	(
	.data(data[7:4]),
	.q(wire_altpriority_encoder12_q),
	.zero(wire_altpriority_encoder12_zero));
	assign
		q = {(~ wire_altpriority_encoder12_zero), (({2{wire_altpriority_encoder12_zero}} & wire_altpriority_encoder11_q) | ({2{(~ wire_altpriority_encoder12_zero)}} & wire_altpriority_encoder12_q))},
		zero = (wire_altpriority_encoder11_zero & wire_altpriority_encoder12_zero);
endmodule //fp_add_sub_altpriority_encoder_be8

