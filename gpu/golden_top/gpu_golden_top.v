// ============================================================================
// Copyright (c) 2015 by Terasic Technologies Inc.
// ============================================================================
//
// Permission:
//
//   Terasic grants permission to use and modify this code for use
//   in synthesis for all Terasic Development Boards and Altera Development 
//   Kits made by Terasic.  Other use of this code, including the selling 
//   ,duplication, or modification of any portion is strictly prohibited.
//
// Disclaimer:
//
//   This VHDL/Verilog or C/C++ source code is intended as a design reference
//   which illustrates how these types of functions can be implemented.
//   It is the user's responsibility to verify their design for
//   consistency and functionality through the use of formal
//   verification methods.  Terasic provides no warranty regarding the use 
//   or functionality of this code.
//
// ============================================================================
//           
//  Terasic Technologies Inc
//  9F., No.176, Sec.2, Gongdao 5th Rd, East Dist, Hsinchu City, 30070. Taiwan
//  
//  
//                     web: http://www.terasic.com/  
//                     email: support@terasic.com
//
// ============================================================================
//Date:  Tue Mar  3 15:11:40 2015
// ============================================================================

//`define ENABLE_HPS

module gpu_golden_top(

      ///////// ADC /////////
      output             ADC_CONVST,
      output             ADC_SCK,
      output             ADC_SDI,
      input              ADC_SDO,

      ///////// ARDUINO /////////
      inout       [15:0] ARDUINO_IO,
      inout              ARDUINO_RESET_N,

      ///////// FPGA /////////
      input              FPGA_CLK1_50,
      input              FPGA_CLK2_50,
      input              FPGA_CLK3_50,

      ///////// GPIO /////////
      inout       [35:0] GPIO_0,
      inout       [35:0] GPIO_1,

      ///////// HDMI /////////
      inout              HDMI_I2C_SCL,
      inout              HDMI_I2C_SDA,
      inout              HDMI_I2S,
      inout              HDMI_LRCLK,
      inout              HDMI_MCLK,
      inout              HDMI_SCLK,
      output             HDMI_TX_CLK,
      output      [23:0] HDMI_TX_D,
      output             HDMI_TX_DE,
      output             HDMI_TX_HS,
      input              HDMI_TX_INT,
      output             HDMI_TX_VS,

`ifdef ENABLE_HPS
      ///////// HPS /////////
      inout              HPS_CONV_USB_N,
      output      [14:0] HPS_DDR3_ADDR,
      output      [2:0]  HPS_DDR3_BA,
      output             HPS_DDR3_CAS_N,
      output             HPS_DDR3_CKE,
      output             HPS_DDR3_CK_N,
      output             HPS_DDR3_CK_P,
      output             HPS_DDR3_CS_N,
      output      [3:0]  HPS_DDR3_DM,
      inout       [31:0] HPS_DDR3_DQ,
      inout       [3:0]  HPS_DDR3_DQS_N,
      inout       [3:0]  HPS_DDR3_DQS_P,
      output             HPS_DDR3_ODT,
      output             HPS_DDR3_RAS_N,
      output             HPS_DDR3_RESET_N,
      input              HPS_DDR3_RZQ,
      output             HPS_DDR3_WE_N,
      output             HPS_ENET_GTX_CLK,
      inout              HPS_ENET_INT_N,
      output             HPS_ENET_MDC,
      inout              HPS_ENET_MDIO,
      input              HPS_ENET_RX_CLK,
      input       [3:0]  HPS_ENET_RX_DATA,
      input              HPS_ENET_RX_DV,
      output      [3:0]  HPS_ENET_TX_DATA,
      output             HPS_ENET_TX_EN,
      inout              HPS_GSENSOR_INT,
      inout              HPS_I2C0_SCLK,
      inout              HPS_I2C0_SDAT,
      inout              HPS_I2C1_SCLK,
      inout              HPS_I2C1_SDAT,
      inout              HPS_KEY,
      inout              HPS_LED,
      inout              HPS_LTC_GPIO,
      output             HPS_SD_CLK,
      inout              HPS_SD_CMD,
      inout       [3:0]  HPS_SD_DATA,
      output             HPS_SPIM_CLK,
      input              HPS_SPIM_MISO,
      output             HPS_SPIM_MOSI,
      inout              HPS_SPIM_SS,
      input              HPS_UART_RX,
      output             HPS_UART_TX,
      input              HPS_USB_CLKOUT,
      inout       [7:0]  HPS_USB_DATA,
      input              HPS_USB_DIR,
      input              HPS_USB_NXT,
      output             HPS_USB_STP,
`endif /*ENABLE_HPS*/

      ///////// KEY /////////
      input       [1:0]  KEY,

      ///////// LED /////////
      output      [7:0]  LED,

      ///////// SW /////////
      input       [3:0]  SW
);


//=======================================================
//  REG/WIRE declarations
//=======================================================





//=======================================================
//  Structural coding
//=======================================================
//

    assign HDMI_TX_D = 0;
    assign LED = 0;
    assign ADC_CONVST = 0;
    assign ADC_SCK = 0;
    assign ADC_SDI = 0;
    assign HDMI_TX_CLK = 0;
    assign HDMI_TX_DE = 0;
    assign HDMI_TX_HS = 0;
    assign HDMI_TX_VS = 0;
    assign ARDUINO_IO = 0;
    assign ARDUINO_RESET_N = 0;
    assign ARDUINO_RESET_N = 0;
    assign GPIO_0[35:32] = 0;
    assign GPIO_1[35:32] = 0;
    assign HDMI_I2C_SCL = 0;
    assign HDMI_I2C_SDA = 0;
    assign HDMI_I2S = 0;
    assign HDMI_LRCLK = 0;
    assign HDMI_MCLK = 0;
    assign HDMI_SCLK = 0;

    wire clock = FPGA_CLK1_50;

    localparam WORD_WIDTH = 32;
    localparam ADDRESS_WIDTH = 14;

`ifdef VERILATOR
    assign sim_f2h_value = f2h_value;
    assign h2f_value = sim_h2f_value;
`else
    // cyclonev_hps_interface_mpu_general_purpose h2f_gp(
         // .gp_in(f2h_value),    // Value to the HPS (continuous).
         // .gp_out(h2f_value)    // Value from the HPS (latched).
    // );
    assign GPIO_0[31:0] = 32'bz;
    assign h2f_value = GPIO_0[31:0];
    assign GPIO_1[31:0] = f2h_value;
`endif

    wire [31:0] h2f_value;
    wire [31:0] f2h_value;

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
            .ext_specialreg_output(special_read_data)
            );



endmodule
