// Created by altera_lib_lpm.pl from int_to_float.v

//synthesis_resources = lpm_add_sub 5 lpm_compare 1 reg 247
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
module  int_to_float_altfp_convert_jen
	(
	clk_en,
	clock,
	dataa,
	result) ;
	input   clk_en;
	input   clock;
	input   [31:0]  dataa;
	output   [31:0]  result;
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_off
`endif
	logic   clk_en; // -- converted tristate to logic
`ifndef ALTERA_RESERVED_QIS
// synopsys translate_on
`endif

	wire  [31:0]   wire_altbarrel_shift5_result;
	wire  [4:0]   wire_altpriority_encoder2_q;
	reg	add_1_adder1_cout_reg;
	reg	[11:0]	add_1_adder1_reg;
	reg	add_1_adder2_cout_reg;
	reg	[11:0]	add_1_adder2_reg;
	reg	add_1_reg;
	reg	[7:0]	exponent_bus_pre_reg;
	reg	[7:0]	exponent_bus_pre_reg2;
	reg	[7:0]	exponent_bus_pre_reg3;
	reg	[30:0]	mag_int_a_reg;
	reg	[30:0]	mag_int_a_reg2;
	reg	[23:0]	mantissa_pre_round_reg;
	reg	[4:0]	priority_encoder_reg;
	reg	[31:0]	result_reg;
	reg	sign_int_a_reg1;
	reg	sign_int_a_reg2;
	reg	sign_int_a_reg3;
	reg	sign_int_a_reg4;
	reg	sign_int_a_reg5;
	wire  [30:0]   wire_add_sub1_result;
	wire  [7:0]   wire_add_sub3_result;
	wire  wire_add_sub6_cout;
	wire  [11:0]   wire_add_sub6_result;
	wire  wire_add_sub7_cout;
	wire  [11:0]   wire_add_sub7_result;
	wire  [7:0]   wire_add_sub8_result;
	wire  wire_cmpr4_alb;
	wire aclr;
	wire  [11:0]  add_1_adder1_w;
	wire  [11:0]  add_1_adder2_w;
	wire  [23:0]  add_1_adder_w;
	wire  add_1_w;
	wire  [7:0]  bias_value_w;
	wire  [7:0]  const_bias_value_add_width_int_w;
	wire  [7:0]  exceptions_value;
	wire  [7:0]  exponent_bus;
	wire  [7:0]  exponent_bus_pre;
	wire  [7:0]  exponent_output_w;
	wire  [7:0]  exponent_rounded;
	wire  [7:0]  exponent_zero_w;
	wire  guard_bit_w;
	wire  [30:0]  int_a;
	wire  [30:0]  int_a_2s;
	wire  [30:0]  invert_int_a;
	wire  [4:0]  leading_zeroes;
	wire  [30:0]  mag_int_a;
	wire  [22:0]  mantissa_bus;
	wire  mantissa_overflow;
	wire  [23:0]  mantissa_post_round;
	wire  [23:0]  mantissa_pre_round;
	wire  [23:0]  mantissa_rounded;
	wire  max_neg_value_selector;
	wire  [7:0]  max_neg_value_w;
	wire  [7:0]  minus_leading_zero;
	wire  [31:0]  prio_mag_int_a;
	wire  [31:0]  result_w;
	wire  round_bit_w;
	wire  [30:0]  shifted_mag_int_a;
	wire  sign_bus;
	wire  sign_int_a;
	wire  [5:0]  sticky_bit_bus;
	wire  [5:0]  sticky_bit_or_w;
	wire  sticky_bit_w;
	wire  [2:0]  zero_padding_w;

	int_to_float_altbarrel_shift_fof   altbarrel_shift5
	(
	.aclr(aclr),
	.clk_en(clk_en),
	.clock(clock),
	.data({1'b0, mag_int_a_reg2}),
	.distance(leading_zeroes),
	.result(wire_altbarrel_shift5_result));
	int_to_float_altpriority_encoder_qb6   altpriority_encoder2
	(
	.data(prio_mag_int_a),
	.q(wire_altpriority_encoder2_q));
	// synopsys translate_off
	initial
		add_1_adder1_cout_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) add_1_adder1_cout_reg <= 1'b0;
		else if  (clk_en == 1'b1)   add_1_adder1_cout_reg <= wire_add_sub6_cout;
	// synopsys translate_off
	initial
		add_1_adder1_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) add_1_adder1_reg <= 12'b0;
		else if  (clk_en == 1'b1)   add_1_adder1_reg <= wire_add_sub6_result;
	// synopsys translate_off
	initial
		add_1_adder2_cout_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) add_1_adder2_cout_reg <= 1'b0;
		else if  (clk_en == 1'b1)   add_1_adder2_cout_reg <= wire_add_sub7_cout;
	// synopsys translate_off
	initial
		add_1_adder2_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) add_1_adder2_reg <= 12'b0;
		else if  (clk_en == 1'b1)   add_1_adder2_reg <= wire_add_sub7_result;
	// synopsys translate_off
	initial
		add_1_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) add_1_reg <= 1'b0;
		else if  (clk_en == 1'b1)   add_1_reg <= add_1_w;
	// synopsys translate_off
	initial
		exponent_bus_pre_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) exponent_bus_pre_reg <= 8'b0;
		else if  (clk_en == 1'b1)   exponent_bus_pre_reg <= exponent_bus_pre_reg2;
	// synopsys translate_off
	initial
		exponent_bus_pre_reg2 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) exponent_bus_pre_reg2 <= 8'b0;
		else if  (clk_en == 1'b1)   exponent_bus_pre_reg2 <= exponent_bus_pre_reg3;
	// synopsys translate_off
	initial
		exponent_bus_pre_reg3 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) exponent_bus_pre_reg3 <= 8'b0;
		else if  (clk_en == 1'b1)   exponent_bus_pre_reg3 <= exponent_bus_pre;
	// synopsys translate_off
	initial
		mag_int_a_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) mag_int_a_reg <= 31'b0;
		else if  (clk_en == 1'b1)   mag_int_a_reg <= mag_int_a;
	// synopsys translate_off
	initial
		mag_int_a_reg2 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) mag_int_a_reg2 <= 31'b0;
		else if  (clk_en == 1'b1)   mag_int_a_reg2 <= mag_int_a_reg;
	// synopsys translate_off
	initial
		mantissa_pre_round_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) mantissa_pre_round_reg <= 24'b0;
		else if  (clk_en == 1'b1)   mantissa_pre_round_reg <= mantissa_pre_round;
	// synopsys translate_off
	initial
		priority_encoder_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) priority_encoder_reg <= 5'b0;
		else if  (clk_en == 1'b1)   priority_encoder_reg <= wire_altpriority_encoder2_q;
	// synopsys translate_off
	initial
		result_reg = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) result_reg <= 32'b0;
		else if  (clk_en == 1'b1)   result_reg <= result_w;
	// synopsys translate_off
	initial
		sign_int_a_reg1 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) sign_int_a_reg1 <= 1'b0;
		else if  (clk_en == 1'b1)   sign_int_a_reg1 <= sign_int_a;
	// synopsys translate_off
	initial
		sign_int_a_reg2 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) sign_int_a_reg2 <= 1'b0;
		else if  (clk_en == 1'b1)   sign_int_a_reg2 <= sign_int_a_reg1;
	// synopsys translate_off
	initial
		sign_int_a_reg3 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) sign_int_a_reg3 <= 1'b0;
		else if  (clk_en == 1'b1)   sign_int_a_reg3 <= sign_int_a_reg2;
	// synopsys translate_off
	initial
		sign_int_a_reg4 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) sign_int_a_reg4 <= 1'b0;
		else if  (clk_en == 1'b1)   sign_int_a_reg4 <= sign_int_a_reg3;
	// synopsys translate_off
	initial
		sign_int_a_reg5 = 0;
	// synopsys translate_on
	always @ ( posedge clock or  posedge aclr)
		if (aclr == 1'b1) sign_int_a_reg5 <= 1'b0;
		else if  (clk_en == 1'b1)   sign_int_a_reg5 <= sign_int_a_reg4;
	lpm_add_sub   add_sub1
	(
	.cout(),
	.dataa(invert_int_a),
	.datab(31'b0000000000000000000000000000001),
	.overflow(),
	.result(wire_add_sub1_result)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.add_sub(1'b1),
	.cin(),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		add_sub1.lpm_direction = "ADD",
		add_sub1.lpm_width = 31,
		add_sub1.lpm_type = "lpm_add_sub",
		add_sub1.lpm_hint = "ONE_INPUT_IS_CONSTANT=YES";
	lpm_add_sub   add_sub3
	(
	.cout(),
	.dataa(const_bias_value_add_width_int_w),
	.datab(minus_leading_zero),
	.overflow(),
	.result(wire_add_sub3_result)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.add_sub(1'b1),
	.cin(),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		add_sub3.lpm_direction = "SUB",
		add_sub3.lpm_width = 8,
		add_sub3.lpm_type = "lpm_add_sub",
		add_sub3.lpm_hint = "ONE_INPUT_IS_CONSTANT=YES";
	lpm_add_sub   add_sub6
	(
	.cout(wire_add_sub6_cout),
	.dataa(mantissa_pre_round[11:0]),
	.datab(12'b000000000001),
	.overflow(),
	.result(wire_add_sub6_result)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.add_sub(1'b1),
	.cin(),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		add_sub6.lpm_direction = "ADD",
		add_sub6.lpm_width = 12,
		add_sub6.lpm_type = "lpm_add_sub",
		add_sub6.lpm_hint = "ONE_INPUT_IS_CONSTANT=YES";
	lpm_add_sub   add_sub7
	(
	.cout(wire_add_sub7_cout),
	.dataa(mantissa_pre_round[23:12]),
	.datab(12'b000000000001),
	.overflow(),
	.result(wire_add_sub7_result)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.add_sub(1'b1),
	.cin(),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		add_sub7.lpm_direction = "ADD",
		add_sub7.lpm_width = 12,
		add_sub7.lpm_type = "lpm_add_sub",
		add_sub7.lpm_hint = "ONE_INPUT_IS_CONSTANT=YES";
	lpm_add_sub   add_sub8
	(
	.cout(),
	.dataa(exponent_bus_pre_reg),
	.datab(8'b00000001),
	.overflow(),
	.result(wire_add_sub8_result)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.add_sub(1'b1),
	.cin(),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		add_sub8.lpm_direction = "ADD",
		add_sub8.lpm_width = 8,
		add_sub8.lpm_type = "lpm_add_sub",
		add_sub8.lpm_hint = "ONE_INPUT_IS_CONSTANT=YES";
	lpm_compare   cmpr4
	(
	.aeb(),
	.agb(),
	.ageb(),
	.alb(wire_cmpr4_alb),
	.aleb(),
	.aneb(),
	.dataa(exponent_output_w),
	.datab(bias_value_w)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_off
	`endif
	,
	.aclr(1'b0),
	.clken(1'b1),
	.clock(1'b0)
	`ifndef FORMAL_VERIFICATION
	// synopsys translate_on
	`endif
	);
	defparam
		cmpr4.lpm_representation = "UNSIGNED",
		cmpr4.lpm_width = 8,
		cmpr4.lpm_type = "lpm_compare";
	assign
		aclr = 1'b0,
		add_1_adder1_w = add_1_adder1_reg,
		add_1_adder2_w = (({12{(~ add_1_adder1_cout_reg)}} & mantissa_pre_round_reg[23:12]) | ({12{add_1_adder1_cout_reg}} & add_1_adder2_reg)),
		add_1_adder_w = {add_1_adder2_w, add_1_adder1_w},
		add_1_w = ((((~ guard_bit_w) & round_bit_w) & sticky_bit_w) | (guard_bit_w & round_bit_w)),
		bias_value_w = 8'b01111111,
		const_bias_value_add_width_int_w = 8'b10011101,
		exceptions_value = (({8{(~ max_neg_value_selector)}} & exponent_zero_w) | ({8{max_neg_value_selector}} & max_neg_value_w)),
		exponent_bus = exponent_rounded,
		exponent_bus_pre = (({8{(~ wire_cmpr4_alb)}} & exponent_output_w) | ({8{wire_cmpr4_alb}} & exceptions_value)),
		exponent_output_w = wire_add_sub3_result,
		exponent_rounded = (({8{(~ mantissa_overflow)}} & exponent_bus_pre_reg) | ({8{mantissa_overflow}} & wire_add_sub8_result)),
		exponent_zero_w = {8{1'b0}},
		guard_bit_w = shifted_mag_int_a[7],
		int_a = dataa[30:0],
		int_a_2s = wire_add_sub1_result,
		invert_int_a = (~ int_a),
		leading_zeroes = (~ priority_encoder_reg),
		mag_int_a = (({31{(~ sign_int_a)}} & int_a) | ({31{sign_int_a}} & int_a_2s)),
		mantissa_bus = mantissa_rounded[22:0],
		mantissa_overflow = ((add_1_reg & add_1_adder1_cout_reg) & add_1_adder2_cout_reg),
		mantissa_post_round = add_1_adder_w,
		mantissa_pre_round = shifted_mag_int_a[30:7],
		mantissa_rounded = (({24{(~ add_1_reg)}} & mantissa_pre_round_reg) | ({24{add_1_reg}} & mantissa_post_round)),
		max_neg_value_selector = (wire_cmpr4_alb & sign_int_a_reg2),
		max_neg_value_w = 8'b10011110,
		minus_leading_zero = {zero_padding_w, leading_zeroes},
		prio_mag_int_a = {mag_int_a_reg, 1'b1},
		result = result_reg,
		result_w = {sign_bus, exponent_bus, mantissa_bus},
		round_bit_w = shifted_mag_int_a[6],
		shifted_mag_int_a = wire_altbarrel_shift5_result[30:0],
		sign_bus = sign_int_a_reg5,
		sign_int_a = dataa[31],
		sticky_bit_bus = shifted_mag_int_a[5:0],
		sticky_bit_or_w = {(sticky_bit_or_w[4] | sticky_bit_bus[5]), (sticky_bit_or_w[3] | sticky_bit_bus[4]), (sticky_bit_or_w[2] | sticky_bit_bus[3]), (sticky_bit_or_w[1] | sticky_bit_bus[2]), (sticky_bit_or_w[0] | sticky_bit_bus[1]), sticky_bit_bus[0]},
		sticky_bit_w = sticky_bit_or_w[5],
		zero_padding_w = {3{1'b0}};
endmodule //int_to_float_altfp_convert_jen

