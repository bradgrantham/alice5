
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
    output wire div_by_zero
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

endmodule
