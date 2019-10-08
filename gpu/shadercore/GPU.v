module GPU
    #(parameter WORD_WIDTH=32, ADDRESS_WIDTH=16) 
(
    input wire reset_n,
    input wire clock,
    input wire run,

    output reg halted,
    output reg exception,
    output reg[23:0] exception_data,

    input wire ext_enable_write_inst,
    input wire[ADDRESS_WIDTH-1:0] ext_inst_address,
    input wire[WORD_WIDTH-1:0] ext_inst_input,
    output wire[WORD_WIDTH-1:0] ext_inst_output,

    input wire ext_enable_write_data,
    input wire[ADDRESS_WIDTH-1:0] ext_data_address,
    input wire[WORD_WIDTH-1:0] ext_data_input,
    output wire[WORD_WIDTH-1:0] ext_data_output,

    input wire[ADDRESS_WIDTH-1:0] ext_register_address,
    output wire[WORD_WIDTH-1:0] ext_register_output,

    input wire[ADDRESS_WIDTH-1:0] ext_floatreg_address,
    output wire[WORD_WIDTH-1:0] ext_floatreg_output,

    input wire[ADDRESS_WIDTH-1:0] ext_specialreg_address,
    output wire[WORD_WIDTH-1:0] ext_specialreg_output
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

    wire [ADDRESS_WIDTH-1:0] shadercore_inst_address;
    wire [ADDRESS_WIDTH-1:0] shadercore_data_address;
    wire [WORD_WIDTH-1:0] shadercore_data_write_data;
    wire shadercore_enable_write_data;

    ShaderCore #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        shaderCore(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),
            .halted(halted),
            .exception(exception),
            .inst_ram_address(shadercore_inst_address),
            .data_ram_address(shadercore_data_address),
            .data_ram_write_data(shadercore_data_write_data),
            .data_ram_write(shadercore_enable_write_data),
            .inst_ram_read_result(inst_ram_out_data),
            .data_ram_read_result(data_ram_out_data)
            // XXX TODO register interface
            );

    // XXX TODO register interface
    assign ext_register_output = 32'hdeadbee1;
    assign ext_floatreg_output = 32'hdeadbee2;
    assign ext_specialreg_output = 32'hdeadbee3;

    assign inst_ram_write = !run ? ext_enable_write_inst : 1'b0;
    assign inst_ram_write_data = !run ? ext_inst_input : 32'b0;
    assign inst_ram_address = !run ? ext_inst_address : shadercore_inst_address;

    assign data_ram_write = !run ? ext_enable_write_data : shadercore_enable_write_data;
    assign data_ram_write_data = !run ? ext_data_input : shadercore_data_write_data;
    assign data_ram_address = !run ? ext_data_address : shadercore_data_address;

    assign ext_inst_output = inst_ram_out_data;
    assign ext_data_output = data_ram_out_data;

endmodule
