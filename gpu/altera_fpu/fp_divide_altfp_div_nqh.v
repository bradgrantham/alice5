// Created by altera_lib_lpm.pl from fp_divide.v

//synthesis_resources = altsyncram 1 lpm_add_sub 4 lpm_compare 1 lpm_mult 5 mux21 74 reg 847
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
module  fp_divide_altfp_div_nqh
	(
	clk_en,
	clock,
	dataa,
	datab,
	result) ;
	input   clk_en;
	input   clock;
	input   [31:0]  dataa;
	input   [31:0]  datab;
	output   [31:0]  result;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
	logic   clk_en; // -- converted tristate to logic
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

	wire  [31:0]   wire_altfp_div_pst1_result;
	wire aclr;

	fp_divide_altfp_div_pst_8qe   altfp_div_pst1
	(
	.aclr(aclr),
	.clk_en(clk_en),
	.clock(clock),
	.dataa(dataa),
	.datab(datab),
	.result(wire_altfp_div_pst1_result));
	assign
		aclr = 1'b0,
		result = wire_altfp_div_pst1_result;
endmodule //fp_divide_altfp_div_nqh

