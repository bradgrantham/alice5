// RISC-V IMF decoder
module RISCVDecode
    #(parameter INSN_WIDTH=32) 
(
    input wire clock,
    input wire [INSN_WIDTH-1:0] insn,

    output reg opcode_is_branch,
    output reg opcode_is_ALU_reg_imm,
    output reg opcode_is_ALU_reg_reg,
    output reg opcode_is_jal,
    output reg opcode_is_jalr,
    output reg opcode_is_lui,
    output reg opcode_is_auipc,
    output reg opcode_is_load,
    output reg opcode_is_store,
    output reg opcode_is_system,

    output reg rs1,
    output reg rs2,
    output reg rs3
);

    wire [6:0] opcode;

    assign opcode = {insn[6:2],insn[1:0]};

    always @(posedge clock) begin

        opcode_is_branch <= opcode == {5'h18,2'h3};
        opcode_is_jal <= opcode == {5'h1b,2'h3};
        opcode_is_jalr <= opcode == {5'h19,2'h3};
        opcode_is_lui <= opcode == {5'h0d,2'h3};
        opcode_is_auipc <= opcode == {5'h05,2'h3};
        opcode_is_ALU_reg_imm <= opcode == {5'h04,2'h3};
        opcode_is_ALU_reg_reg <= opcode == {5'h0c,2'h3};
        opcode_is_load <= opcode == {5'h00,2'h3};
        opcode_is_store <= opcode == {5'h08,2'h3};
        opcode_is_system <= opcode == {5'h1c,2'h3};
        // And float operations

        rs1 <= insn[19:15];
        rs2 <= insn[24:20];
        rs3 <= insn[31:27];
    end
endmodule
