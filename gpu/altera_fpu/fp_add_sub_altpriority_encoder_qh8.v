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
module  fp_add_sub_altpriority_encoder_qh8
	(
	data,
	q,
	zero) ;
	input   [3:0]  data;
	output   [1:0]  q;
	output   zero;

	wire  [0:0]   wire_altpriority_encoder27_q;
	wire  wire_altpriority_encoder27_zero;
	wire  [0:0]   wire_altpriority_encoder28_q;
	wire  wire_altpriority_encoder28_zero;

	fp_add_sub_altpriority_encoder_nh8   altpriority_encoder27
	(
	.data(data[1:0]),
	.q(wire_altpriority_encoder27_q),
	.zero(wire_altpriority_encoder27_zero));
	fp_add_sub_altpriority_encoder_nh8   altpriority_encoder28
	(
	.data(data[3:2]),
	.q(wire_altpriority_encoder28_q),
	.zero(wire_altpriority_encoder28_zero));
	assign
		q = {wire_altpriority_encoder27_zero, ((wire_altpriority_encoder27_zero & wire_altpriority_encoder28_q) | ((~ wire_altpriority_encoder27_zero) & wire_altpriority_encoder27_q))},
		zero = (wire_altpriority_encoder27_zero & wire_altpriority_encoder28_zero);
endmodule //fp_add_sub_altpriority_encoder_qh8

