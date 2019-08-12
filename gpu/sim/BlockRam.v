
// Single-ported block RAM.
module BlockRam
    #(parameter WORD_WIDTH=32, ADDRESS_WIDTH) 
(
    input wire clock,
    input wire [ADDRESS_WIDTH-1:0] address,
    input wire write,
    input wire [WORD_WIDTH-1:0] in_data,
    output wire [WORD_WIDTH-1:0] out_data
);

    reg [WORD_WIDTH-1:0] memory [2**ADDRESS_WIDTH-1:0];

    assign out_data = write ? in_data : memory[address];

    always @(posedge clock) begin
        if (write) begin
            memory[address] <= in_data;
        end
    end

endmodule

