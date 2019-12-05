// Created by altera_lib_lpm.pl from fp_add_sub.v

//synthesis_resources = reg 14
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
module  fp_add_sub_altpriority_encoder_dna
	(
	aclr,
	clk_en,
	clock,
	data,
	q) ;
	input   aclr;
	input   clk_en;
	input   clock;
	input   [31:0]  data;
	output   [4:0]  q;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
	logic   aclr; // -- converted tristate to logic
	logic   clk_en; // -- converted tristate to logic
	logic   clock; // -- converted tristate to logic
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

	wire  [3:0]   wire_altpriority_encoder21_q;
	wire  wire_altpriority_encoder21_zero;
	wire  [3:0]   wire_altpriority_encoder22_q;
	reg	[4:0]	pipeline_q_dffe;
	wire  [4:0]  tmp_q_wire;

	fp_add_sub_altpriority_encoder_d6b   altpriority_encoder21
	(
	.aclr(aclr),
	.clk_en(clk_en),
	.clock(clock),
	.data(data[15:0]),
	.q(wire_altpriority_encoder21_q),
	.zero(wire_altpriority_encoder21_zero));
	fp_add_sub_altpriority_encoder_ena   altpriority_encoder22
	(
	.aclr(aclr),
	.clk_en(clk_en),
	.clock(clock),
	.data(data[31:16]),
	.q(wire_altpriority_encoder22_q));
	// synopsys translate_off
	initial
		pipeline_q_dffe = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) pipeline_q_dffe <= 5'b0;
		else if  (clk_en == 1'b1)   pipeline_q_dffe <= (~ tmp_q_wire);
	assign
		q = (~ pipeline_q_dffe),
		tmp_q_wire = {wire_altpriority_encoder21_zero, (({4{wire_altpriority_encoder21_zero}} & wire_altpriority_encoder22_q) | ({4{(~ wire_altpriority_encoder21_zero)}} & wire_altpriority_encoder21_q))};
endmodule //fp_add_sub_altpriority_encoder_dna

