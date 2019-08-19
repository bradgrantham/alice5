
module Main(
    input wire clock,
    input wire [1:0] rmode,
    input wire [2:0] fpu_op,
    input wire [31:0] opa,
    input wire [31:0] opb,
    output wire [31:0] out,
    output wire inf, snan, qnan,
    output wire ine,
    output wire overflow, underflow,
    output wire zero,
    output wire div_by_zero,

    input [31:0] cmp_opa, cmp_opb,
    output cmp_unordered,
    output cmp_altb, cmp_blta, cmp_aeqb,
    output cmp_inf, cmp_zero
);

fpu fpu(
    .clk(clock),
    .rmode(rmode),
    .fpu_op(fpu_op),
    .opa(opa),
    .opb(opb),
    .out(out),
    .inf(inf),
    .snan(snan),
    .qnan(qnan),
    .ine(ine),
    .overflow(overflow),
    .underflow(underflow),
    .zero(zero),
    .div_by_zero(div_by_zero));

fcmp fcmp(
    .opa(cmp_opa),
    .opb(cmp_opb),
    .unordered(cmp_unordered),
    .altb(cmp_altb),
    .blta(cmp_blta),
    .aeqb(cmp_aeqb),
    .inf(cmp_inf),
    .zero(cmp_zero));

endmodule
