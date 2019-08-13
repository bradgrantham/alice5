
// Simple dual-ported block RAM.
module BlockRam
    #(parameter WORD_WIDTH=32, ADDRESS_WIDTH) 
(
    input wire clock,

    // For writing:
    input wire [ADDRESS_WIDTH-1:0] write_address,
    input wire write,
    input wire [WORD_WIDTH-1:0] write_data,

    // For reading:
    input wire [ADDRESS_WIDTH-1:0] read_address,
    input wire read,
    output reg [WORD_WIDTH-1:0] read_data
);

    reg [WORD_WIDTH-1:0] memory [2**ADDRESS_WIDTH-1:0] /* verilator public */;

    always @(posedge clock) begin
        // Handle writes.
        if (write) begin
            memory[write_address] <= write_data;
        end

        // Handle reads.
        if (read) begin
            if (write && write_address == read_address) begin
                // Carefully read the Cyclone V behavior for read-during-write. It's
                // configurable and complicated. I just did something simple here.
                read_data <= write_data;
            end else begin
                read_data <= memory[read_address];
            end
        end else begin
            // Or just let it latch?
            read_data <= {WORD_WIDTH{1'b0}};
        end
    end

endmodule

