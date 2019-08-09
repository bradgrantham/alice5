
module Main(
    input wire clock,
    output wire led,

    input wire [15:0] ext_address,
    input wire ext_write,
    input wire [31:0] ext_in_data,
    output wire [31:0] ext_out_data
);

    // Toggle LED.
    reg [2:0] counter;
    always @(posedge clock) begin
        counter <= counter + 1'b1;
    end
    assign led = counter[2];

    // Connect block RAM to our own module's parameters.
    wire [15:0] block_ram_address = ext_address;
    wire [31:0] block_ram_in_data = ext_in_data;
    wire block_ram_write = ext_write;
    wire [31:0] block_ram_out_data;
    assign ext_out_data = block_ram_out_data;

    BlockRam #(.WORD_WIDTH(32), .ADDRESS_WIDTH(16))
        blockRam(
            .clock(clock),
            .address(block_ram_address),
            .write(block_ram_write),
            .in_data(block_ram_in_data),
            .out_data(block_ram_out_data));

endmodule

