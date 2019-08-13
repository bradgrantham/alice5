
module Main(
    input wire clock,
    output wire led,

    input wire [15:0] inst_ext_address,
    input wire inst_ext_write,
    input wire [31:0] inst_ext_in_data,
    output wire [31:0] inst_ext_out_data,

    input wire [15:0] data_ext_address,
    input wire data_ext_write,
    input wire [31:0] data_ext_in_data,
    output wire [31:0] data_ext_out_data
);

    // Toggle LED.
    reg [2:0] counter;
    always @(posedge clock) begin
        counter <= counter + 1'b1;
    end
    assign led = counter[2];

    // Instruction RAM
    // Connect block RAM to our own module's parameters.
    wire [15:0] inst_ram_address = inst_ext_address;
    wire [31:0] inst_ram_in_data = inst_ext_in_data;
    wire inst_ram_write = inst_ext_write;
    wire [31:0] inst_ram_out_data;
    assign inst_ext_out_data = inst_ram_out_data;

    BlockRam #(.WORD_WIDTH(32), .ADDRESS_WIDTH(16))
        instRam(
            .clock(clock),
            .address(inst_ram_address),
            .write(inst_ram_write),
            .in_data(inst_ram_in_data),
            .out_data(inst_ram_out_data));

    // Data RAM
    wire [15:0] data_ram_address = data_ext_address;
    wire [31:0] data_ram_in_data = data_ext_in_data;
    wire data_ram_write = data_ext_write;
    wire [31:0] data_ram_out_data;
    assign data_ext_out_data = data_ram_out_data;

    BlockRam #(.WORD_WIDTH(32), .ADDRESS_WIDTH(16))
        dataRam(
            .clock(clock),
            .address(data_ram_address),
            .write(data_ram_write),
            .in_data(data_ram_in_data),
            .out_data(data_ram_out_data));

endmodule

