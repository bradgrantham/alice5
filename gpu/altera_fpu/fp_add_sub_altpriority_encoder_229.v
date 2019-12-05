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
module  fp_add_sub_altpriority_encoder_229
	(
	data,
	q) ;
	input   [7:0]  data;
	output   [2:0]  q;

	wire  [1:0]   wire_altpriority_encoder31_q;
	wire  wire_altpriority_encoder31_zero;
	wire  [1:0]   wire_altpriority_encoder32_q;

	fp_add_sub_altpriority_encoder_qh8   altpriority_encoder31
	(
	.data(data[3:0]),
	.q(wire_altpriority_encoder31_q),
	.zero(wire_altpriority_encoder31_zero));
	fp_add_sub_altpriority_encoder_q28   altpriority_encoder32
	(
	.data(data[7:4]),
	.q(wire_altpriority_encoder32_q));
	assign
		q = {wire_altpriority_encoder31_zero, (({2{wire_altpriority_encoder31_zero}} & wire_altpriority_encoder32_q) | ({2{(~ wire_altpriority_encoder31_zero)}} & wire_altpriority_encoder31_q))};
endmodule //fp_add_sub_altpriority_encoder_229

