// Created by altera_lib_lpm.pl from altera_mf.v

// START_MODULE_NAME------------------------------------------------------------
//
// Module Name     : ALTSYNCRAM
//
// Description     : Synchronous ram model for Stratix series family
//
// Limitation      :
//
// END_MODULE_NAME--------------------------------------------------------------

`timescale 1 ps / 1 ps

// BEGINNING OF MODULE

// MODULE DECLARATION

/*verilator lint_off CASEX*/
/*verilator lint_off COMBDLY*/
/*verilator lint_off INITIALDLY*/
/*verilator lint_off LITENDIAN*/
/*verilator lint_off MULTIDRIVEN*/
/*verilator lint_off UNOPTFLAT*/
/*verilator lint_off BLKANDNBLK*/
module altsyncram   (
                    wren_a,
                    wren_b,
                    rden_a,
                    rden_b,
                    data_a,
                    data_b,
                    address_a,
                    address_b,
                    clock0,
                    clock1,
                    clocken0,
                    clocken1,
                    clocken2,
                    clocken3,
                    aclr0,
                    aclr1,
                    byteena_a,
                    byteena_b,
                    addressstall_a,
                    addressstall_b,
                    q_a,
                    q_b,
                    eccstatus
                    );

// GLOBAL PARAMETER DECLARATION

    // PORT A PARAMETERS
    parameter width_a          = 1;
    parameter widthad_a        = 1;
    parameter numwords_a       = 0;
    parameter outdata_reg_a    = "UNREGISTERED";
    parameter address_aclr_a   = "NONE";
    parameter outdata_aclr_a   = "NONE";
    parameter indata_aclr_a    = "NONE";
    parameter wrcontrol_aclr_a = "NONE";
    parameter byteena_aclr_a   = "NONE";
    parameter width_byteena_a  = 1;

    // PORT B PARAMETERS
    parameter width_b                   = 1;
    parameter widthad_b                 = 1;
    parameter numwords_b                = 0;
    parameter rdcontrol_reg_b           = "CLOCK1";
    parameter address_reg_b             = "CLOCK1";
    parameter outdata_reg_b             = "UNREGISTERED";
    parameter outdata_aclr_b            = "NONE";
    parameter rdcontrol_aclr_b          = "NONE";
    parameter indata_reg_b              = "CLOCK1";
    parameter wrcontrol_wraddress_reg_b = "CLOCK1";
    parameter byteena_reg_b             = "CLOCK1";
    parameter indata_aclr_b             = "NONE";
    parameter wrcontrol_aclr_b          = "NONE";
    parameter address_aclr_b            = "NONE";
    parameter byteena_aclr_b            = "NONE";
    parameter width_byteena_b           = 1;

    // STRATIX II RELATED PARAMETERS
    parameter clock_enable_input_a  = "NORMAL";
    parameter clock_enable_output_a = "NORMAL";
    parameter clock_enable_input_b  = "NORMAL";
    parameter clock_enable_output_b = "NORMAL";

    parameter clock_enable_core_a = "USE_INPUT_CLKEN";
    parameter clock_enable_core_b = "USE_INPUT_CLKEN";
    parameter read_during_write_mode_port_a = "NEW_DATA_NO_NBE_READ";
    parameter read_during_write_mode_port_b = "NEW_DATA_NO_NBE_READ";

    // ECC STATUS RELATED PARAMETERS
    parameter enable_ecc = "FALSE";
    parameter width_eccstatus = 3;
	parameter ecc_pipeline_stage_enabled = "FALSE";

    // GLOBAL PARAMETERS
    parameter operation_mode                     = "BIDIR_DUAL_PORT";
    parameter byte_size                          = 0;
    parameter read_during_write_mode_mixed_ports = "DONT_CARE";
    parameter ram_block_type                     = "AUTO";
    parameter init_file                          = "UNUSED";
    parameter init_file_layout                   = "UNUSED";
    parameter maximum_depth                      = 0;
    parameter intended_device_family             = "Stratix";

    parameter lpm_hint                           = "UNUSED";
    parameter lpm_type                           = "altsyncram";
    parameter implement_in_les                   = "OFF";
    parameter power_up_uninitialized             = "FALSE";

//Simulation model selection sim-only parameter
	parameter family_arria10 = ((intended_device_family == "Arria 10") || (intended_device_family == "ARRIA 10") || (intended_device_family == "arria 10") || (intended_device_family == "Arria10") || (intended_device_family == "ARRIA10") || (intended_device_family == "arria10")) ? 1 : 0;

	input  wren_a;
    input  wren_b;
    input  rden_a;
    input  rden_b;
    input  [width_a-1:0] data_a;
    input  [width_b-1:0] data_b;
    input  [widthad_a-1:0] address_a;
    input  [widthad_b-1:0] address_b;
    input  clock0;
    input  clock1;
    input  clocken0;
    input  clocken1;
    input  clocken2;
    input  clocken3;
    input  aclr0;
    input  aclr1;
    input [width_byteena_a-1:0] byteena_a;
    input [width_byteena_b-1:0] byteena_b;
    input addressstall_a;
    input addressstall_b;
    output [width_a-1:0] q_a;
    output [width_b-1:0] q_b;
    output [width_eccstatus-1:0] eccstatus;


// INTERNAL TRI DECLARATION

    logic wren_a; // -- converted tristate to logic
    logic wren_b; // -- converted tristate to logic
    logic rden_a; // -- converted tristate to logic
    logic rden_b; // -- converted tristate to logic
    logic clock0; // -- converted tristate to logic
    logic clocken0; // -- converted tristate to logic
    logic clocken1; // -- converted tristate to logic
    logic clocken2; // -- converted tristate to logic
    logic clocken3; // -- converted tristate to logic
    logic aclr0; // -- converted tristate to logic
    logic aclr1; // -- converted tristate to logic
    logic addressstall_a; // -- converted tristate to logic
    logic addressstall_b; // -- converted tristate to logic
    logic [width_byteena_a-1:0] byteena_a; // -- converted tristate to logic
    logic [width_byteena_b-1:0] byteena_b; // -- converted tristate to logic

generate
if(family_arria10)begin:m_arria10
	altera_syncram_derived   altera_syncram_inst(
                    .wren_a(wren_a),
                    .wren_b(wren_b),
                    .rden_a(rden_a),
                    .rden_b(rden_b),
                    .data_a(data_a),
                    .data_b(data_b),
                    .address_a(address_a),
                    .address_b(address_b),
                    .clock0(clock0),
                    .clock1(clock1),
                    .clocken0(clocken0),
                    .clocken1(clocken1),
                    .clocken2(clocken2),
                    .clocken3(clocken3),
                    .aclr0(aclr0),
                    .aclr1(aclr1),
                    .byteena_a(byteena_a),
                    .byteena_b(byteena_b),
                    .addressstall_a(addressstall_a),
                    .addressstall_b(addressstall_b),
                    .q_a(q_a),
                    .q_b(q_b),
		    .eccstatus(eccstatus),
		    .address2_a(1'b1),
		    .address2_b(1'b1),
		    .eccencparity({8{1'b1}}),
		    .eccencbypass(1'b0),
		    .sclr(1'b0)
                   );

	defparam
		altera_syncram_inst.width_a          					= width_a,
		altera_syncram_inst.widthad_a        					= widthad_a,
		altera_syncram_inst.numwords_a       					= numwords_a,
		altera_syncram_inst.outdata_reg_a    					= outdata_reg_a,
		altera_syncram_inst.address_aclr_a   					= address_aclr_a,
		altera_syncram_inst.outdata_aclr_a   					= outdata_aclr_a,
		altera_syncram_inst.width_byteena_a  					= width_byteena_a,
		altera_syncram_inst.width_b                   			= width_b,
		altera_syncram_inst.widthad_b                 			= widthad_b,
		altera_syncram_inst.numwords_b                			= numwords_b,
		altera_syncram_inst.rdcontrol_reg_b           			= rdcontrol_reg_b,
		altera_syncram_inst.address_reg_b             			= address_reg_b,
		altera_syncram_inst.outdata_reg_b             			= outdata_reg_b,
		altera_syncram_inst.outdata_aclr_b            			= outdata_aclr_b,
		altera_syncram_inst.indata_reg_b              			= indata_reg_b,
		altera_syncram_inst.wrcontrol_wraddress_reg_b 			= wrcontrol_wraddress_reg_b,
		altera_syncram_inst.byteena_reg_b             			= byteena_reg_b,
		altera_syncram_inst.address_aclr_b            			= address_aclr_b,
		altera_syncram_inst.width_byteena_b           			= width_byteena_b,
		altera_syncram_inst.clock_enable_input_a  				= clock_enable_input_a,
		altera_syncram_inst.clock_enable_output_a 				= clock_enable_output_a,
		altera_syncram_inst.clock_enable_input_b  				= clock_enable_input_b,
		altera_syncram_inst.clock_enable_output_b 				= clock_enable_output_b,
		altera_syncram_inst.clock_enable_core_a 				= clock_enable_core_a,
		altera_syncram_inst.clock_enable_core_b 				= clock_enable_core_b,
		altera_syncram_inst.read_during_write_mode_port_a 		= read_during_write_mode_port_a,
		altera_syncram_inst.read_during_write_mode_port_b 		= read_during_write_mode_port_b,
		altera_syncram_inst.enable_ecc 							= enable_ecc,
		altera_syncram_inst.width_eccstatus 					= width_eccstatus,
		altera_syncram_inst.ecc_pipeline_stage_enabled 			= ecc_pipeline_stage_enabled,
		altera_syncram_inst.operation_mode                     	= operation_mode,
		altera_syncram_inst.byte_size                          	= byte_size,
		altera_syncram_inst.read_during_write_mode_mixed_ports 	= read_during_write_mode_mixed_ports,
		altera_syncram_inst.ram_block_type                     	= ram_block_type,
		altera_syncram_inst.init_file                          	= init_file,
		altera_syncram_inst.init_file_layout                   	= init_file_layout,
		altera_syncram_inst.maximum_depth                      	= maximum_depth,
		altera_syncram_inst.intended_device_family             	= "Arria 10",
		altera_syncram_inst.lpm_hint                           	= lpm_hint,
		altera_syncram_inst.lpm_type                           	= lpm_type,
		altera_syncram_inst.implement_in_les                 	= implement_in_les,
		altera_syncram_inst.power_up_uninitialized            	= power_up_uninitialized;
end
else begin:m_default
	altsyncram_body   altsyncram_inst(
                    .wren_a(wren_a),
                    .wren_b(wren_b),
                    .rden_a(rden_a),
                    .rden_b(rden_b),
                    .data_a(data_a),
                    .data_b(data_b),
                    .address_a(address_a),
                    .address_b(address_b),
                    .clock0(clock0),
                    .clock1(clock1),
                    .clocken0(clocken0),
                    .clocken1(clocken1),
                    .clocken2(clocken2),
                    .clocken3(clocken3),
                    .aclr0(aclr0),
                    .aclr1(aclr1),
                    .byteena_a(byteena_a),
                    .byteena_b(byteena_b),
                    .addressstall_a(addressstall_a),
                    .addressstall_b(addressstall_b),
                    .q_a(q_a),
                    .q_b(q_b),
		    .eccstatus(eccstatus)
                   );

	defparam
		altsyncram_inst.width_a          						= width_a,
		altsyncram_inst.widthad_a        						= widthad_a,
		altsyncram_inst.numwords_a       						= numwords_a,
		altsyncram_inst.outdata_reg_a    						= outdata_reg_a,
		altsyncram_inst.address_aclr_a   						= address_aclr_a,
		altsyncram_inst.outdata_aclr_a   						= outdata_aclr_a,
		altsyncram_inst.indata_aclr_a    						= indata_aclr_a,
		altsyncram_inst.wrcontrol_aclr_a 						= wrcontrol_aclr_a,
		altsyncram_inst.byteena_aclr_a   						= byteena_aclr_a,
		altsyncram_inst.width_byteena_a  						= width_byteena_a,
		altsyncram_inst.width_b                   				= width_b,
		altsyncram_inst.widthad_b                 				= widthad_b,
		altsyncram_inst.numwords_b                				= numwords_b,
		altsyncram_inst.rdcontrol_reg_b           				= rdcontrol_reg_b,
		altsyncram_inst.address_reg_b             				= address_reg_b,
		altsyncram_inst.outdata_reg_b             				= outdata_reg_b,
		altsyncram_inst.outdata_aclr_b            				= outdata_aclr_b,
		altsyncram_inst.rdcontrol_aclr_b          				= rdcontrol_aclr_b,
		altsyncram_inst.indata_reg_b              				= indata_reg_b,
		altsyncram_inst.wrcontrol_wraddress_reg_b 				= wrcontrol_wraddress_reg_b,
		altsyncram_inst.byteena_reg_b             				= byteena_reg_b,
		altsyncram_inst.indata_aclr_b             				= indata_aclr_b,
		altsyncram_inst.wrcontrol_aclr_b          				= wrcontrol_aclr_b,
		altsyncram_inst.address_aclr_b            				= address_aclr_b,
		altsyncram_inst.byteena_aclr_b            				= byteena_aclr_b,
		altsyncram_inst.width_byteena_b           				= width_byteena_b,
		altsyncram_inst.clock_enable_input_a  					= clock_enable_input_a,
		altsyncram_inst.clock_enable_output_a 					= clock_enable_output_a,
		altsyncram_inst.clock_enable_input_b  					= clock_enable_input_b,
		altsyncram_inst.clock_enable_output_b 					= clock_enable_output_b,
		altsyncram_inst.clock_enable_core_a 					= clock_enable_core_a,
		altsyncram_inst.clock_enable_core_b 					= clock_enable_core_b,
		altsyncram_inst.read_during_write_mode_port_a 			= read_during_write_mode_port_a,
		altsyncram_inst.read_during_write_mode_port_b 			= read_during_write_mode_port_b,
		altsyncram_inst.enable_ecc 								= enable_ecc,
		altsyncram_inst.width_eccstatus 						= width_eccstatus,
		altsyncram_inst.ecc_pipeline_stage_enabled 				= ecc_pipeline_stage_enabled,
		altsyncram_inst.operation_mode                     		= operation_mode,
		altsyncram_inst.byte_size                          		= byte_size,
		altsyncram_inst.read_during_write_mode_mixed_ports 		= read_during_write_mode_mixed_ports,
		altsyncram_inst.ram_block_type                     		= ram_block_type,
		altsyncram_inst.init_file                        	  	= init_file,
		altsyncram_inst.init_file_layout                   		= init_file_layout,
		altsyncram_inst.maximum_depth                   	   	= maximum_depth,
		altsyncram_inst.intended_device_family          	   	= intended_device_family,
		altsyncram_inst.lpm_hint                         	  	= lpm_hint,
		altsyncram_inst.lpm_type                         	  	= lpm_type,
		altsyncram_inst.implement_in_les                 		= implement_in_les,
		altsyncram_inst.power_up_uninitialized            		= power_up_uninitialized;
end
endgenerate

endmodule // ALTSYNCRAM

