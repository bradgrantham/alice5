
// Converts 32-bit float to 32-bit int.
module float_to_int(
    input wire [31:0] op,
    output wire [31:0] res);

// Physical parts.
wire [7:0] exp_part = op[30:23];
wire [22:0] fract_part = op[22:0];
wire sign = op[31];

// Logical parts.
wire [31:0] significand = { 8'h0, 1'b1, fract_part };
wire signed [7:0] exponent = exp_part - 127;

// Detect special cases.
wire exp_part_all_on = &exp_part;
wire exp_part_all_off = !|exp_part;
wire fract_part_all_off = !|fract_part;

wire is_inf = exp_part_all_on & fract_part_all_off;
wire is_qnan = exp_part_all_on & fract_part[22];
wire is_snan = exp_part_all_on & !fract_part[22] & |fract_part[21:0];
wire is_zero = exp_part_all_off & fract_part_all_off;

// Partially processed.
wire [31:0] shifted = exponent < 23
            ? significand >> (23 - exponent)
            : significand << (exponent - 23);
wire overflow = exponent >= 31; // Would shift into the sign bit.

assign res = is_zero ? 32'h0
        : is_inf | is_qnan | is_snan | overflow ? 32'h80000000
        : sign ? -shifted
        : shifted;

endmodule

