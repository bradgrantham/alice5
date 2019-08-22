
module Main(
    input wire clock,
    input wire reset_n,
    input wire run, // hold low and release reset to write inst and data
    output reg halted,

    input wire [15:0] ext_write_address,
    input wire [31:0] ext_write_data,
    input wire ext_enable_write_inst,
    input wire ext_enable_write_data

);

    localparam WORD_WIDTH = 32;
    localparam ADDRESS_WIDTH = 16;

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

    reg [ADDRESS_WIDTH-1:0] shadercore_inst_address;
    reg [ADDRESS_WIDTH-1:0] shadercore_data_address;
    reg [WORD_WIDTH-1:0] shadercore_data_write_data;
    reg shadercore_enable_write_data;

    ShaderCore #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        shaderCore(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),
            .halted(halted),
            .inst_ram_address(shadercore_inst_address),
            .data_ram_address(shadercore_data_address),
            .data_ram_write_data(shadercore_data_write_data),
            .data_ram_write(shadercore_enable_write_data),
            .inst_ram_read_result(inst_ram_out_data),
            .data_ram_read_result(data_ram_out_data)
            );

    assign inst_ram_write = !run ? ext_enable_write_inst : 0;
    assign inst_ram_write_data = !run ? ext_write_data : 0;
    assign inst_ram_address = !run ? ext_write_address : shadercore_inst_address;

    assign data_ram_write = !run ? ext_enable_write_data : shadercore_enable_write_data;
    assign data_ram_write_data = !run ? ext_write_data : shadercore_data_write_data;
    assign data_ram_address = !run ? ext_write_address : shadercore_data_address;

endmodule
