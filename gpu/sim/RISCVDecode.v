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

    output reg [4:0] rs1,
    output reg [4:0] rs2,
    output reg [4:0] rs3,
    output reg [4:0] rd,
    output reg [1:0] fmt,
    output reg [2:0] funct3,
    output reg [6:0] funct7,
    output reg [4:0] funct5,
    output reg [4:0] shamt,

    output reg [31:0] imm_alu_load,
    output reg [31:0] imm_store,
    output reg [31:0] imm_branch,
    output reg [31:0] imm_upper,
    output reg [31:0] imm_jump
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
        // TODO float operations

        rs1 <= insn[19:15];
        rs2 <= insn[24:20];
        rs3 <= insn[31:27];
        rd <= insn[11:7];
        fmt <= insn[26:25];
        funct3 <= insn[14:12];
        funct7 <= insn[31:25];
        funct5 <= insn[31:27];
        shamt <= insn[24:20];

        // Replications of bit 31 in following are sign extensions
        imm_alu_load <= {{20{insn[31]}}, insn[31:20]};
        imm_store <= {{20{insn[31]}}, insn[31:25], insn[11:7]};
        imm_branch <= {{19{insn[31]}}, insn[31], insn[7], insn[30:25], insn[11:8], 1'b0};
        imm_upper <= {insn[31:12], 12'b0};
        imm_jump <= {{11{insn[31]}}, insn[31], insn[19:12], insn[20], insn[30:21], 1'b0};
    end
endmodule
