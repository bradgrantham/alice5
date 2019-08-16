
module Main(
    input wire clock,
    input wire reset_n,

    input wire [15:0] inst_ext_address,
    input wire inst_ext_write,
    input wire [31:0] inst_ext_in_data,
    output wire [31:0] inst_ext_out_data,

    input wire [15:0] data_ext_address,
    input wire data_ext_write,
    input wire [31:0] data_ext_in_data,
    output wire [31:0] data_ext_out_data,

    input wire [4:0] reg_ext_address,
    output wire [31:0] reg_ext_out_data,

    input wire [31:0] ext_inst_to_decode,
    output wire decode_opcode_is_branch,
    output wire decode_opcode_is_ALU_reg_imm,
    output wire decode_opcode_is_ALU_reg_reg,
    output wire decode_opcode_is_jal,
    output wire decode_opcode_is_jalr,
    output wire decode_opcode_is_lui,
    output wire decode_opcode_is_auipc,
    output wire decode_opcode_is_load,
    output wire decode_opcode_is_store,
    output wire decode_opcode_is_system,
    output wire decode_opcode_is_fadd,
    output wire decode_opcode_is_fsub,
    output wire decode_opcode_is_fmul,
    output wire decode_opcode_is_fdiv,
    output wire decode_opcode_is_fsgnj,
    output wire decode_opcode_is_fminmax,
    output wire decode_opcode_is_fsqrt,
    output wire decode_opcode_is_fcmp,
    output wire decode_opcode_is_fcvt_f2i,
    output wire decode_opcode_is_fmv_f2i,
    output wire decode_opcode_is_fcvt_i2f,
    output wire decode_opcode_is_fmv_i2f,
    output wire decode_opcode_is_flw,
    output wire decode_opcode_is_fsw,
    output wire decode_opcode_is_fmadd,
    output wire decode_opcode_is_fmsub,
    output wire decode_opcode_is_fnmsub,
    output wire decode_opcode_is_fnmadd,
    output wire [4:0] decode_rs1,
    output wire [4:0] decode_rs2,
    output wire [4:0] decode_rs3,
    output wire [4:0] decode_rd,
    output wire [1:0] decode_fmt,
    output wire [2:0] decode_funct3_rm,
    output wire [6:0] decode_funct7,
    output wire [4:0] decode_funct5,
    output wire [4:0] decode_shamt_ftype,
    output wire [31:0] decode_imm_alu_load,
    output wire [31:0] decode_imm_store,
    output wire [31:0] decode_imm_branch,
    output wire [31:0] decode_imm_upper,
    output wire [31:0] decode_imm_jump
);

    // CPU State Machine
    localparam STATE_INIT = 3'h00;
    localparam STATE_FETCH1 = 3'h01;
    localparam STATE_FETCH2 = 3'h02;
    localparam STATE_DECODE = 3'h03;
    localparam STATE_ALU = 3'h04;
    localparam STATE_STEPLOADSTORE = 3'h05;
    reg [2:0] state /* verilator public */;
    reg [31:0] PC /* verilator public */;

    reg [15:0] fetch_inst_address /* verilator public */;
    reg [31:0] fetched_inst /* verilator public */;

    wire [31:0] inst_to_decode = (state == STATE_INIT) ? ext_inst_to_decode : fetched_inst;

    // Instruction RAM
    // Connect block RAM to our own module's parameters.
    wire [15:0] inst_ram_address = (state == STATE_INIT) ? inst_ext_address : fetch_inst_address;
    wire [31:0] inst_ram_in_data = inst_ext_in_data;
    wire inst_ram_write = inst_ext_write;
    wire [31:0] inst_ram_out_data;
    assign inst_ext_out_data = inst_ram_out_data;

    RISCVDecode #(.INSN_WIDTH(32))
        instDecode(
            .clock(clock),
            .inst(inst_to_decode),
            .opcode_is_branch(decode_opcode_is_branch),
            .opcode_is_ALU_reg_imm(decode_opcode_is_ALU_reg_imm),
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

    BlockRam #(.WORD_WIDTH(32), .ADDRESS_WIDTH(16))
        instRam(
            .clock(clock),
            .write_address({2'b00, inst_ram_address[15:2]}),
            .write(inst_ram_write),
            .write_data(inst_ram_in_data),
            .read_address({2'b00, inst_ram_address[15:2]}),
            .read_data(inst_ram_out_data));

    // Data RAM
    wire [15:0] data_ram_address = data_ext_address;
    wire [31:0] data_ram_in_data = data_ext_in_data;
    wire data_ram_write = data_ext_write;
    wire [31:0] data_ram_out_data;
    assign data_ext_out_data = data_ram_out_data;

    BlockRam #(.WORD_WIDTH(32), .ADDRESS_WIDTH(16))
        dataRam(
            .clock(clock),
            .write_address({2'b00, data_ram_address[15:2]}),
            .write(data_ram_write),
            .write_data(data_ram_in_data),
            .read_address({2'b00, data_ram_address[15:2]}),
            .read_data(data_ram_out_data));

    // Register bank.
    wire [31:0] unused;
    Registers registers(
        .clock(clock),

        .write_address(test_write_address),
        .write(test_write),
        .write_data(test_write_data),

        .read1_address(reg_ext_address),
        .read1_data(reg_ext_out_data),

        .read2_address(test_read_address),
        .read2_data(test_read_data));

    // Test auto-increment of register x1.
    reg [4:0] test_write_address /* verilator public */;
    reg test_write /* verilator public */;
    reg [31:0] test_write_data /* verilator public */;
    reg [4:0] test_read_address /* verilator public */;
    wire [31:0] test_read_data /* verilator public */;
    reg [1:0] test_state /* verilator public */;
    always @(posedge clock) begin
        case (test_state)
            0: begin
                // Read register x1. Result will be ready next clock.
                test_write <= 1'b0;
                test_read_address <= 5'h1;
            end
            1: begin
                // Wait.
            end
            2: begin
                // Write new value x1.
                test_write_address <= 5'h1;
                test_write <= 1'b1;
                test_write_data <= test_read_data + 32'h1;
            end
            3: begin
                // Wait.
            end
        endcase

        test_state <= test_state + 1'b1;
    end

    always @(posedge clock) begin
        if(!reset_n) begin

            // Enter INIT when we leave RESET
            state <= STATE_INIT;
            PC <= 32'b0;
            // reset registers?  Could assume they're junk in the compiler and save gates

        end else begin

            case (state)
                 STATE_INIT: begin
                     state <= STATE_FETCH1;
                 end
                 STATE_FETCH1: begin
                     // want PC to be settled here
                     // get 32 bits from memory[PC]
                     fetch_inst_address <= PC[15:0];
                     state <= STATE_FETCH2;
                 end
                 STATE_FETCH2: begin
                     // want output of RAM to be settled here
                     fetched_inst <= inst_ram_out_data;
                     state <= STATE_DECODE;
                 end
                 STATE_DECODE: begin
                     // clock the 32 bits of instruction into decoded signals
                     state <= STATE_ALU;
                 end
                 STATE_ALU: begin
                     // want decode of instruction to be settled here
                     // do ALU operation
                     state <= STATE_STEPLOADSTORE;
                 end
                 STATE_STEPLOADSTORE: begin
                     // want result of ALU to be settled here
                     // Would be output of ALU for branch and jalr or route from imm20 for branch
                     PC <= PC + 4;
                     // load into registers?
                     // store from registers - need to stall reads from data, don't need to stall inst reads
                     state <= STATE_FETCH1;
                 end
            endcase
        end
    end

endmodule
