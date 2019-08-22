module ShaderCore
    #(parameter WORD_WIDTH=32, ADDRESS_WIDTH=16) 
(
    input wire clock,
    input wire run,
    input wire reset_n,
    output reg halted,

    // Instruction RAM write control
    output reg [ADDRESS_WIDTH-1:0] inst_ram_address /* verilator public */,

    // Data RAM write control
    output reg [ADDRESS_WIDTH-1:0] data_ram_address /* verilator public */,
    output reg [WORD_WIDTH-1:0] data_ram_write_data /* verilator public */,
    output reg data_ram_write /* verilator public */,

    // Inst RAM read out
    input wire [WORD_WIDTH-1:0] inst_ram_read_result /* verilator public */,

    // Data RAM read out
    input wire [WORD_WIDTH-1:0] data_ram_read_result /* verilator public */
);

    localparam REGISTER_ADDRESS_WIDTH = 5;

    // CPU State Machine
    localparam STATE_INIT /* verilator public */ = 4'd00;
    localparam STATE_FETCH /* verilator public */ = 4'd01;
    localparam STATE_FETCH2 /* verilator public */ = 4'd02;
    localparam STATE_DECODE /* verilator public */ = 4'd03;
    localparam STATE_REGISTERS /* verilator public */ = 4'd04;
    localparam STATE_ALU /* verilator public */ = 4'd05;
    localparam STATE_RETIRE /* verilator public */ = 4'd06;
    localparam STATE_LOAD /* verilator public */ = 4'd07;
    localparam STATE_LOAD2 /* verilator public */ = 4'd08;
    localparam STATE_STORE /* verilator public */ = 4'd09;
    localparam STATE_HALTED /* verilator public */ = 4'd10;
    reg [3:0] state /* verilator public */;

    reg [WORD_WIDTH-1:0] PC /* verilator public */;

    // instruction latched for inst decoder
    reg [WORD_WIDTH-1:0] inst_to_decode /* verilator public */;

    wire [WORD_WIDTH-1:0] rs1_value /* verilator public */;
    wire [WORD_WIDTH-1:0] rs2_value /* verilator public */;
    wire [WORD_WIDTH-1:0] float_rs1_value /* verilator public */;
    wire [WORD_WIDTH-1:0] float_rs2_value /* verilator public */;
    reg [REGISTER_ADDRESS_WIDTH-1:0] rd_address;
    reg [WORD_WIDTH-1:0] alu_result;
    reg [WORD_WIDTH-1:0] rd_value;
    reg [WORD_WIDTH-1:0] float_rd_value;
    reg enable_write_rd;
    reg enable_write_float_rd;

    // Register bank.
    Registers registers(
        .clock(clock),

        .write_address(rd_address),
        .write(enable_write_rd),
        .write_data(rd_value),

        .read1_address(decode_rs1),
        .read1_data(rs1_value),

        .read2_address(decode_rs2),
        .read2_data(rs2_value));

    // Register bank.
    Registers float_registers(
        .clock(clock),

        .write_address(rd_address),
        .write(enable_write_float_rd),
        .write_data(float_rd_value),

        .read1_address(decode_rs1),
        .read1_data(float_rs1_value),

        .read2_address(decode_rs2),
        .read2_data(float_rs2_value));

    wire decode_opcode_is_branch /* verilator public */;
    wire decode_opcode_is_ALU_reg_imm /* verilator public */;
    wire decode_opcode_add_is_add /* verilator public */;
    wire decode_opcode_shift_is_logical /* verilator public */;
    wire decode_opcode_is_ALU_reg_reg /* verilator public */;
    wire decode_opcode_is_jal /* verilator public */;
    wire decode_opcode_is_jalr /* verilator public */;
    wire decode_opcode_is_lui /* verilator public */;
    wire decode_opcode_is_auipc /* verilator public */;
    wire decode_opcode_is_load /* verilator public */;
    wire decode_opcode_is_store /* verilator public */;
    wire decode_opcode_is_system /* verilator public */;
    wire decode_opcode_is_fadd /* verilator public */;
    wire decode_opcode_is_fsub /* verilator public */;
    wire decode_opcode_is_fmul /* verilator public */;
    wire decode_opcode_is_fdiv /* verilator public */;
    wire decode_opcode_is_fsgnj /* verilator public */;
    wire decode_opcode_is_fminmax /* verilator public */;
    wire decode_opcode_is_fsqrt /* verilator public */;
    wire decode_opcode_is_fcmp /* verilator public */;
    wire decode_opcode_is_fcvt_f2i /* verilator public */;
    wire decode_opcode_is_fmv_f2i /* verilator public */;
    wire decode_opcode_is_fcvt_i2f /* verilator public */;
    wire decode_opcode_is_fmv_i2f /* verilator public */;
    wire decode_opcode_is_flw /* verilator public */;
    wire decode_opcode_is_fsw /* verilator public */;
    wire decode_opcode_is_fmadd /* verilator public */;
    wire decode_opcode_is_fmsub /* verilator public */;
    wire decode_opcode_is_fnmsub /* verilator public */;
    wire decode_opcode_is_fnmadd /* verilator public */;
    wire [4:0] decode_rs1 /* verilator public */;
    wire [4:0] decode_rs2 /* verilator public */;
    /* verilator lint_off UNUSED */
    wire [4:0] decode_rs3 /* verilator public */;
    /* verilator lint_on UNUSED */
    wire [4:0] decode_rd /* verilator public */;
    wire [1:0] decode_fmt /* verilator public */;
    wire [2:0] decode_funct3_rm /* verilator public */;
    wire [6:0] decode_funct7 /* verilator public */;
    wire [4:0] decode_funct5 /* verilator public */;
    wire [4:0] decode_shamt_ftype /* verilator public */;
    wire signed [11:0] decode_imm_alu_load /* verilator public */;
    wire signed [11:0] decode_imm_store /* verilator public */;
    wire signed [12:0] decode_imm_branch /* verilator public */;
    wire signed [31:0] decode_imm_upper /* verilator public */;
    wire signed [20:0] decode_imm_jump /* verilator public */;

    RISCVDecode #(.INSN_WIDTH(WORD_WIDTH))
        instDecode(
            .inst(inst_to_decode),
            .opcode_is_branch(decode_opcode_is_branch),
            .opcode_is_ALU_reg_imm(decode_opcode_is_ALU_reg_imm),
            .opcode_add_is_add(decode_opcode_add_is_add),
            .opcode_shift_is_logical(decode_opcode_shift_is_logical),
            .opcode_is_ALU_reg_reg(decode_opcode_is_ALU_reg_reg),
            .opcode_is_jal(decode_opcode_is_jal),
            .opcode_is_jalr(decode_opcode_is_jalr),
            .opcode_is_lui(decode_opcode_is_lui),
            .opcode_is_auipc(decode_opcode_is_auipc),
            .opcode_is_load(decode_opcode_is_load),
            .opcode_is_store(decode_opcode_is_store),
            .opcode_is_system(decode_opcode_is_system),
            .opcode_is_fadd(decode_opcode_is_fadd),
            .opcode_is_fsub(decode_opcode_is_fsub),
            .opcode_is_fmul(decode_opcode_is_fmul),
            .opcode_is_fdiv(decode_opcode_is_fdiv),
            .opcode_is_fsgnj(decode_opcode_is_fsgnj),
            .opcode_is_fminmax(decode_opcode_is_fminmax),
            .opcode_is_fsqrt(decode_opcode_is_fsqrt),
            .opcode_is_fcmp(decode_opcode_is_fcmp),
            .opcode_is_fcvt_f2i(decode_opcode_is_fcvt_f2i),
            .opcode_is_fmv_f2i(decode_opcode_is_fmv_f2i),
            .opcode_is_fcvt_i2f(decode_opcode_is_fcvt_i2f),
            .opcode_is_fmv_i2f(decode_opcode_is_fmv_i2f),
            .opcode_is_flw(decode_opcode_is_flw),
            .opcode_is_fsw(decode_opcode_is_fsw),
            .opcode_is_fmadd(decode_opcode_is_fmadd),
            .opcode_is_fmsub(decode_opcode_is_fmsub),
            .opcode_is_fnmsub(decode_opcode_is_fnmsub),
            .opcode_is_fnmadd(decode_opcode_is_fnmadd),
            .rs1(decode_rs1),
            .rs2(decode_rs2),
            .rs3(decode_rs3),
            .rd(decode_rd),
            .fmt(decode_fmt),
            .funct3_rm(decode_funct3_rm),
            .funct7(decode_funct7),
            .funct5(decode_funct5),
            .shamt_ftype(decode_shamt_ftype),
            .imm_alu_load(decode_imm_alu_load),
            .imm_store(decode_imm_store),
            .imm_branch(decode_imm_branch),
            .imm_upper(decode_imm_upper),
            .imm_jump(decode_imm_jump)
            );

    wire inst_has_float_dest =
        decode_opcode_is_fadd ||
        decode_opcode_is_fsub || 
        decode_opcode_is_fmul || 
        decode_opcode_is_fdiv || 
        decode_opcode_is_fsgnj || 
        decode_opcode_is_fminmax || 
        decode_opcode_is_fsqrt || 
        decode_opcode_is_fcmp || 
        decode_opcode_is_fcvt_i2f ||
        decode_opcode_is_fmv_i2f || 
        decode_opcode_is_flw || 
        decode_opcode_is_fmadd || 
        decode_opcode_is_fmsub || 
        decode_opcode_is_fnmsub || 
        decode_opcode_is_fnmadd;

    wire decoded_beq = (decode_funct3_rm == 0);
    wire decoded_bne = (decode_funct3_rm == 1);
    wire decoded_blt = (decode_funct3_rm == 4);
    wire decoded_bge = (decode_funct3_rm == 5);
    wire decoded_bltu = (decode_funct3_rm == 6);
    wire decoded_bgeu = (decode_funct3_rm == 7);

    wire comparison_succeeded;
    reg comparison_succeeded_reg;

    Comparison
        comparison(
            .v1(rs1_value),
            .v2(rs2_value),
            .compare_equal(decoded_beq),
            .compare_not_equal(decoded_bne),
            .compare_less_than(decoded_blt),
            .compare_greater_equal(decoded_bge),
            .compare_less_than_unsigned(decoded_bltu),
            .compare_greater_equal_unsigned(decoded_bgeu),
            .result(comparison_succeeded)
            );

    wire signed [WORD_WIDTH-1:0] alu_op1 /* verilator public */ ;
    wire signed [WORD_WIDTH-1:0] alu_op2 /* verilator public */ ;
    wire signed [3:0] alu_operator /* verilator public */ ;

    // If all parameters are signed, shorter parameters will be
    // sign-extended, according to Pong
    assign alu_op1 =
        (decode_opcode_is_ALU_reg_imm ||
           decode_opcode_is_ALU_reg_reg ||
           decode_opcode_is_jalr ||
           decode_opcode_is_load ||
           decode_opcode_is_flw ||
           decode_opcode_is_fsw ||
           decode_opcode_is_store) ? $signed(rs1_value) :
        decode_opcode_is_lui ? $signed(decode_imm_upper) :
        decode_opcode_is_jal ? $signed(PC) :
        decode_opcode_is_branch ? $signed(PC) :
        $signed(32'hdeadbeef);

    wire alu_op_is_shift = (decode_funct3_rm == 1) || (decode_funct3_rm == 5);

/* verilator lint_off WIDTH */
    /* skip extension warnings in alu_op2 below by extending everything */
    wire signed [WORD_WIDTH-1:0] extended_shamt = decode_shamt_ftype;
    wire signed [WORD_WIDTH-1:0] extended_imm_alu_load /* verilator public */ = decode_imm_alu_load;
    wire signed [WORD_WIDTH-1:0] masked_rs2_value = rs2_value[4:0];
    wire signed [WORD_WIDTH-1:0] extended_imm_jump = decode_imm_jump;
    wire signed [WORD_WIDTH-1:0] extended_imm_store = decode_imm_store;
    wire signed [WORD_WIDTH-1:0] extended_imm_branch = decode_imm_branch;
/* verilator lint_on WIDTH */

    assign alu_op2 =
        decode_opcode_is_ALU_reg_imm ? 
            (alu_op_is_shift ? extended_shamt : extended_imm_alu_load) :
        (decode_opcode_is_load ||
            decode_opcode_is_flw ||
            decode_opcode_is_jalr) ? extended_imm_alu_load :
        decode_opcode_is_ALU_reg_reg ? 
            (alu_op_is_shift ? masked_rs2_value : $signed(rs2_value)) :
        decode_opcode_is_lui ? 0 :
        decode_opcode_is_jal ? extended_imm_jump :
        (decode_opcode_is_store || decode_opcode_is_fsw) ? extended_imm_store :
        decode_opcode_is_branch ? extended_imm_branch :
        $signed(32'hcafebabe);

    wire [3:0] alu_reg_imm_operator =
        (decode_funct3_rm == 0) ? alu.ALU_OP_ADD :
        (decode_funct3_rm == 1) ? alu.ALU_OP_SLL :
        (decode_funct3_rm == 2) ? alu.ALU_OP_SLT :
        (decode_funct3_rm == 3) ? alu.ALU_OP_SLTU :
        (decode_funct3_rm == 4) ? alu.ALU_OP_XOR :
        (decode_funct3_rm == 5) ? (decode_opcode_shift_is_logical ? alu.ALU_OP_SRL : alu.ALU_OP_SRA) :
        (decode_funct3_rm == 6) ? alu.ALU_OP_OR :
        /* (decode_funct3_rm == 7) ? */ alu.ALU_OP_AND; // has to be AND

    wire [3:0] alu_reg_reg_operator =
        (decode_funct3_rm == 0) ? (decode_opcode_add_is_add ? alu.ALU_OP_ADD : alu.ALU_OP_SUB) :
        (decode_funct3_rm == 1) ? alu.ALU_OP_SLL :
        (decode_funct3_rm == 2) ? alu.ALU_OP_SLT :
        (decode_funct3_rm == 3) ? alu.ALU_OP_SLTU :
        (decode_funct3_rm == 4) ? alu.ALU_OP_XOR :
        (decode_funct3_rm == 5) ? (decode_opcode_shift_is_logical ? alu.ALU_OP_SRL : alu.ALU_OP_SRA) :
        (decode_funct3_rm == 6) ? alu.ALU_OP_OR :
        /* (decode_funct3_rm == 7) ? */ alu.ALU_OP_AND; // has to be AND

    assign alu_operator =
        decode_opcode_is_ALU_reg_imm ? alu_reg_imm_operator :
        decode_opcode_is_ALU_reg_reg ? alu_reg_reg_operator :
        (   decode_opcode_is_branch ||
            decode_opcode_is_jalr ||
            decode_opcode_is_jal ||
            decode_opcode_is_auipc ||
            decode_opcode_is_lui ||
            decode_opcode_is_load ||
            decode_opcode_is_store ) ? alu.ALU_OP_ADD :
        alu.ALU_OP_NONE;

    ALU #(.WORD_WIDTH(WORD_WIDTH))
        alu 
        (
            .clock(clock),
            .operand1(alu_op1),
            .operand2(alu_op2),
            .operator(alu_operator),
            .result(alu_result)
        );

    always @(posedge clock) begin
        if(!reset_n) begin

            state <= STATE_INIT;
            PC <= 0;
            data_ram_write <= 0;
            enable_write_rd <= 0;
            enable_write_float_rd <= 0;

            halted <= 0;

            // reset registers?  Could assume they're junk in the compiler and save gates

        end else begin

            if(run) begin

                case (state)

                    STATE_INIT: begin
                        state <= STATE_FETCH;
                        PC <= 0;
                    end

                    // TODO: put result of PC calculation into
                    // inst_ram_address in RETIRE, and then memory result
                    // will be available here (or at least one clock
                    // earlier)
                    STATE_FETCH: begin
                        // want PC to be settled here
                        // get instruction from memory[PC]
                        enable_write_rd <= 0;
                        enable_write_float_rd <= 0;
                        inst_ram_address <= PC[ADDRESS_WIDTH-1:0];
                        state <= STATE_FETCH2;
                    end

                    STATE_FETCH2: begin
                        state <= STATE_DECODE;
                    end

                    STATE_DECODE: begin
                        // clock the 32 bits of instruction into decoded signals
                        inst_to_decode <= inst_ram_read_result;
                        state <= STATE_REGISTERS;
                    end

                    STATE_REGISTERS: begin
                        halted <= decode_opcode_is_system &&
                            (decode_imm_alu_load[11:0] == 12'd1);
                        state <= STATE_ALU;
                    end

                    STATE_ALU: begin
                        // want decode of instruction and registers output to be settled here
                        // ALU operation occurs in alu instance

                        comparison_succeeded_reg <= comparison_succeeded;

                        rd_address <= decode_rd;

                        state <= halted ? STATE_HALTED :
                            (decode_opcode_is_load || decode_opcode_is_flw) ? STATE_LOAD :
                            (decode_opcode_is_store || decode_opcode_is_fsw) ? STATE_STORE :
                            STATE_RETIRE;
                    end

                    STATE_LOAD: begin
                        // want result of ALU to be settled here
                        data_ram_address <= alu_result[ADDRESS_WIDTH-1:0];
                        state <= STATE_LOAD2;
                    end

                    STATE_LOAD2: begin
                        // clock out load result
                        state <= STATE_RETIRE;
                    end

                    STATE_STORE: begin
                        // want result of ALU to be settled here
                        data_ram_address <= alu_result[ADDRESS_WIDTH-1:0];
                        data_ram_write_data <= decode_opcode_is_fsw ? float_rs2_value : rs2_value;
                        data_ram_write <= 1;
                        state <= STATE_RETIRE;
                    end

                    STATE_RETIRE: begin
                        data_ram_write <= 0;
                        // want result of ALU to be settled here

                        enable_write_rd <= (decode_opcode_is_ALU_reg_imm ||
                            decode_opcode_is_ALU_reg_reg ||
                            decode_opcode_is_lui ||
                            decode_opcode_is_jalr ||
                            decode_opcode_is_jal ||
                            decode_opcode_is_load) &&
                            (rd_address != 0);
                        rd_value <= 
                            (decode_opcode_is_jalr || decode_opcode_is_jal) ? (PC + 4) :
                            decode_opcode_is_load ? data_ram_read_result :
    -                            $unsigned(alu_result);

                        enable_write_float_rd <= inst_has_float_dest;
                        float_rd_value <= 
                            decode_opcode_is_flw ? data_ram_read_result :
                            // float_alu_result;
                            32'hbadf10a7; /* 'Bad Float' */

                        // We know the result of ALU for jal has
                        // LSB 0, so just ignore LSB for both jal and
                        // jalr
                        PC <= 
                            (decode_opcode_is_jal ||
                                 decode_opcode_is_jalr) ? $unsigned({alu_result[WORD_WIDTH-1:1],1'b0}) :
                            (decode_opcode_is_branch && comparison_succeeded_reg) ? $unsigned(alu_result) :
                            (PC + 4);

                        // load into registers?
                        // store from registers - need to stall reads from data, don't need to stall inst reads
                        state <= STATE_FETCH;
                    end

                    STATE_HALTED: begin
                    end

                    default: begin
                        // error, just reset
                        state <= STATE_INIT;
                    end

                endcase
            end
        end
    end

endmodule

