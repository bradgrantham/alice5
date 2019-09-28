
module Main(
    input wire clock,   // hopefully will be 50 MHz FPGA clock later

`ifdef VERILATOR
    input [31:0] sim_h2f_value,
    output [31:0] sim_f2h_value
`else
`endif
);

    localparam WORD_WIDTH = 32;
    localparam ADDRESS_WIDTH = 16;


    // HPS-to-FPGA communication

    localparam H2F_RESET_N_BIT = 31;            // Reset cores, active low
    localparam H2F_RUN_BIT = 30;                // Operate cores, active high
    localparam H2F_REQUEST_BIT = 29;            // Set high if request command is in H2F[23:0]

    localparam H2F_CMD_PUT_LOW_16 = 8'd0;       // Put 15:0 into low 16 of write register
    localparam H2F_CMD_PUT_HIGH_16 = 8'd1;      // Put 15:0 into high 16 of write register
    localparam H2F_CMD_WRITE_INST_RAM = 8'd2;   // Write write register into inst RAM at address in 15:0
    localparam H2F_CMD_WRITE_DATA_RAM = 8'd3;   // Write write register into data RAM at address in 15:0
    localparam H2F_CMD_READ_INST_RAM = 8'd4;    // Read inst RAM at address in 15:0 into read register
    localparam H2F_CMD_READ_DATA_RAM = 8'd5;    // Read data RAM at address in 15:0 into read register
    // TODO - need to implement reading registers through ShaderCore
    localparam H2F_CMD_READ_X_REG = 8'd6;       // Read X register at 4:0 into read register
    localparam H2F_CMD_READ_F_REG = 8'd7;       // Read F register at 4:0 into read register
    localparam H2F_CMD_READ_SPECIAL = 8'd8;     // Read special CPU value indicated in 15:0 into read register
    localparam H2F_CMD_GET_LOW_16 = 8'd9;       // Get low 16 of read register into F2H 15:0
    localparam H2F_CMD_GET_HIGH_16 = 8'd10;     // Get high 16 of read register into F2H 15:0

    localparam F2H_ERR_UNKNOWN_CMD = 24'hdead00;

    wire [31:0] h2f_value;

    wire reset_n /* verilator public */ = h2f_value[H2F_RESET_N_BIT];
    wire run /* verilator public */ = h2f_value[H2F_RUN_BIT];
    wire h2f_request /* verilator public */ = h2f_value[H2F_REQUEST_BIT];
    wire [7:0] h2f_command /* verilator public */ = h2f_value[23:16];

    // FPGA-to-HPS communication

    reg f2h_busy /* verilator public */;
    reg f2h_cmd_error /* verilator public */;
    reg f2h_run_halted /* verilator public */;
    reg f2h_run_exception /* verilator public */;
    reg [23:0] f2h_data_field /* verilator public */;

    wire [31:0] f2h_value /* verilator public */ = {
        f2h_run_halted,
        f2h_run_exception,
        f2h_busy,
        f2h_cmd_error,
        4'b0,
        f2h_data_field
    };

    // Internal state maintained for H2F and F2H communication
    reg [31:0] read_register /* verilator public */;
    reg [15:0] write_register_low16 /* verilator public */;
    reg [15:0] write_register_high16 /* verilator public */;

    // State machine states for completing command processing
    localparam STATE_INIT = 0;
    localparam STATE_IDLE = 1;
    localparam STATE_ERROR = 1;
    localparam STATE_WAIT_WRITE_INST_RAM = 1;
    localparam STATE_WAIT_WRITE_DATA_RAM = 2;
    localparam STATE_WAIT_READ_INST_RAM = 3;
    localparam STATE_WAIT_READ_DATA_RAM = 4;
    localparam STATE_WAIT_READ_X_REG = 5;
    localparam STATE_WAIT_READ_F_REG = 6;
    localparam STATE_WAIT_READ_SPECIAL = 7;
    reg [3:0] state /* verilator public */;


`ifdef VERILATOR
    assign sim_f2h_value = f2h_value;
    assign h2f_value = sim_h2f_value;
`else
    cyclonev_hps_interface_mpu_general_purpose h2f_gp(
         .gp_in(f2h_value),    // Value to the HPS (continuous).
         .gp_out(h2f_value)    // Value from the HPS (latched).
    );
`endif


    // Instruction RAM write control
    wire [ADDRESS_WIDTH-1:0] inst_ram_address /* verilator public */;
    wire [WORD_WIDTH-1:0] inst_ram_write_data /* verilator public */;
    wire inst_ram_write /* verilator public */;

    // Data RAM write control
    wire [ADDRESS_WIDTH-1:0] data_ram_address /* verilator public */;
    wire [WORD_WIDTH-1:0] data_ram_write_data /* verilator public */;
    wire data_ram_write /* verilator public */;

    // Inst RAM read out
    wire [WORD_WIDTH-1:0] inst_ram_out_data /* verilator public */;

    // Data RAM read out
    wire [WORD_WIDTH-1:0] data_ram_out_data /* verilator public */;

    BlockRam #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        instRam(
            .clock(clock),
            .write_address({2'b00, inst_ram_address[ADDRESS_WIDTH-1:2]}),
            .write(inst_ram_write),
            .write_data(inst_ram_write_data),
            .read_address({2'b00, inst_ram_address[ADDRESS_WIDTH-1:2]}),
            .read_data(inst_ram_out_data));

    BlockRam #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        dataRam(
            .clock(clock),
            .write_address({2'b00, data_ram_address[ADDRESS_WIDTH-1:2]}),
            .write(data_ram_write),
            .write_data(data_ram_write_data),
            .read_address({2'b00, data_ram_address[ADDRESS_WIDTH-1:2]}),
            .read_data(data_ram_out_data));

    wire [ADDRESS_WIDTH-1:0] shadercore_inst_address;
    wire [ADDRESS_WIDTH-1:0] shadercore_data_address;
    wire [WORD_WIDTH-1:0] shadercore_data_write_data;
    wire shadercore_enable_write_data;

    ShaderCore #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        shaderCore(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),
            .halted(f2h_run_halted),
            .exception(f2h_run_exception),
            .inst_ram_address(shadercore_inst_address),
            .data_ram_address(shadercore_data_address),
            .data_ram_write_data(shadercore_data_write_data),
            .data_ram_write(shadercore_enable_write_data),
            .inst_ram_read_result(inst_ram_out_data),
            .data_ram_read_result(data_ram_out_data)
            );

    reg ext_enable_write_inst;
    reg ext_enable_write_data;

    wire [31:0] write_data = {write_register_high16, write_register_low16};
    wire [15:0] cmd_parameter = h2f_value[15:0];

    assign inst_ram_write = !run ? ext_enable_write_inst : 1'b0;
    assign inst_ram_write_data = !run ? write_data : 32'b0;
    assign inst_ram_address = !run ? cmd_parameter : shadercore_inst_address;

    assign data_ram_write = !run ? ext_enable_write_data : shadercore_enable_write_data;
    assign data_ram_write_data = !run ? write_data : shadercore_data_write_data;
    assign data_ram_address = !run ? cmd_parameter : shadercore_data_address;

    always @(posedge clock) begin
        if (h2f_request && !run) begin

            f2h_busy <= 1;
            f2h_cmd_error <= 0;

            case (h2f_command)
                H2F_CMD_PUT_LOW_16: begin
                    write_register_low16 <= cmd_parameter;
                    state <= STATE_IDLE;
                end
                H2F_CMD_PUT_HIGH_16: begin
                    write_register_high16 <= cmd_parameter;
                    state <= STATE_IDLE;
                end
                H2F_CMD_WRITE_INST_RAM: begin
                    ext_enable_write_inst <= 1;
                    // inst_ram_address gets cmd_parameter combinationally if !run
                    // inst_ram_write_data gets {write_register_high16, write_register_low16} combinationally if !run
                    state <= STATE_WAIT_WRITE_INST_RAM;
                end
                H2F_CMD_WRITE_DATA_RAM: begin
                    ext_enable_write_data <= 1;
                    // data_ram_address gets cmd_parameter combinationally if !run
                    // data_ram_write_data gets {write_register_high16, write_register_low16} combinationally if !run
                    state <= STATE_WAIT_WRITE_DATA_RAM;
                end
                H2F_CMD_READ_INST_RAM: begin
                    // inst_ram_address gets cmd_parameter combinationally if !run
                    state <= STATE_WAIT_READ_INST_RAM;
                end
                H2F_CMD_READ_DATA_RAM: begin
                    // data_ram_address gets cmd_parameter combinationally if !run
                    state <= STATE_WAIT_READ_DATA_RAM;
                end
                H2F_CMD_READ_X_REG: begin
                    // x_reg_number <= cmd_parameter;
                    state <= STATE_WAIT_READ_X_REG;
                end
                H2F_CMD_READ_F_REG: begin
                    // f_reg_number <= cmd_parameter;
                    state <= STATE_WAIT_READ_F_REG;
                end
                H2F_CMD_READ_SPECIAL: begin
                    // f_reg_number <= cmd_parameter;
                    state <= STATE_WAIT_READ_SPECIAL;
                end
                H2F_CMD_GET_LOW_16: begin
                    f2h_data_field <= {8'b0, read_register[15:0]};
                    state <= STATE_IDLE;
                end
                H2F_CMD_GET_HIGH_16: begin
                    f2h_data_field <= {8'b0, read_register[31:16]};
                    state <= STATE_IDLE;
                end
                default: begin
                    f2h_data_field <= F2H_ERR_UNKNOWN_CMD;
                    state <= STATE_IDLE;
                end
            endcase
        end else begin
	    // I haven't extracted "f2h_busy <= 0" from any case
	    // statement yet because there could end up being
            // transitional states in which the core is still busy
            // on the way to STATE_IDLE
            case (state)
                STATE_INIT: begin
                    f2h_busy <= 0;
                    f2h_cmd_error <= 0;
                    state <= STATE_IDLE;
                end
                STATE_ERROR: begin
                    f2h_busy <= 0;
                    f2h_cmd_error <= 1;
                    state <= STATE_ERROR;
                end
                STATE_IDLE: begin
                    f2h_busy <= 0;
                    state <= STATE_IDLE;
                end
                STATE_WAIT_WRITE_INST_RAM: begin
                    // for now assume one cycle write
                    ext_enable_write_inst <= 0;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_WRITE_DATA_RAM: begin
                    // for now assume one cycle write
                    ext_enable_write_data <= 0;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_READ_INST_RAM: begin
                    // for now assume one cycle read
                    read_register <= inst_ram_out_data;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_READ_DATA_RAM: begin
                    // for now assume one cycle read
                    read_register <= data_ram_out_data;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_READ_X_REG: begin
                    read_register <= 32'hdeafca75;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_READ_F_REG: begin
                    read_register <= 32'hdeafca75;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
                STATE_WAIT_READ_SPECIAL: begin
                    read_register <= 32'hdeafca75;
                    state <= STATE_IDLE;
                    f2h_busy <= 0;
                end
            endcase
        end
    end

endmodule
