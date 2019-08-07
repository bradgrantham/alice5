
module Main(
    input clock,
    output led
);

    reg [2:0] counter;
    always @(posedge clock) begin
        counter <= counter + 1'b1;
    end
    assign led = counter[2];

endmodule

