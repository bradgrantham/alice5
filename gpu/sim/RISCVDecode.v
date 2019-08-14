// RISC-V IMF decoder
module RISCVDecode
    #(parameter INSN_WIDTH=32) 
(
    input wire clock,
    input wire [INSN_WIDTH-1:0] insn,

    output reg opcode_is_ALU_reg_imm
);

    wire [6:0] opcode;

    assign opcode = {insn[6:2],insn[1:0]};

// addi    rd rs1 imm12           14..12=0 6..2=0x04 1..0=3

    always @(posedge clock) begin

        opcode_is_ALU_reg_imm <= opcode == {5'h04,2'h3};

    end
endmodule

