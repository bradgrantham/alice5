// Created by altera_lib_lpm.pl from fp_compare.v
//VALID FILE


// synopsys translate_off
`timescale 1 ps / 1 ps
// synopsys translate_on
/*verilator lint_off CASEX*/
/*verilator lint_off COMBDLY*/
/*verilator lint_off INITIALDLY*/
/*verilator lint_off LITENDIAN*/
/*verilator lint_off MULTIDRIVEN*/
/*verilator lint_off UNOPTFLAT*/
/*verilator lint_off BLKANDNBLK*/
module fp_compare (
	clk_en,
	clock,
	dataa,
	datab,
	aeb,
	agb,
	alb);

	input	  clk_en;
	input	  clock;
	input	[31:0]  dataa;
	input	[31:0]  datab;
	output	  aeb;
	output	  agb;
	output	  alb;

	wire  sub_wire0;
	wire  sub_wire1;
	wire  sub_wire2;
	wire  aeb = sub_wire0;
	wire  agb = sub_wire1;
	wire  alb = sub_wire2;

	fp_compare_altfp_compare_q9c	fp_compare_altfp_compare_q9c_component (
				.clk_en (clk_en),
				.clock (clock),
				.dataa (dataa),
				.datab (datab),
				.aeb (sub_wire0),
				.agb (sub_wire1),
				.alb (sub_wire2));

endmodule

