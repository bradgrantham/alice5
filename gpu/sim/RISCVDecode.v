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
    output reg opcode_is_fadd,
    output reg opcode_is_fsub,
    output reg opcode_is_fmul,
    output reg opcode_is_fdiv,
    output reg opcode_is_fsgnj,
    output reg opcode_is_fminmax,
    output reg opcode_is_fsqrt,
    output reg opcode_is_fcmp,
    output reg opcode_is_fcvt_f2i,
    output reg opcode_is_fmv_f2i,
    output reg opcode_is_fcvt_i2f,
    output reg opcode_is_fmv_i2f,

    output reg [4:0] rs1,
    output reg [4:0] rs2,
    output reg [4:0] rs3,
    output reg [4:0] rd,
    output reg [1:0] fmt,
    output reg [2:0] funct3_rm,
    output reg [6:0] funct7,
    output reg [4:0] funct5,
    output reg [4:0] shamt_ftype,

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

        opcode_is_fadd <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h0}));
        opcode_is_fsub <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h1}));
        opcode_is_fmul <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h2}));
        opcode_is_fdiv <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h3}));
        opcode_is_fsgnj <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h4}));
        opcode_is_fminmax <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h5}));
        opcode_is_fsqrt <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'hb}));
        opcode_is_fcmp <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h14}));
        opcode_is_fcvt_f2i <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h18}));
        opcode_is_fmv_f2i <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h1c}));
        opcode_is_fcvt_i2f <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h1a}));
        opcode_is_fmv_i2f <= ((opcode == {5'h14,2'h3}) && (funct5 == {5'h1e}));


        rs1 <= insn[19:15];
        rs2 <= insn[24:20];
        rs3 <= insn[31:27];
        rd <= insn[11:7];
        fmt <= insn[26:25];
        funct3_rm <= insn[14:12];
        funct7 <= insn[31:25];
        funct5 <= insn[31:27];
        shamt_ftype <= insn[24:20];

        // Replications of bit 31 in following are sign extensions
        imm_alu_load <= {{20{insn[31]}}, insn[31:20]};
        imm_store <= {{20{insn[31]}}, insn[31:25], insn[11:7]};
        imm_branch <= {{19{insn[31]}}, insn[31], insn[7], insn[30:25], insn[11:8], 1'b0};
        imm_upper <= {insn[31:12], 12'b0};
        imm_jump <= {{11{insn[31]}}, insn[31], insn[19:12], insn[20], insn[30:21], 1'b0};
    end
endmodule
