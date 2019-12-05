// Created by altera_lib_lpm.pl from float_to_int.v
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
module float_to_int (
	clk_en,
	clock,
	dataa,
	result);

	input	  clk_en;
	input	  clock;
	input	[31:0]  dataa;
	output	[31:0]  result;

	wire [31:0] sub_wire0;
	wire [31:0] result = sub_wire0[31:0];

	float_to_int_altfp_convert_jen	float_to_int_altfp_convert_jen_component (
				.clk_en (clk_en),
				.clock (clock),
				.dataa (dataa),
				.result (sub_wire0));

endmodule

