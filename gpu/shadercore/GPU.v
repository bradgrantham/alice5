module GPU
    #(parameter WORD_WIDTH=32, ADDRESS_WIDTH=16, SDRAM_ADDRESS_WIDTH=24)
(
    input wire reset_n,
    input wire clock,
    input wire run,

    output wire halted,
    output wire exception,
    output wire [23:0] exception_data,

    input wire ext_enable_write_inst_ram,
    input wire[ADDRESS_WIDTH-1:0] ext_inst_ram_address,
    input wire[WORD_WIDTH-1:0] ext_inst_ram_input,
    output wire[WORD_WIDTH-1:0] ext_inst_ram_output,

    input wire ext_enable_write_data_ram,
    input wire[ADDRESS_WIDTH-1:0] ext_data_ram_address,
    input wire[WORD_WIDTH-1:0] ext_data_ram_input,
    output wire[WORD_WIDTH-1:0] ext_data_ram_output,

    input wire[ADDRESS_WIDTH-1:0] ext_register_address,
    output wire[WORD_WIDTH-1:0] ext_register_output,

    input wire[ADDRESS_WIDTH-1:0] ext_floatreg_address,
    output wire[WORD_WIDTH-1:0] ext_floatreg_output,

    input wire[ADDRESS_WIDTH-1:0] ext_specialreg_address,
    output wire[WORD_WIDTH-1:0] ext_specialreg_output,

    // SDRAM interface
    output wire [SDRAM_ADDRESS_WIDTH-1:0] sdram_address /* verilator public */,
    input wire sdram_waitrequest /* verilator public */,
    input wire [WORD_WIDTH-1:0] sdram_readdata /* verilator public */,
    input wire sdram_readdatavalid /* verilator public */,
    output wire sdram_read /* verilator public */,
    output wire [WORD_WIDTH-1:0] sdram_writedata /* verilator public */,
    output wire sdram_write /* verilator public */,

    output wire mem_request,
    input wire mem_authorized
);

    // Instruction RAM write control
    wire [ADDRESS_WIDTH-1:0] inst_ram_address /* verilator public */;
    wire [WORD_WIDTH-1:0] inst_ram_write_data /* verilator public */;
    wire inst_ram_write /* verilator public */;

    // Inst RAM read out
    wire [WORD_WIDTH-1:0] inst_ram_out_data /* verilator public */;

    // Data RAM write control
    wire [ADDRESS_WIDTH-1:0] data_ram_address /* verilator public */;
    wire [WORD_WIDTH-1:0] data_ram_write_data /* verilator public */;
    wire data_ram_write /* verilator public */;

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

    wire [ADDRESS_WIDTH-1:0] shadercore_inst_ram_address;
    wire [ADDRESS_WIDTH-1:0] shadercore_data_ram_address;
    wire [WORD_WIDTH-1:0] shadercore_data_ram_write_data;
    wire shadercore_enable_write_data_ram;

    ShaderCore #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH), .SDRAM_ADDRESS_WIDTH(SDRAM_ADDRESS_WIDTH))
        shaderCore(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),
            .halted(halted),
            .exception(exception),
            .inst_ram_address(shadercore_inst_ram_address),
            .data_ram_address(shadercore_data_ram_address),
            .data_ram_write_data(shadercore_data_ram_write_data),
            .data_ram_write(shadercore_enable_write_data_ram),
            .inst_ram_read_result(inst_ram_out_data),
            .data_ram_read_result(data_ram_out_data),
            .sdram_address(sdram_address),
            .sdram_waitrequest(sdram_waitrequest),
            .sdram_readdata(sdram_readdata),
            .sdram_readdatavalid(sdram_readdatavalid),
            .sdram_read(sdram_read),
            .sdram_writedata(sdram_writedata),
            .sdram_write(sdram_write),
            .mem_request(mem_request),
            .mem_authorized(mem_authorized)
            // XXX TODO register interface
            );

    // XXX TODO register interface
    assign ext_register_output = 32'hdeadbee1;
    assign ext_floatreg_output = 32'hdeadbee2;
    assign ext_specialreg_output = 32'hdeadbee3;

    assign inst_ram_write = !run ? ext_enable_write_inst_ram : 1'b0;
    assign inst_ram_write_data = !run ? ext_inst_ram_input : 32'b0;
    assign inst_ram_address = !run ? ext_inst_ram_address : shadercore_inst_ram_address;

    assign data_ram_write = !run ? ext_enable_write_data_ram : shadercore_enable_write_data_ram;
    assign data_ram_write_data = !run ? ext_data_ram_input : shadercore_data_ram_write_data;
    assign data_ram_address = !run ? ext_data_ram_address : shadercore_data_ram_address;

    assign ext_inst_ram_output = inst_ram_out_data;
    assign ext_data_ram_output = data_ram_out_data;
	 
    assign exception_data = 0;

endmodule
