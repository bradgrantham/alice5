
module Main(
    input wire clock,

    input [31:0] sim_h2f_value,
    output [31:0] sim_f2h_value,

    output wire [29:0] sdram_address,
    input wire sdram_waitrequest,
    input wire [WORD_WIDTH-1:0] sdram_readdata,
    input wire sdram_readdatavalid,
    output wire sdram_read,
    output wire [WORD_WIDTH-1:0] sdram_writedata,
    output wire sdram_write
);

    localparam WORD_WIDTH = 32;
    localparam ADDRESS_WIDTH = 16;
    localparam SDRAM_ADDRESS_WIDTH = 24;
    localparam CORE_COUNT = 1;

    // HPS-to-FPGA communication

    wire [31:0] h2f_value;
    wire [31:0] f2h_value;

`ifdef VERILATOR
    assign sim_f2h_value = f2h_value;
    assign h2f_value = sim_h2f_value;
`else
    cyclonev_hps_interface_mpu_general_purpose h2f_gp(
         .gp_in(f2h_value),    // Value to the HPS (continuous).
         .gp_out(h2f_value)    // Value from the HPS (latched).
    );
`endif

    wire reset_n;
    wire run;
    wire gpu_halted;
    wire exception;
    wire [23:0] exception_data;

    assign sdram_address = (sdram_gpu_address + 30'h3E000000) >> 2;
    wire [SDRAM_ADDRESS_WIDTH-1:0] sdram_gpu_address;

    GPU #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        gpu(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),

            .halted(gpu_halted),
            .exception(exception),
            .exception_data(exception_data),

            .h2f_value(h2f_value),
            .f2h_value(f2h_value),

            .sdram_address(sdram_gpu_address),
            .sdram_waitrequest(sdram_waitrequest),
            .sdram_readdata(sdram_readdata),
            .sdram_readdatavalid(sdram_readdatavalid),
            .sdram_read(sdram_read),
            .sdram_writedata(sdram_writedata),
            .sdram_write(sdram_write),

            .mem_request(mem_request[0]),
            .mem_authorized(mem_authorized[0])
            );

    // Memory arbiter.
    wire [CORE_COUNT-1:0] mem_request;
    wire [CORE_COUNT-1:0] mem_authorized;
    MemArb #(.COUNT(CORE_COUNT)) memArb(
        .clock(clock),
        .reset_n(reset_n),
        .request(mem_request),
        .authorized(mem_authorized));

endmodule
