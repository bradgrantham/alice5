
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

    wire enable_write_inst_ram;
    wire enable_write_data_ram;
    wire enable_read_inst_ram;
    wire enable_read_data_ram;
    wire enable_read_register;
    wire enable_read_floatreg;
    wire enable_read_special;

    wire [ADDRESS_WIDTH-1:0] rw_address;
    wire [WORD_WIDTH-1:0] write_data;

    wire [WORD_WIDTH - 1:0] inst_ram_read_data;
    wire [WORD_WIDTH - 1:0] data_ram_read_data;
    wire [WORD_WIDTH - 1:0] register_read_data;
    wire [WORD_WIDTH - 1:0] floatreg_read_data;
    wire [WORD_WIDTH - 1:0] special_read_data;

    wire [WORD_WIDTH-1:0] read_data =
        enable_read_inst_ram ? inst_ram_read_data :
        enable_read_data_ram ? data_ram_read_data :
        enable_read_register ? register_read_data :
        enable_read_floatreg ? floatreg_read_data :
        /* enable_read_special ? */ special_read_data;

    wire [29:0] sdram_address = (sdram_gpu_address + 30'h3E000000) >> 2;
    wire [SDRAM_ADDRESS_WIDTH-1:0] sdram_gpu_address;

    GPU32BitInterface #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        gpu_if(
            .clock(clock),

            .h2f_value(h2f_value),
            .f2h_value(f2h_value),

            .reset_n(reset_n),
            .run(run),

            .halted(gpu_halted),
            .exception(exception),
            .exception_data(exception_data),

            .enable_write_inst_ram(enable_write_inst_ram),
            .enable_write_data_ram(enable_write_data_ram),
            .enable_read_inst_ram(enable_read_inst_ram),
            .enable_read_data_ram(enable_read_data_ram),
            .enable_read_register(enable_read_register),
            .enable_read_floatreg(enable_read_floatreg),
            .enable_read_special(enable_read_special),

            .rw_address(rw_address),
            .write_data(write_data),
            .read_data(read_data)
        );

    GPU #(.WORD_WIDTH(WORD_WIDTH), .ADDRESS_WIDTH(ADDRESS_WIDTH))
        gpu(
            .clock(clock),
            .reset_n(reset_n),
            .run(run),

            .halted(gpu_halted),
            .exception(exception),
            .exception_data(exception_data),

            .ext_enable_write_inst_ram(enable_write_inst_ram),
            .ext_inst_ram_address(rw_address),
            .ext_inst_ram_input(write_data),
            .ext_inst_ram_output(inst_ram_read_data),

            .ext_enable_write_data_ram(enable_write_data_ram),
            .ext_data_ram_address(rw_address),
            .ext_data_ram_input(write_data),
            .ext_data_ram_output(data_ram_read_data),

            .ext_register_address(rw_address),
            .ext_register_output(register_read_data),

            .ext_floatreg_address(rw_address),
            .ext_floatreg_output(floatreg_read_data),

            .ext_specialreg_address(rw_address),
            .ext_specialreg_output(special_read_data),

            .sdram_address(sdram_gpu_address),
            .sdram_waitrequest(sdram_waitrequest),
            .sdram_readdata(sdram_readdata),
            .sdram_readdatavalid(sdram_readdatavalid),
            .sdram_read(sdram_read),
            .sdram_writedata(sdram_writedata),
            .sdram_write(sdram_write)
            );

endmodule
