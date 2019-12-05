// Created by altera_lib_lpm.pl from altera_mf.v

/*verilator lint_off CASEX*/
/*verilator lint_off COMBDLY*/
/*verilator lint_off INITIALDLY*/
/*verilator lint_off LITENDIAN*/
/*verilator lint_off MULTIDRIVEN*/
/*verilator lint_off UNOPTFLAT*/
/*verilator lint_off BLKANDNBLK*/
module altsyncram_body   (
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

    parameter implement_in_les                 = "OFF";

    parameter power_up_uninitialized            = "FALSE";

// SIMULATION_ONLY_PARAMETERS_BEGIN

    parameter sim_show_memory_data_in_port_b_layout  = "OFF";

// SIMULATION_ONLY_PARAMETERS_END

// LOCAL_PARAMETERS_BEGIN

    parameter is_lutram = ((ram_block_type == "LUTRAM") || (ram_block_type == "MLAB"))? 1 : 0;

    parameter is_bidir_and_wrcontrol_addb_clk0 =    (((operation_mode == "BIDIR_DUAL_PORT") && (address_reg_b == "CLOCK0"))?
                                                    1 : 0);

    parameter is_bidir_and_wrcontrol_addb_clk1 =    (((operation_mode == "BIDIR_DUAL_PORT") && (address_reg_b == "CLOCK1"))?
                                                    1 : 0);

    parameter check_simultaneous_read_write =   (((operation_mode == "BIDIR_DUAL_PORT") || (operation_mode == "DUAL_PORT")) &&
                                                ((ram_block_type == "M-RAM") ||
                                                    (ram_block_type == "MEGARAM") ||
                                                    ((ram_block_type == "AUTO") && ((read_during_write_mode_mixed_ports == "DONT_CARE") || (read_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE"))) ||
                                                    ((is_lutram == 1) && ((read_during_write_mode_mixed_ports != "OLD_DATA") || (outdata_reg_b == "UNREGISTERED")))))? 1 : 0;

    parameter dual_port_addreg_b_clk0 = (((operation_mode == "DUAL_PORT") && (address_reg_b == "CLOCK0"))? 1: 0);

    parameter dual_port_addreg_b_clk1 = (((operation_mode == "DUAL_PORT") && (address_reg_b == "CLOCK1"))? 1: 0);

    parameter i_byte_size_tmp = (width_byteena_a > 1)? width_a / width_byteena_a : 8;

    parameter i_lutram_read = (((is_lutram == 1) && (read_during_write_mode_port_a == "DONT_CARE")) ||
                                ((is_lutram == 1) && (outdata_reg_a == "UNREGISTERED") && (operation_mode == "SINGLE_PORT")))? 1 : 0;

   parameter enable_mem_data_b_reading =  (sim_show_memory_data_in_port_b_layout == "ON") && ((operation_mode == "BIDIR_DUAL_PORT") || (operation_mode == "DUAL_PORT")) ? 1 : 0;

   parameter family_arriav = ((intended_device_family == "Arria V") || (intended_device_family == "ARRIA V") || (intended_device_family == "arria v") || (intended_device_family == "ArriaV") || (intended_device_family == "ARRIAV") || (intended_device_family == "arriav") || (intended_device_family == "Arria V (GS)") || (intended_device_family == "ARRIA V (GS)") || (intended_device_family == "arria v (gs)") || (intended_device_family == "ArriaV(GS)") || (intended_device_family == "ARRIAV(GS)") || (intended_device_family == "arriav(gs)") || (intended_device_family == "Arria V (GX)") || (intended_device_family == "ARRIA V (GX)") || (intended_device_family == "arria v (gx)") || (intended_device_family == "ArriaV(GX)") || (intended_device_family == "ARRIAV(GX)") || (intended_device_family == "arriav(gx)") || (intended_device_family == "Arria V (GS/GX)") || (intended_device_family == "ARRIA V (GS/GX)") || (intended_device_family == "arria v (gs/gx)") || (intended_device_family == "ArriaV(GS/GX)") || (intended_device_family == "ARRIAV(GS/GX)") || (intended_device_family == "arriav(gs/gx)") || (intended_device_family == "Arria V (GX/GS)") || (intended_device_family == "ARRIA V (GX/GS)") || (intended_device_family == "arria v (gx/gs)") || (intended_device_family == "ArriaV(GX/GS)") || (intended_device_family == "ARRIAV(GX/GS)") || (intended_device_family == "arriav(gx/gs)")) ? 1 : 0;

   parameter family_cyclonev = ((intended_device_family == "Cyclone V") || (intended_device_family == "CYCLONE V") || (intended_device_family == "cyclone v") || (intended_device_family == "CycloneV") || (intended_device_family == "CYCLONEV") || (intended_device_family == "cyclonev") || (intended_device_family == "Cyclone V (GS)") || (intended_device_family == "CYCLONE V (GS)") || (intended_device_family == "cyclone v (gs)") || (intended_device_family == "CycloneV(GS)") || (intended_device_family == "CYCLONEV(GS)") || (intended_device_family == "cyclonev(gs)") || (intended_device_family == "Cyclone V (GX)") || (intended_device_family == "CYCLONE V (GX)") || (intended_device_family == "cyclone v (gx)") || (intended_device_family == "CycloneV(GX)") || (intended_device_family == "CYCLONEV(GX)") || (intended_device_family == "cyclonev(gx)") || (intended_device_family == "Cyclone V (GS/GX)") || (intended_device_family == "CYCLONE V (GS/GX)") || (intended_device_family == "cyclone v (gs/gx)") || (intended_device_family == "CycloneV(GS/GX)") || (intended_device_family == "CYCLONEV(GS/GX)") || (intended_device_family == "cyclonev(gs/gx)") || (intended_device_family == "Cyclone V (GX/GS)") || (intended_device_family == "CYCLONE V (GX/GS)") || (intended_device_family == "cyclone v (gx/gs)") || (intended_device_family == "CycloneV(GX/GS)") || (intended_device_family == "CYCLONEV(GX/GS)") || (intended_device_family == "cyclonev(gx/gs)")) ? 1 : 0;

   parameter family_base_arriav = ((family_arriav == 1) || (family_cyclonev == 1)) ? 1 : 0 ;

   parameter family_arria10 = ((intended_device_family == "Arria 10") || (intended_device_family == "ARRIA 10") || (intended_device_family == "arria 10") || (intended_device_family == "Arria10") || (intended_device_family == "ARRIA10") || (intended_device_family == "arria10")) ? 1 : 0;

   parameter family_stratix10 = ((intended_device_family == "Stratix 10") || (intended_device_family == "STRATIX 10") || (intended_device_family == "stratix 10") || (intended_device_family == "Stratix10") || (intended_device_family == "STRATIX10") || (intended_device_family == "stratix10")) ? 1 : 0;

   parameter family_arriavi = ((intended_device_family == "Arria VI") || (intended_device_family == "ARRIA VI") || (intended_device_family == "arria vi") || (intended_device_family == "ArriaVI") || (intended_device_family == "ARRIAVI") || (intended_device_family == "arriavi") || (intended_device_family == "arria vi")) ? 1 : 0;

   parameter family_nightfury = ((intended_device_family == "Nightfury") || (intended_device_family == "NIGHTFURY") || (intended_device_family == "nightfury") || (intended_device_family == "Night Fury")  || (intended_device_family == "NIGHT FURY") || (intended_device_family == "night fury") || (family_arriavi == 1) || (family_arria10 == 1) || (family_stratix10 == 1)) ? 1 : 0;

   parameter family_arriavgz = ((intended_device_family == "Arria V GZ") || (intended_device_family == "ARRIA V GZ") || (intended_device_family == "arria v gz") || (intended_device_family == "ArriaVGZ")  || (intended_device_family == "ARRIAVGZ")  || (intended_device_family == "arriavgz")) ? 1 : 0;

   parameter family_stratixv = ((intended_device_family == "Stratix V") || (intended_device_family == "STRATIX V") || (intended_device_family == "stratix v") || (intended_device_family == "StratixV") || (intended_device_family == "STRATIXV") || (intended_device_family == "stratixv") || (intended_device_family == "Stratix V (GS)") || (intended_device_family == "STRATIX V (GS)") || (intended_device_family == "stratix v (gs)") || (intended_device_family == "StratixV(GS)") || (intended_device_family == "STRATIXV(GS)") || (intended_device_family == "stratixv(gs)") || (intended_device_family == "Stratix V (GX)") || (intended_device_family == "STRATIX V (GX)") || (intended_device_family == "stratix v (gx)") || (intended_device_family == "StratixV(GX)") || (intended_device_family == "STRATIXV(GX)") || (intended_device_family == "stratixv(gx)") || (intended_device_family == "Stratix V (GS/GX)") || (intended_device_family == "STRATIX V (GS/GX)") || (intended_device_family == "stratix v (gs/gx)") || (intended_device_family == "StratixV(GS/GX)") || (intended_device_family == "STRATIXV(GS/GX)") || (intended_device_family == "stratixv(gs/gx)") || (intended_device_family == "Stratix V (GX/GS)") || (intended_device_family == "STRATIX V (GX/GS)") || (intended_device_family == "stratix v (gx/gs)") || (intended_device_family == "StratixV(GX/GS)") || (intended_device_family == "STRATIXV(GX/GS)") || (intended_device_family == "stratixv(gx/gs)") || (family_base_arriav == 1)  || (family_nightfury == 1) || (family_arriavgz == 1)) ? 1 : 0;

   parameter family_hardcopyiv = ((intended_device_family == "HardCopy IV") || (intended_device_family == "HARDCOPY IV") || (intended_device_family == "hardcopy iv") || (intended_device_family == "HardCopyIV") || (intended_device_family == "HARDCOPYIV") || (intended_device_family == "hardcopyiv") || (intended_device_family == "HardCopy IV (GX)") || (intended_device_family == "HARDCOPY IV (GX)") || (intended_device_family == "hardcopy iv (gx)") || (intended_device_family == "HardCopy IV (E)") || (intended_device_family == "HARDCOPY IV (E)") || (intended_device_family == "hardcopy iv (e)") || (intended_device_family == "HardCopyIV(GX)") || (intended_device_family == "HARDCOPYIV(GX)") || (intended_device_family == "hardcopyiv(gx)") || (intended_device_family == "HardCopyIV(E)") || (intended_device_family == "HARDCOPYIV(E)") || (intended_device_family == "hardcopyiv(e)") || (intended_device_family == "HCXIV") || (intended_device_family == "hcxiv") || (intended_device_family == "HardCopy IV (GX/E)") || (intended_device_family == "HARDCOPY IV (GX/E)") || (intended_device_family == "hardcopy iv (gx/e)") || (intended_device_family == "HardCopy IV (E/GX)") || (intended_device_family == "HARDCOPY IV (E/GX)") || (intended_device_family == "hardcopy iv (e/gx)") || (intended_device_family == "HardCopyIV(GX/E)") || (intended_device_family == "HARDCOPYIV(GX/E)") || (intended_device_family == "hardcopyiv(gx/e)") || (intended_device_family == "HardCopyIV(E/GX)") || (intended_device_family == "HARDCOPYIV(E/GX)") || (intended_device_family == "hardcopyiv(e/gx)")) ? 1 : 0 ;

   parameter family_hardcopyiii = ((intended_device_family == "HardCopy III") || (intended_device_family == "HARDCOPY III") || (intended_device_family == "hardcopy iii") || (intended_device_family == "HardCopyIII") || (intended_device_family == "HARDCOPYIII") || (intended_device_family == "hardcopyiii") || (intended_device_family == "HCX") || (intended_device_family == "hcx")) ? 1 : 0;

   parameter family_hardcopyii = ((intended_device_family == "HardCopy II") || (intended_device_family == "HARDCOPY II") || (intended_device_family == "hardcopy ii") || (intended_device_family == "HardCopyII") || (intended_device_family == "HARDCOPYII") || (intended_device_family == "hardcopyii") || (intended_device_family == "Fusion") || (intended_device_family == "FUSION") || (intended_device_family == "fusion")) ? 1 : 0 ;

   parameter family_arriaiigz = ((intended_device_family == "Arria II GZ") || (intended_device_family == "ARRIA II GZ") || (intended_device_family == "arria ii gz") || (intended_device_family == "ArriaII GZ") || (intended_device_family == "ARRIAII GZ") || (intended_device_family == "arriaii gz") || (intended_device_family == "Arria IIGZ") || (intended_device_family == "ARRIA IIGZ") || (intended_device_family == "arria iigz") || (intended_device_family == "ArriaIIGZ") || (intended_device_family == "ARRIAIIGZ") || (intended_device_family == "arriaii gz")) ? 1 : 0 ;
   parameter family_arriaiigx = ((intended_device_family == "Arria II GX") || (intended_device_family == "ARRIA II GX") || (intended_device_family == "arria ii gx") || (intended_device_family == "ArriaIIGX") || (intended_device_family == "ARRIAIIGX") || (intended_device_family == "arriaiigx") || (intended_device_family == "Arria IIGX") || (intended_device_family == "ARRIA IIGX") || (intended_device_family == "arria iigx") || (intended_device_family == "ArriaII GX") || (intended_device_family == "ARRIAII GX") || (intended_device_family == "arriaii gx") || (intended_device_family == "Arria II") || (intended_device_family == "ARRIA II") || (intended_device_family == "arria ii") || (intended_device_family == "ArriaII") || (intended_device_family == "ARRIAII") || (intended_device_family == "arriaii") || (intended_device_family == "Arria II (GX/E)") || (intended_device_family == "ARRIA II (GX/E)") || (intended_device_family == "arria ii (gx/e)") || (intended_device_family == "ArriaII(GX/E)") || (intended_device_family == "ARRIAII(GX/E)") || (intended_device_family == "arriaii(gx/e)") || (intended_device_family == "PIRANHA") || (intended_device_family == "piranha")) ? 1 : 0 ;

   parameter family_stratixiii = ((intended_device_family == "Stratix III") || (intended_device_family == "STRATIX III") || (intended_device_family == "stratix iii") || (intended_device_family == "StratixIII") || (intended_device_family == "STRATIXIII") || (intended_device_family == "stratixiii") || (intended_device_family == "Titan") || (intended_device_family == "TITAN") || (intended_device_family == "titan") || (intended_device_family == "SIII") || (intended_device_family == "siii") || (intended_device_family == "Stratix IV") || (intended_device_family == "STRATIX IV") || (intended_device_family == "stratix iv") || (intended_device_family == "TGX") || (intended_device_family == "tgx") || (intended_device_family == "StratixIV") || (intended_device_family == "STRATIXIV") || (intended_device_family == "stratixiv") || (intended_device_family == "Stratix IV (GT)") || (intended_device_family == "STRATIX IV (GT)") || (intended_device_family == "stratix iv (gt)") || (intended_device_family == "Stratix IV (GX)") || (intended_device_family == "STRATIX IV (GX)") || (intended_device_family == "stratix iv (gx)") || (intended_device_family == "Stratix IV (E)") || (intended_device_family == "STRATIX IV (E)") || (intended_device_family == "stratix iv (e)") || (intended_device_family == "StratixIV(GT)") || (intended_device_family == "STRATIXIV(GT)") || (intended_device_family == "stratixiv(gt)") || (intended_device_family == "StratixIV(GX)") || (intended_device_family == "STRATIXIV(GX)") || (intended_device_family == "stratixiv(gx)") || (intended_device_family == "StratixIV(E)") || (intended_device_family == "STRATIXIV(E)") || (intended_device_family == "stratixiv(e)") || (intended_device_family == "StratixIIIGX") || (intended_device_family == "STRATIXIIIGX") || (intended_device_family == "stratixiiigx") || (intended_device_family == "Stratix IV (GT/GX/E)") || (intended_device_family == "STRATIX IV (GT/GX/E)") || (intended_device_family == "stratix iv (gt/gx/e)") || (intended_device_family == "Stratix IV (GT/E/GX)") || (intended_device_family == "STRATIX IV (GT/E/GX)") || (intended_device_family == "stratix iv (gt/e/gx)") || (intended_device_family == "Stratix IV (E/GT/GX)") || (intended_device_family == "STRATIX IV (E/GT/GX)") || (intended_device_family == "stratix iv (e/gt/gx)") || (intended_device_family == "Stratix IV (E/GX/GT)") || (intended_device_family == "STRATIX IV (E/GX/GT)") || (intended_device_family == "stratix iv (e/gx/gt)") || (intended_device_family == "StratixIV(GT/GX/E)") || (intended_device_family == "STRATIXIV(GT/GX/E)") || (intended_device_family == "stratixiv(gt/gx/e)") || (intended_device_family == "StratixIV(GT/E/GX)") || (intended_device_family == "STRATIXIV(GT/E/GX)") || (intended_device_family == "stratixiv(gt/e/gx)") || (intended_device_family == "StratixIV(E/GX/GT)") || (intended_device_family == "STRATIXIV(E/GX/GT)") || (intended_device_family == "stratixiv(e/gx/gt)") || (intended_device_family == "StratixIV(E/GT/GX)") || (intended_device_family == "STRATIXIV(E/GT/GX)") || (intended_device_family == "stratixiv(e/gt/gx)") || (intended_device_family == "Stratix IV (GX/E)") || (intended_device_family == "STRATIX IV (GX/E)") || (intended_device_family == "stratix iv (gx/e)") || (intended_device_family == "StratixIV(GX/E)") || (intended_device_family == "STRATIXIV(GX/E)") || (intended_device_family == "stratixiv(gx/e)") || (family_arriaiigx == 1) || (family_hardcopyiv == 1) || (family_hardcopyiii == 1) || (family_stratixv == 1) || (family_arriaiigz == 1) || (family_base_arriav == 1)) ? 1 : 0 ;

   parameter family_zippleback = ((intended_device_family == "MAX 10") || (intended_device_family == "MAX10") || (intended_device_family == "max 10") || (intended_device_family == "max10") || (intended_device_family == "MAX 10 FPGA") || (intended_device_family == "max 10 fpga") || (intended_device_family == "Zippleback") || (intended_device_family == "ZIPPLEBACK")|| (intended_device_family == "zippleback")|| (intended_device_family == "MAX10FPGA")|| (intended_device_family == "max10fpga")|| (intended_device_family == "MAX 10 FPGA (DA/DF/DC/SF/SC)")|| (intended_device_family == "max 10 fpga (da/df/dc/sf/sc)")|| (intended_device_family == "MAX10FPGA(DA/DF/DC/SF/SC)")|| (intended_device_family == "max10fpga(da/df/dc/sf/sc)")|| (intended_device_family == "MAX 10 FPGA (DA)")|| (intended_device_family == "max 10 fpga (da)")|| (intended_device_family == "MAX10FPGA(DA)")|| (intended_device_family == "max10fpga(da)")|| (intended_device_family == "MAX 10 FPGA (DF)")|| (intended_device_family == "max 10 fpga (df)")|| (intended_device_family == "MAX10FPGA(DF)")|| (intended_device_family == "max10fpga(df)")|| (intended_device_family == "MAX 10 FPGA (DC)")|| (intended_device_family == "max 10 fpga (dc)")|| (intended_device_family == "MAX10FPGA(DC)")|| (intended_device_family == "max10fpga(dc)")|| (intended_device_family == "MAX 10 FPGA (SF)")|| (intended_device_family == "max 10 fpga (sf)")|| (intended_device_family == "MAX10FPGA(SF)")|| (intended_device_family == "max10fpga(sf)")|| (intended_device_family == "MAX 10 FPGA (SC)")|| (intended_device_family == "max 10 fpga (sc)")|| (intended_device_family == "MAX10FPGA(SC)")|| (intended_device_family == "max10fpga(sc)")) ? 1 : 0 ;

   parameter family_cycloneiii = ((intended_device_family == "Cyclone III") || (intended_device_family == "CYCLONE III") || (intended_device_family == "cyclone iii") || (intended_device_family == "CycloneIII") || (intended_device_family == "CYCLONEIII") || (intended_device_family == "cycloneiii") || (intended_device_family == "Barracuda") || (intended_device_family == "BARRACUDA") || (intended_device_family == "barracuda") || (intended_device_family == "Cuda") || (intended_device_family == "CUDA") || (intended_device_family == "cuda") || (intended_device_family == "CIII") || (intended_device_family == "ciii") || (intended_device_family == "Cyclone III LS") || (intended_device_family == "CYCLONE III LS") || (intended_device_family == "cyclone iii ls") || (intended_device_family == "CycloneIIILS") || (intended_device_family == "CYCLONEIIILS") || (intended_device_family == "cycloneiiils") || (intended_device_family == "Cyclone III LPS") || (intended_device_family == "CYCLONE III LPS") || (intended_device_family == "cyclone iii lps") || (intended_device_family == "Cyclone LPS") || (intended_device_family == "CYCLONE LPS") || (intended_device_family == "cyclone lps") || (intended_device_family == "CycloneLPS") || (intended_device_family == "CYCLONELPS") || (intended_device_family == "cyclonelps") || (intended_device_family == "Tarpon") || (intended_device_family == "TARPON") || (intended_device_family == "tarpon") || (intended_device_family == "Cyclone IIIE") || (intended_device_family == "CYCLONE IIIE") || (intended_device_family == "cyclone iiie") || (intended_device_family == "Cyclone IV GX") || (intended_device_family == "CYCLONE IV GX") || (intended_device_family == "cyclone iv gx") || (intended_device_family == "Cyclone IVGX") || (intended_device_family == "CYCLONE IVGX") || (intended_device_family == "cyclone ivgx") || (intended_device_family == "CycloneIV GX") || (intended_device_family == "CYCLONEIV GX") || (intended_device_family == "cycloneiv gx") || (intended_device_family == "CycloneIVGX") || (intended_device_family == "CYCLONEIVGX") || (intended_device_family == "cycloneivgx") || (intended_device_family == "Cyclone IV") || (intended_device_family == "CYCLONE IV") || (intended_device_family == "cyclone iv") || (intended_device_family == "CycloneIV") || (intended_device_family == "CYCLONEIV") || (intended_device_family == "cycloneiv") || (intended_device_family == "Cyclone IV (GX)") || (intended_device_family == "CYCLONE IV (GX)") || (intended_device_family == "cyclone iv (gx)") || (intended_device_family == "CycloneIV(GX)") || (intended_device_family == "CYCLONEIV(GX)") || (intended_device_family == "cycloneiv(gx)") || (intended_device_family == "Cyclone III GX") || (intended_device_family == "CYCLONE III GX") || (intended_device_family == "cyclone iii gx") || (intended_device_family == "CycloneIII GX") || (intended_device_family == "CYCLONEIII GX") || (intended_device_family == "cycloneiii gx") || (intended_device_family == "Cyclone IIIGX") || (intended_device_family == "CYCLONE IIIGX") || (intended_device_family == "cyclone iiigx") || (intended_device_family == "CycloneIIIGX") || (intended_device_family == "CYCLONEIIIGX") || (intended_device_family == "cycloneiiigx") || (intended_device_family == "Cyclone III GL") || (intended_device_family == "CYCLONE III GL") || (intended_device_family == "cyclone iii gl") || (intended_device_family == "CycloneIII GL") || (intended_device_family == "CYCLONEIII GL") || (intended_device_family == "cycloneiii gl") || (intended_device_family == "Cyclone IIIGL") || (intended_device_family == "CYCLONE IIIGL") || (intended_device_family == "cyclone iiigl") || (intended_device_family == "CycloneIIIGL") || (intended_device_family == "CYCLONEIIIGL") || (intended_device_family == "cycloneiiigl") || (intended_device_family == "Stingray") || (intended_device_family == "STINGRAY") || (intended_device_family == "stingray") || (intended_device_family == "Cyclone IV E") || (intended_device_family == "CYCLONE IV E") || (intended_device_family == "cyclone iv e") || (intended_device_family == "CycloneIV E") || (intended_device_family == "CYCLONEIV E") || (intended_device_family == "cycloneiv e") || (intended_device_family == "Cyclone IVE") || (intended_device_family == "CYCLONE IVE") || (intended_device_family == "cyclone ive") || (intended_device_family == "CycloneIVE") || (intended_device_family == "CYCLONEIVE") || (intended_device_family == "cycloneive") || (intended_device_family == "Cyclone 10 LP") || (intended_device_family == "CYCLONE 10 LP") || (intended_device_family == "cyclone 10 lp") || (intended_device_family == "Cyclone10 LP") || (intended_device_family == "CYCLONE10 LP") || (intended_device_family == "cyclone10 lp") || (intended_device_family == "Cyclone 10LP") || (intended_device_family == "CYCLONE 10LP") || (intended_device_family == "cyclone 10lp") || (intended_device_family == "Cyclone10LP") || (intended_device_family == "CYCLONE10LP") || (intended_device_family == "cyclone10lp")|| family_zippleback) ? 1 : 0 ;

   parameter family_cyclone = ((intended_device_family == "Cyclone") || (intended_device_family == "CYCLONE") || (intended_device_family == "cyclone") || (intended_device_family == "ACEX2K") || (intended_device_family == "acex2k") || (intended_device_family == "ACEX 2K") || (intended_device_family == "acex 2k") || (intended_device_family == "Tornado") || (intended_device_family == "TORNADO") || (intended_device_family == "tornado")) ? 1 : 0 ;

   parameter family_base_cycloneii = ((intended_device_family == "Cyclone II") || (intended_device_family == "CYCLONE II") || (intended_device_family == "cyclone ii") || (intended_device_family == "Cycloneii") || (intended_device_family == "CYCLONEII") || (intended_device_family == "cycloneii") || (intended_device_family == "Magellan") || (intended_device_family == "MAGELLAN") || (intended_device_family == "magellan")) ? 1 : 0 ;

   parameter family_cycloneii = ((family_base_cycloneii == 1) || (family_cycloneiii == 1)) ? 1 : 0 ;

   parameter family_base_stratix = ((intended_device_family == "Stratix") || (intended_device_family == "STRATIX") || (intended_device_family == "stratix") || (intended_device_family == "Yeager") || (intended_device_family == "YEAGER") || (intended_device_family == "yeager") || (intended_device_family == "Stratix GX") || (intended_device_family == "STRATIX GX") || (intended_device_family == "stratix gx") || (intended_device_family == "Stratix-GX") || (intended_device_family == "STRATIX-GX") || (intended_device_family == "stratix-gx") || (intended_device_family == "StratixGX") || (intended_device_family == "STRATIXGX") || (intended_device_family == "stratixgx") || (intended_device_family == "Aurora") || (intended_device_family == "AURORA") || (intended_device_family == "aurora")) ? 1 : 0 ;

   parameter family_base_stratixii = ((intended_device_family == "Stratix II") || (intended_device_family == "STRATIX II") || (intended_device_family == "stratix ii") || (intended_device_family == "StratixII") || (intended_device_family == "STRATIXII") || (intended_device_family == "stratixii") || (intended_device_family == "Armstrong") || (intended_device_family == "ARMSTRONG") || (intended_device_family == "armstrong") || (intended_device_family == "Stratix II GX") || (intended_device_family == "STRATIX II GX") || (intended_device_family == "stratix ii gx") || (intended_device_family == "StratixIIGX") || (intended_device_family == "STRATIXIIGX") || (intended_device_family == "stratixiigx") || (intended_device_family == "Arria GX") || (intended_device_family == "ARRIA GX") || (intended_device_family == "arria gx") || (intended_device_family == "ArriaGX") || (intended_device_family == "ARRIAGX") || (intended_device_family == "arriagx") || (intended_device_family == "Stratix II GX Lite") || (intended_device_family == "STRATIX II GX LITE") || (intended_device_family == "stratix ii gx lite") || (intended_device_family == "StratixIIGXLite") || (intended_device_family == "STRATIXIIGXLITE") || (intended_device_family == "stratixiigxlite") || (family_hardcopyii == 1)) ? 1 : 0 ;

   parameter family_has_lutram = ((family_stratixiii == 1) || (family_stratixv == 1) || (family_base_arriav == 1) || (family_nightfury == 1)) ? 1 : 0 ;
   parameter family_has_stratixv_style_ram = ((family_base_arriav == 1) || (family_stratixv == 1) || (family_nightfury == 1)) ? 1 : 0 ;
   parameter family_has_stratixiii_style_ram = ((family_stratixiii == 1) || (family_cycloneiii == 1)) ? 1 : 0;

   parameter family_has_m512 = (((intended_device_family == "StratixHC") || (family_base_stratix == 1) || (family_base_stratixii == 1)) && (family_hardcopyii == 0)) ? 1 : 0;

   parameter family_has_megaram = (((intended_device_family == "StratixHC") || (family_base_stratix == 1) || (family_base_stratixii == 1) || (family_stratixiii == 1)) && (family_arriaiigx == 0) && (family_stratixv == 0) && (family_base_arriav == 0)) ? 1 : 0 ;

   parameter family_has_stratixi_style_ram = ((intended_device_family == "StratixHC") || (family_base_stratix == 1) || (family_cyclone == 1)) ? 1 : 0;

   parameter is_write_on_positive_edge = (((ram_block_type == "M-RAM") || (ram_block_type == "MEGARAM")) || (ram_block_type == "M9K") || (ram_block_type == "M20K") || (ram_block_type == "M10K") || (ram_block_type == "M144K") || ((family_has_stratixv_style_ram == 1) && (is_lutram == 1)) || (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) && (ram_block_type == "AUTO"))) ? 1 : 0;

   parameter lutram_single_port_fast_read = ((is_lutram == 1) && ((read_during_write_mode_port_a == "DONT_CARE") || (outdata_reg_a == "UNREGISTERED")) && (operation_mode == "SINGLE_PORT")) ? 1 : 0;

   parameter lutram_dual_port_fast_read = ((is_lutram == 1) && ((read_during_write_mode_mixed_ports == "NEW_DATA") || (read_during_write_mode_mixed_ports == "DONT_CARE") || (read_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE") || ((read_during_write_mode_mixed_ports == "OLD_DATA") && (outdata_reg_b == "UNREGISTERED")))) ? 1 : 0;

   parameter s3_address_aclr_a =  ((family_has_stratixv_style_ram || family_stratixiii) && (is_lutram != 1) && (outdata_reg_a != "CLOCK0") && (outdata_reg_a != "CLOCK1")) ? 1 : 0;

   parameter s3_address_aclr_b =  ((family_has_stratixv_style_ram || family_stratixiii) && (is_lutram != 1) && (outdata_reg_b != "CLOCK0") && (outdata_reg_b != "CLOCK1")) ? 1 : 0;

   parameter i_address_aclr_family_a = ((((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) && (operation_mode != "ROM")) || (family_base_stratixii == 1 || family_base_cycloneii == 1)) ? 1 : 0;

   parameter i_address_aclr_family_b = ((((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) && (operation_mode != "DUAL_PORT")) || ((is_lutram == 1) && (operation_mode == "DUAL_PORT") && (read_during_write_mode_mixed_ports == "OLD_DATA")) || (family_base_stratixii == 1 || family_base_cycloneii == 1)) ? 1 : 0;

// LOCAL_PARAMETERS_END

// INPUT PORT DECLARATION

    input  wren_a; // Port A write/read enable input
    input  wren_b; // Port B write enable input
    input  rden_a; // Port A read enable input
    input  rden_b; // Port B read enable input
    input  [width_a-1:0] data_a; // Port A data input
    input  [width_b-1:0] data_b; // Port B data input
    input  [widthad_a-1:0] address_a; // Port A address input
    input  [widthad_b-1:0] address_b; // Port B address input

    // clock inputs on both ports and here are their usage
    // Port A -- 1. all input registers must be clocked by clock0.
    //           2. output register can be clocked by either clock0, clock1 or none.
    // Port B -- 1. all input registered must be clocked by either clock0 or clock1.
    //           2. output register can be clocked by either clock0, clock1 or none.
    input  clock0;
    input  clock1;

    // clock enable inputs and here are their usage
    // clocken0 -- can only be used for enabling clock0.
    // clocken1 -- can only be used for enabling clock1.
    // clocken2 -- as an alternative for enabling clock0.
    // clocken3 -- as an alternative for enabling clock1.
    input  clocken0;
    input  clocken1;
    input  clocken2;
    input  clocken3;

    // clear inputs on both ports and here are their usage
    // Port A -- 1. all input registers can only be cleared by clear0 or none.
    //           2. output register can be cleared by either clear0, clear1 or none.
    // Port B -- 1. all input registers can be cleared by clear0, clear1 or none.
    //           2. output register can be cleared by either clear0, clear1 or none.
    input  aclr0;
    input  aclr1;

    input [width_byteena_a-1:0] byteena_a; // Port A byte enable input
    input [width_byteena_b-1:0] byteena_b; // Port B byte enable input

    // Stratix II related ports
    input addressstall_a;
    input addressstall_b;



// OUTPUT PORT DECLARATION

    output [width_a-1:0] q_a; // Port A output
    output [width_b-1:0] q_b; // Port B output

    output [width_eccstatus-1:0] eccstatus;   // ECC status flags

// INTERNAL REGISTERS DECLARATION

    reg [width_a-1:0] mem_data [0:(1<<widthad_a)-1];
    reg [width_b-1:0] mem_data_b [0:(1<<widthad_b)-1];
    reg [width_a-1:0] i_data_reg_a;
    reg [width_a-1:0] temp_wa;
    reg [width_a-1:0] temp_wa2;
    reg [width_a-1:0] temp_wa2b;
    reg [width_a-1:0] init_temp;
    reg [width_b-1:0] i_data_reg_b;
    reg [width_b-1:0] temp_wb;
    reg [width_b-1:0] temp_wb2;
    reg temp;
    reg [width_a-1:0] i_q_reg_a;
    reg [width_a-1:0] i_q_tmp_a;
    reg [width_a-1:0] i_q_tmp2_a;
    reg [width_b-1:0] i_q_reg_b;
    reg [width_b-1:0] i_q_tmp_b;
    reg [width_b-1:0] i_q_tmp2_b;
    reg [width_b-1:0] i_q_output_latch;
    reg [width_a-1:0] i_byteena_mask_reg_a;
    reg [width_b-1:0] i_byteena_mask_reg_b;
    reg [widthad_a-1:0] i_address_reg_a;
    reg [widthad_b-1:0] i_address_reg_b;

	reg [width_b-1:0] i_q_ecc_reg_b;
    reg [width_b-1:0] i_q_ecc_tmp_b;

    reg [widthad_a-1:0] i_original_address_a;

    reg [width_a-1:0] i_byteena_mask_reg_a_tmp;
    reg [width_b-1:0] i_byteena_mask_reg_b_tmp;
    reg [width_a-1:0] i_byteena_mask_reg_a_out;
    reg [width_b-1:0] i_byteena_mask_reg_b_out;
    reg [width_a-1:0] i_byteena_mask_reg_a_x;
    reg [width_b-1:0] i_byteena_mask_reg_b_x;
    reg [width_a-1:0] i_byteena_mask_reg_a_out_b;
    reg [width_b-1:0] i_byteena_mask_reg_b_out_a;


    reg [8*256:1] ram_initf;
    reg i_wren_reg_a;
    reg i_wren_reg_b;
    reg i_rden_reg_a;
    reg i_rden_reg_b;
    reg i_read_flag_a;
    reg i_read_flag_b;
    reg i_write_flag_a;
    reg i_write_flag_b;
    reg good_to_go_a;
    reg good_to_go_b;
    reg [31:0] file_desc;
    reg init_file_b_port;
    reg i_nmram_write_a;
    reg i_nmram_write_b;

    reg [width_a - 1: 0] wa_mult_x;
    reg [width_a - 1: 0] wa_mult_x_ii;
    reg [width_a - 1: 0] wa_mult_x_iii;
    reg [widthad_a + width_a - 1:0] add_reg_a_mult_wa;
    reg [widthad_b + width_b -1:0] add_reg_b_mult_wb;
    reg [widthad_a + width_a - 1:0] add_reg_a_mult_wa_pl_wa;
    reg [widthad_b + width_b -1:0] add_reg_b_mult_wb_pl_wb;

    reg same_clock_pulse0;
    reg same_clock_pulse1;

    reg [width_b - 1 : 0] i_original_data_b;
    reg [width_a - 1 : 0] i_original_data_a;

    reg i_address_aclr_a_flag;
    reg i_address_aclr_a_prev;
    reg i_address_aclr_b_flag;
    reg i_address_aclr_b_prev;
    reg i_outdata_aclr_a_prev;
    reg i_outdata_aclr_b_prev;
    reg i_force_reread_a;
    reg i_force_reread_a1;
    reg i_force_reread_b;
    reg i_force_reread_b1;
    reg i_force_reread_a_signal;
    reg i_force_reread_b_signal;

// INTERNAL PARAMETER
    reg [21*8:0] cread_during_write_mode_mixed_ports;
    reg [7*8:0] i_ram_block_type;
    integer i_byte_size;

    wire i_good_to_write_a;
    wire i_good_to_write_b;
    reg i_good_to_write_a2;
    reg i_good_to_write_b2;

    reg i_core_clocken_a_reg;
    reg i_core_clocken0_b_reg;
    reg i_core_clocken1_b_reg;

// INTERNAL WIRE DECLARATIONS

    wire i_indata_aclr_a;
    wire i_address_aclr_a;
    wire i_wrcontrol_aclr_a;
    wire i_indata_aclr_b;
    wire i_address_aclr_b;
    wire i_wrcontrol_aclr_b;
    wire i_outdata_aclr_a;
    wire i_outdata_aclr_b;
    wire i_rdcontrol_aclr_b;
    wire i_byteena_aclr_a;
    wire i_byteena_aclr_b;
    wire i_outdata_clken_a;
    wire i_outdata_clken_b;
    wire i_outlatch_clken_a;
    wire i_outlatch_clken_b;
    wire i_clocken0;
    wire i_clocken1_b;
    wire i_clocken0_b;
    wire i_core_clocken_a;
    wire i_core_clocken_b;
    wire i_core_clocken0_b;
    wire i_core_clocken1_b;

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
    logic [width_byteena_a-1:0] i_byteena_a; // -- converted tristate to logic
    logic [width_byteena_b-1:0] i_byteena_b; // -- converted tristate to logic


// LOCAL INTEGER DECLARATION

    integer i_numwords_a;
    integer i_numwords_b;
    integer i_aclr_flag_a;
    integer i_aclr_flag_b;
    integer i_q_tmp2_a_idx;

    // for loop iterators
    integer init_i;
    integer i;
    integer i2;
    integer i3;
    integer i4;
    integer i5;
    integer j;
    integer j2;
    integer j3;
    integer k;
    integer k2;
    integer k3;
    integer k4;

    // For temporary calculation
    integer i_div_wa;
    integer i_div_wb;
    integer j_plus_i2;
    integer j2_plus_i5;
    integer j3_plus_i5;
    integer j_plus_i2_div_a;
    integer j2_plus_i5_div_a;
    integer j3_plus_i5_div_a;
    integer j3_plus_i5_div_b;
    integer i_byteena_count;
    integer port_a_bit_count_low;
    integer port_a_bit_count_high;
    integer port_b_bit_count_low;
    integer port_b_bit_count_high;

    time i_data_write_time_a;
    time i_data_write_time_b;

    // ------------------------
    // COMPONENT INSTANTIATIONS
    // ------------------------
`ifndef VERILATOR
    ALTERA_DEVICE_FAMILIES dev ();
    ALTERA_MF_MEMORY_INITIALIZATION mem ();
`endif

// INITIAL CONSTRUCT BLOCK

    initial
    begin


        i_numwords_a = (numwords_a != 0) ? numwords_a : (1 << widthad_a);
        i_numwords_b = (numwords_b != 0) ? numwords_b : (1 << widthad_b);

        if (family_has_stratixv_style_ram == 1)
        begin
            if ((((is_lutram == 1) || (ram_block_type == "M10K")) && (family_base_arriav == 1)) ||
                (((is_lutram == 1) || (ram_block_type == "M20K")) && (family_stratixv == 1)))
                i_ram_block_type = ram_block_type;
            else
                i_ram_block_type = "AUTO";
        end
        else if (family_has_stratixiii_style_ram == 1)
        begin
            if ((ram_block_type == "M-RAM") || (ram_block_type == "MEGARAM"))
                i_ram_block_type = "M144K";
            else if ((((ram_block_type == "M144K") || (is_lutram == 1)) && (family_stratixiii == 1)) ||
                    (ram_block_type == "M9K"))
                i_ram_block_type = ram_block_type;
            else
                i_ram_block_type = "AUTO";
        end
        else
        begin
            if ((ram_block_type != "AUTO") &&
                (ram_block_type != "M-RAM") && (ram_block_type != "MEGARAM") &&
                (ram_block_type != "M512") &&
                (ram_block_type != "M4K"))
                i_ram_block_type = "AUTO";
            else
                i_ram_block_type = ram_block_type;
        end

        if ((family_cyclone == 1) || (family_cycloneii == 1))
            cread_during_write_mode_mixed_ports = "OLD_DATA";
        else if (read_during_write_mode_mixed_ports == "UNUSED")
            cread_during_write_mode_mixed_ports = "DONT_CARE";
        else
            cread_during_write_mode_mixed_ports = read_during_write_mode_mixed_ports;

        i_byte_size = (byte_size > 0) ? byte_size
                        : ((((family_has_stratixi_style_ram == 1) || family_cycloneiii == 1) && (i_byte_size_tmp != 8) && (i_byte_size_tmp != 9)) ||
                            (((family_base_stratixii == 1) || (family_base_cycloneii == 1)) && (i_byte_size_tmp != 1) && (i_byte_size_tmp != 2) && (i_byte_size_tmp != 4) && (i_byte_size_tmp != 8) && (i_byte_size_tmp != 9)) ||
                            (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) && (i_byte_size_tmp != 5) && (i_byte_size_tmp !=10) && (i_byte_size_tmp != 8) && (i_byte_size_tmp != 9))) ?
                            8 : i_byte_size_tmp;

        // Parameter Checking
`ifndef VERILATOR
        if ((operation_mode != "BIDIR_DUAL_PORT") && (operation_mode != "SINGLE_PORT") &&
            (operation_mode != "DUAL_PORT") && (operation_mode != "ROM"))
        begin
            $display("Error: Not a valid operation mode.");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((family_base_arriav == 1) &&
            (ram_block_type != "M10K") && (is_lutram != 1) && (ram_block_type != "AUTO"))
        begin
            $display("Warning: RAM_BLOCK_TYPE HAS AN INVALID VALUE. IT CAN ONLY BE M10K, LUTRAM OR AUTO for %s device family. This parameter will take AUTO as its value", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((family_base_arriav != 1) && (family_stratixv == 1) &&
            (ram_block_type != "M20K") && (is_lutram != 1) && (ram_block_type != "AUTO"))
        begin
            $display("Warning: RAM_BLOCK_TYPE HAS AN INVALID VALUE. IT CAN ONLY BE M20K, LUTRAM OR AUTO for %s device family. This parameter will take AUTO as its value", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((family_stratixv != 1) && (family_stratixiii == 1) &&
            (ram_block_type != "M9K") && (ram_block_type != "M144K") && (is_lutram != 1) &&
            (ram_block_type != "AUTO") && (((ram_block_type == "M-RAM") || (ram_block_type == "MEGARAM")) != 1))
        begin
            $display("Warning: RAM_BLOCK_TYPE HAS AN INVALID VALUE. IT CAN ONLY BE M9K, M144K, LUTRAM OR AUTO for %s device family. This parameter will take AUTO as its value", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (i_ram_block_type != ram_block_type)
        begin
            $display("Warning: RAM block type is assumed as %s", i_ram_block_type);
            $display("Time: %0t  Instance: %m", $time);
        end


        if ((cread_during_write_mode_mixed_ports != "DONT_CARE") &&
            (cread_during_write_mode_mixed_ports != "CONSTRAINED_DONT_CARE") &&
            (cread_during_write_mode_mixed_ports != "OLD_DATA") &&
            (cread_during_write_mode_mixed_ports != "NEW_DATA"))
        begin
            $display("Error: Invalid value for read_during_write_mode_mixed_ports parameter. It has to be OLD_DATA or DONT_CARE or CONSTRAINED_DONT_CARE or NEW_DATA");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((cread_during_write_mode_mixed_ports != read_during_write_mode_mixed_ports) && ((operation_mode != "SINGLE_PORT") && (operation_mode != "ROM")))
        begin
            $display("Warning: read_during_write_mode_mixed_ports is assumed as %s", cread_during_write_mode_mixed_ports);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((is_lutram != 1) && (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE"))
        begin
            $display("Warning: read_during_write_mode_mixed_ports cannot be set to CONSTRAINED_DONT_CARE for non-LUTRAM ram block type. This will cause incorrect simulation result.");
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((is_lutram != 1) && (cread_during_write_mode_mixed_ports == "NEW_DATA"))
        begin
            $display("Warning: read_during_write_mode_mixed_ports cannot be set to NEW_DATA for non-LUTRAM ram block type. This will cause incorrect simulation result.");
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((i_ram_block_type == "M-RAM") || (i_ram_block_type == "MEGARAM")) && init_file != "UNUSED")
        begin
            $display("Error: M-RAM block type doesn't support the use of an initialization file");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((i_byte_size != 8) && (i_byte_size != 9) && (family_has_stratixi_style_ram == 1))
        begin
            $display("Error: byte_size HAS TO BE EITHER 8 or 9");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((i_byte_size != 8) && (i_byte_size != 9) && (i_byte_size != 1) &&
            (i_byte_size != 2) && (i_byte_size != 4) &&
            ((family_base_stratixii == 1) || (family_base_cycloneii == 1)))
        begin
            $display("Error: byte_size has to be either 1, 2, 4, 8 or 9 for %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((i_byte_size != 5) && (i_byte_size != 8) && (i_byte_size != 9) && (i_byte_size != 10) &&
            ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)))
        begin
            $display("Error: byte_size has to be either 5,8,9 or 10 for %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (width_a <= 0)
        begin
            $display("Error: Invalid value for WIDTH_A parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((width_b <= 0) &&
            ((operation_mode != "SINGLE_PORT") && (operation_mode != "ROM")))
        begin
            $display("Error: Invalid value for WIDTH_B parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (widthad_a <= 0)
        begin
            $display("Error: Invalid value for WIDTHAD_A parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((widthad_b <= 0) &&
            ((operation_mode != "SINGLE_PORT") && (operation_mode != "ROM")))
        begin
            $display("Error: Invalid value for WIDTHAD_B parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((operation_mode == "ROM") &&
            ((i_ram_block_type == "M-RAM") || (i_ram_block_type == "MEGARAM")))
        begin
            $display("Error: ROM mode does not support RAM_BLOCK_TYPE = M-RAM");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((wrcontrol_aclr_a != "NONE") && (wrcontrol_aclr_a != "UNUSED")) && (i_ram_block_type == "M512") && (operation_mode == "SINGLE_PORT"))
        begin
            $display("Error: Wren_a cannot have clear in single port mode for M512 block");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((operation_mode == "DUAL_PORT") && (i_numwords_a * width_a != i_numwords_b * width_b))
        begin
            $display("Error: Total number of bits of port A and port B should be the same for dual port mode");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((rdcontrol_aclr_b != "NONE") && (rdcontrol_aclr_b != "UNUSED")) && (i_ram_block_type == "M512") && (operation_mode == "DUAL_PORT"))
        begin
            $display("Error: rden_b cannot have clear in simple dual port mode for M512 block");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((operation_mode == "BIDIR_DUAL_PORT") && (i_numwords_a * width_a != i_numwords_b * width_b))
        begin
            $display("Error: Total number of bits of port A and port B should be the same for bidir dual port mode");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((operation_mode == "BIDIR_DUAL_PORT") && (i_ram_block_type == "M512"))
        begin
            $display("Error: M512 block type doesn't support bidir dual mode");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((i_ram_block_type == "M-RAM") || (i_ram_block_type == "MEGARAM")) &&
            (cread_during_write_mode_mixed_ports == "OLD_DATA"))
        begin
            $display("Error: M-RAM doesn't support OLD_DATA value for READ_DURING_WRITE_MODE_MIXED_PORTS parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((family_has_stratixi_style_ram == 1) &&
            (clock_enable_input_a == "BYPASS"))
        begin
            $display("Error: BYPASS value for CLOCK_ENABLE_INPUT_A is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((family_has_stratixi_style_ram == 1) &&
            (clock_enable_output_a == "BYPASS"))
        begin
            $display("Error: BYPASS value for CLOCK_ENABLE_OUTPUT_A is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((family_has_stratixi_style_ram == 1) &&
            (clock_enable_input_b == "BYPASS") &&
			((operation_mode == "BIDIR_DUAL_PORT") || (operation_mode == "DUAL_PORT")))
        begin
            $display("Error: BYPASS value for CLOCK_ENABLE_INPUT_B is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((family_has_stratixi_style_ram == 1) &&
            (clock_enable_output_b == "BYPASS") &&
			((operation_mode == "BIDIR_DUAL_PORT") || (operation_mode == "DUAL_PORT")))
        begin
            $display("Error: BYPASS value for CLOCK_ENABLE_OUTPUT_B is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((implement_in_les != "OFF") && (implement_in_les != "ON"))
        begin
            $display("Error: Illegal value for implement_in_les parameter");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((family_has_m512) == 0) && (i_ram_block_type == "M512"))
        begin
            $display("Error: M512 value for RAM_BLOCK_TYPE parameter is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((family_has_megaram) == 0) &&
            ((i_ram_block_type == "M-RAM") || (i_ram_block_type == "MEGARAM")))
        begin
            $display("Error: MEGARAM value for RAM_BLOCK_TYPE parameter is not supported in %s device family", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((init_file == "UNUSED") || (init_file == "")) &&
            (operation_mode == "ROM"))
        begin
            $display("Error! Altsyncram needs data file for memory initialization in ROM mode.");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((family_base_stratixii == 1) || (family_base_cycloneii == 1)) &&
            (((indata_aclr_a != "UNUSED") && (indata_aclr_a != "NONE")) ||
            ((wrcontrol_aclr_a != "UNUSED") && (wrcontrol_aclr_a != "NONE")) ||
            ((byteena_aclr_a  != "UNUSED") && (byteena_aclr_a != "NONE")) ||
            ((address_aclr_a != "UNUSED") && (address_aclr_a != "NONE") && (operation_mode != "ROM")) ||
            ((indata_aclr_b != "UNUSED") && (indata_aclr_b != "NONE")) ||
            ((rdcontrol_aclr_b != "UNUSED") && (rdcontrol_aclr_b != "NONE")) ||
            ((wrcontrol_aclr_b != "UNUSED") && (wrcontrol_aclr_b != "NONE")) ||
            ((byteena_aclr_b != "UNUSED") && (byteena_aclr_b != "NONE")) ||
            ((address_aclr_b != "UNUSED") && (address_aclr_b != "NONE") && (operation_mode != "DUAL_PORT"))))
        begin
            $display("Warning: %s device family does not support aclr signal on input ports. The aclr to input ports will be ignored.", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) &&
            (((indata_aclr_a != "UNUSED") && (indata_aclr_a != "NONE")) ||
            ((wrcontrol_aclr_a != "UNUSED") && (wrcontrol_aclr_a != "NONE")) ||
            ((byteena_aclr_a  != "UNUSED") && (byteena_aclr_a != "NONE")) ||
            ((address_aclr_a != "UNUSED") && (address_aclr_a != "NONE") && (operation_mode != "ROM")) ||
            ((indata_aclr_b != "UNUSED") && (indata_aclr_b != "NONE")) ||
            ((rdcontrol_aclr_b != "UNUSED") && (rdcontrol_aclr_b != "NONE")) ||
            ((wrcontrol_aclr_b != "UNUSED") && (wrcontrol_aclr_b != "NONE")) ||
            ((byteena_aclr_b != "UNUSED") && (byteena_aclr_b != "NONE")) ||
            ((address_aclr_b != "UNUSED") && (address_aclr_b != "NONE") && (operation_mode != "DUAL_PORT"))))
        begin
            $display("Warning: %s device family does not support aclr signal on input ports. The aclr to input ports will be ignored.", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))
            && (read_during_write_mode_port_a != "NEW_DATA_NO_NBE_READ"))
        begin
            $display("Warning: %s value for read_during_write_mode_port_a is not supported in %s device family, it might cause incorrect behavioural simulation result", read_during_write_mode_port_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))
            && (read_during_write_mode_port_b != "NEW_DATA_NO_NBE_READ"))
        begin
            $display("Warning: %s value for read_during_write_mode_port_b is not supported in %s device family, it might cause incorrect behavioural simulation result", read_during_write_mode_port_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end
// SPR 249576: Enable don't care as RDW setting in MegaFunctions - eliminates checking for ram_block_type = "AUTO"
        if (!((is_lutram == 1) || ((i_ram_block_type == "AUTO") && (family_has_lutram == 1)) ||
            ((i_ram_block_type != "AUTO") && ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)))) &&
            (operation_mode != "SINGLE_PORT") && (read_during_write_mode_port_a == "DONT_CARE"))
        begin
            $display("Error: %s value for read_during_write_mode_port_a is not supported in %s device family for %s ram block type in %s operation_mode",
                read_during_write_mode_port_a, intended_device_family, i_ram_block_type, operation_mode);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((is_lutram != 1) && (i_ram_block_type != "AUTO") &&
            ((read_during_write_mode_mixed_ports == "NEW_DATA") || (read_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE")))
        begin
            $display("Error: %s value for read_during_write_mode_mixed_ports is not supported in %s RAM block type", read_during_write_mode_mixed_ports, i_ram_block_type);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if ((operation_mode == "DUAL_PORT") && (outdata_reg_b != "CLOCK0") && (is_lutram == 1) && (read_during_write_mode_mixed_ports == "OLD_DATA"))
        begin
            $display("Warning: Value for read_during_write_mode_mixed_ports of instance is not honoured in DUAL PORT operation mode when output registers are not clocked by clock0 for LUTRAM.");
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((indata_aclr_a != "NONE") && (indata_aclr_a != "UNUSED")))
        begin
            $display("Warning: %s value for indata_aclr_a is not supported in %s device family. The aclr to data_a registers will be ignored.", indata_aclr_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((wrcontrol_aclr_a != "NONE") && (wrcontrol_aclr_a != "UNUSED")))
        begin
            $display("Warning: %s value for wrcontrol_aclr_a is not supported in %s device family. The aclr to write control registers of port A will be ignored.", wrcontrol_aclr_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((byteena_aclr_a != "NONE") && (byteena_aclr_a != "UNUSED")))
        begin
            $display("Warning: %s value for byteena_aclr_a is not supported in %s device family. The aclr to byteena_a registers will be ignored.", byteena_aclr_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((address_aclr_a != "NONE") && (address_aclr_a != "UNUSED")) && (operation_mode != "ROM"))
        begin
            $display("Warning: %s value for address_aclr_a is not supported for write port in %s device family. The aclr to address_a registers will be ignored.", byteena_aclr_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((indata_aclr_b != "NONE") && (indata_aclr_b != "UNUSED")))
        begin
            $display("Warning: %s value for indata_aclr_b is not supported in %s device family. The aclr to data_b registers will be ignored.", indata_aclr_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((rdcontrol_aclr_b != "NONE") && (rdcontrol_aclr_b != "UNUSED")))
        begin
            $display("Warning: %s value for rdcontrol_aclr_b is not supported in %s device family. The aclr to read control registers will be ignored.", rdcontrol_aclr_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((wrcontrol_aclr_b != "NONE") && (wrcontrol_aclr_b != "UNUSED")))
        begin
            $display("Warning: %s value for wrcontrol_aclr_b is not supported in %s device family. The aclr to write control registers will be ignored.", wrcontrol_aclr_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((byteena_aclr_b != "NONE") && (byteena_aclr_b != "UNUSED")))
        begin
            $display("Warning: %s value for byteena_aclr_b is not supported in %s device family. The aclr to byteena_a register will be ignored.", byteena_aclr_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
            && ((address_aclr_b != "NONE") && (address_aclr_b != "UNUSED")) && (operation_mode == "BIDIR_DUAL_PORT"))
        begin
            $display("Warning: %s value for address_aclr_b is not supported for write port in %s device family. The aclr to address_b registers will be ignored.", address_aclr_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((is_lutram == 1) && (read_during_write_mode_mixed_ports == "OLD_DATA")
            && ((address_aclr_b != "NONE") && (address_aclr_b != "UNUSED")) && (operation_mode == "DUAL_PORT"))
        begin
            $display("Warning : aclr signal for address_b is ignored for RAM block type %s when read_during_write_mode_mixed_ports is set to OLD_DATA", ram_block_type);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1)))
            && ((clock_enable_core_a != clock_enable_input_a) && (clock_enable_core_a != "USE_INPUT_CLKEN")))
        begin
            $display("Warning: clock_enable_core_a value must be USE_INPUT_CLKEN or same as clock_enable_input_a in %s device family. It will be set to clock_enable_input_a value.", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if ((((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1)))
            && ((clock_enable_core_b != clock_enable_input_b) && (clock_enable_core_b != "USE_INPUT_CLKEN")))
        begin
            $display("Warning: clock_enable_core_b must be USE_INPUT_CLKEN or same as clock_enable_input_b in %s device family. It will be set to clock_enable_input_b value.", intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
        end

        if (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))
            && (clock_enable_input_a == "ALTERNATE"))
        begin
            $display("Error: %s value for clock_enable_input_a is not supported in %s device family.", clock_enable_input_a, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))
            && (clock_enable_input_b == "ALTERNATE"))
        begin
            $display("Error: %s value for clock_enable_input_b is not supported in %s device family.", clock_enable_input_b, intended_device_family);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

       if((enable_ecc == "TRUE") && (((i_ram_block_type != "M20K") && (i_ram_block_type != "M144K")) || (operation_mode != "DUAL_PORT")))
        begin
            $display("Error: %s value for enable_ecc is not supported in %s ram block type for %s device family in %s operation mode", enable_ecc, i_ram_block_type, intended_device_family, operation_mode);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

		if ((i_ram_block_type != "M20K") && (ecc_pipeline_stage_enabled == "TRUE"))
        begin
            $display("Error: %s value for ecc_pipeline_stage_enabled is not supported in %s ram block type.", ecc_pipeline_stage_enabled, i_ram_block_type);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

		if ((outdata_reg_b == "UNREGISTERED") && (ecc_pipeline_stage_enabled == "TRUE"))
        begin
            $display("Error: %s value for ecc_pipeline_stage_enabled is not supported when output_reg_b is set to %s.", ecc_pipeline_stage_enabled, outdata_reg_b);
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

		//Setting this to only warning because in synthesis it will ignore the ecc_pipeline_stage_enabled parameter when enable_ecc is set to false
		if((ecc_pipeline_stage_enabled == "TRUE") && (enable_ecc != "TRUE"))
		begin
            $display("Warning: %s value for ecc_pipeline_stage_enabled is not supported when enable_ecc is set to %s", ecc_pipeline_stage_enabled, enable_ecc);
            $display("Time: %0t  Instance: %m", $time);
		end

        if (((i_ram_block_type == "M20K") || (i_ram_block_type == "M144K")) && (enable_ecc == "TRUE") && (read_during_write_mode_mixed_ports == "OLD_DATA"))
        begin
            $display("Error : ECC is not supported for read-before-write mode.");
            $display("Time: %0t  Instance: %m", $time);
            $finish;
        end

        if (operation_mode != "DUAL_PORT")
        begin
            if ((outdata_reg_a != "CLOCK0") && (outdata_reg_a != "CLOCK1") && (outdata_reg_a != "UNUSED")  && (outdata_reg_a != "UNREGISTERED"))
            begin
                $display("Error: %s value for outdata_reg_a is not supported.", outdata_reg_a);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end
        end

        if ((operation_mode == "BIDIR_DUAL_PORT") || (operation_mode == "DUAL_PORT"))
        begin
            if ((address_reg_b != "CLOCK0") && (address_reg_b != "CLOCK1") && (address_reg_b != "UNUSED"))
            begin
                $display("Error: %s value for address_reg_b is not supported.", address_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end

            if ((outdata_reg_b != "CLOCK0") && (outdata_reg_b != "CLOCK1") && (outdata_reg_b != "UNUSED") && (outdata_reg_b != "UNREGISTERED"))
            begin
                $display("Error: %s value for outdata_reg_b is not supported.", outdata_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end

            if ((rdcontrol_reg_b != "CLOCK0") && (rdcontrol_reg_b != "CLOCK1") && (rdcontrol_reg_b != "UNUSED") && (operation_mode == "DUAL_PORT"))
            begin
                $display("Error: %s value for rdcontrol_reg_b is not supported.", rdcontrol_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end

            if ((indata_reg_b != "CLOCK0") && (indata_reg_b != "CLOCK1") && (indata_reg_b != "UNUSED") && (operation_mode == "BIDIR_DUAL_PORT"))
            begin
                $display("Error: %s value for indata_reg_b is not supported.", indata_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end

            if ((wrcontrol_wraddress_reg_b != "CLOCK0") && (wrcontrol_wraddress_reg_b != "CLOCK1") && (wrcontrol_wraddress_reg_b != "UNUSED") && (operation_mode == "BIDIR_DUAL_PORT"))
            begin
                $display("Error: %s value for wrcontrol_wraddress_reg_b is not supported.", wrcontrol_wraddress_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end

            if ((byteena_reg_b != "CLOCK0") && (byteena_reg_b != "CLOCK1") && (byteena_reg_b != "UNUSED") && (operation_mode == "BIDIR_DUAL_PORT"))
            begin
                $display("Error: %s value for byteena_reg_b is not supported.", byteena_reg_b);
                $display("Time: %0t  Instance: %m", $time);
                $finish;
            end
        end
`endif

        // *****************************************
        // legal operations for all operation modes:
        //      |  PORT A  |  PORT B  |
        //      |  RD  WR  |  RD  WR  |
        // BDP  |  x   x   |  x   x   |
        // DP   |      x   |  x       |
        // SP   |  x   x   |          |
        // ROM  |  x       |          |
        // *****************************************


        // Initialize mem_data

        if ((init_file == "UNUSED") || (init_file == ""))
        begin
            if (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) && (family_hardcopyiii == 0) && (family_hardcopyiv == 0) && (power_up_uninitialized != "TRUE"))
            begin
                wa_mult_x = {width_a{1'b0}};
                for (i = 0; i < (1 << widthad_a); i = i + 1)
                    mem_data[i] = wa_mult_x;

                if (enable_mem_data_b_reading)
                begin
                    for (i = 0; i < (1 << widthad_b); i = i + 1)
                        mem_data_b[i] = {width_b{1'b0}};
                end

            end
            else if (((i_ram_block_type == "M-RAM") ||
                (i_ram_block_type == "MEGARAM") ||
                ((i_ram_block_type == "AUTO") && ((cread_during_write_mode_mixed_ports == "DONT_CARE") || (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE"))) ||
                (family_hardcopyii == 1) ||
                (family_hardcopyiii == 1) ||
                (family_hardcopyiv == 1) ||
                (power_up_uninitialized == "TRUE") ) && (implement_in_les == "OFF"))
            begin
                wa_mult_x = {width_a{1'b0 /* converted x or z to 1'b0 */}};
                for (i = 0; i < (1 << widthad_a); i = i + 1)
                    mem_data[i] = wa_mult_x;

                if (enable_mem_data_b_reading)
                begin
                    for (i = 0; i < (1 << widthad_b); i = i + 1)
                    mem_data_b[i] = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                end
            end
            else
            begin
                wa_mult_x = {width_a{1'b0}};
                for (i = 0; i < (1 << widthad_a); i = i + 1)
                    mem_data[i] = wa_mult_x;

                if (enable_mem_data_b_reading)
                begin
                    for (i = 0; i < (1 << widthad_b); i = i + 1)
                    mem_data_b[i] = {width_b{1'b0}};
                end
            end
        end

        else  // Memory initialization file is used
        begin

`ifndef VERILATOR
            wa_mult_x = {width_a{1'b0}};
            for (i = 0; i < (1 << widthad_a); i = i + 1)
                mem_data[i] = wa_mult_x;

            for (i = 0; i < (1 << widthad_b); i = i + 1)
                mem_data_b[i] = {width_b{1'b0}};

            init_file_b_port = 0;

            if ((init_file_layout != "PORT_A") &&
                (init_file_layout != "PORT_B"))
            begin
                if (operation_mode == "DUAL_PORT")
                    init_file_b_port = 1;
                else
                    init_file_b_port = 0;
            end
            else
            begin
                if (init_file_layout == "PORT_A")
                    init_file_b_port = 0;
                else if (init_file_layout == "PORT_B")
                    init_file_b_port = 1;
            end

            if (init_file_b_port)
            begin
                 mem.convert_to_ver_file(init_file, width_b, ram_initf);
                 $readmemh(ram_initf, mem_data_b);

                for (i = 0; i < (i_numwords_b * width_b); i = i + 1)
                begin
                    temp_wb = mem_data_b[i / width_b];
                    i_div_wa = i / width_a;
                    temp_wa = mem_data[i_div_wa];
                    temp_wa[i % width_a] = temp_wb[i % width_b];
                    mem_data[i_div_wa] = temp_wa;
                end
            end
            else
            begin
                mem.convert_to_ver_file(init_file, width_a, ram_initf);
                $readmemh(ram_initf, mem_data);

                if (enable_mem_data_b_reading)
                begin
                    for (i = 0; i < (i_numwords_a * width_a); i = i + 1)
                    begin
                        temp_wa = mem_data[i / width_a];
                        i_div_wb = i / width_b;
                        temp_wb = mem_data_b[i_div_wb];
                        temp_wb[i % width_b] = temp_wa[i % width_a];
                        mem_data_b[i_div_wb] = temp_wb;
                    end
                end
            end
`endif
        end
        i_nmram_write_a = 0;
        i_nmram_write_b = 0;

        i_aclr_flag_a = 0;
        i_aclr_flag_b = 0;

        i_outdata_aclr_a_prev = 0;
        i_outdata_aclr_b_prev = 0;
        i_address_aclr_a_prev = 0;
        i_address_aclr_b_prev = 0;

        i_force_reread_a = 0;
        i_force_reread_a1 = 0;
        i_force_reread_b = 0;
        i_force_reread_b1 = 0;
        i_force_reread_a_signal = 0;
        i_force_reread_b_signal = 0;

        // Initialize internal registers/signals
        i_data_reg_a = 0;
        i_data_reg_b = 0;
        i_address_reg_a = 0;
        i_address_reg_b = 0;
        i_original_address_a = 0;
        i_wren_reg_a = 0;
        i_wren_reg_b = 0;
        i_read_flag_a = 0;
        i_read_flag_b = 0;
        i_write_flag_a = 0;
        i_write_flag_b = 0;
        i_byteena_mask_reg_a_x = 0;
        i_byteena_mask_reg_b_x = 0;
        i_original_data_b = 0;
        i_original_data_a = 0;
        i_data_write_time_a = 0;
        i_data_write_time_b = 0;
        i_core_clocken_a_reg = 0;
        i_core_clocken0_b_reg = 0;
        i_core_clocken1_b_reg = 0;

        i_byteena_mask_reg_a = (family_has_stratixv_style_ram == 1)? {width_a{1'b0}} : {width_a{1'b1}};
        i_byteena_mask_reg_b = (family_has_stratixv_style_ram == 1)? {width_a{1'b0}} : {width_a{1'b1}};
        i_byteena_mask_reg_a_out = (family_has_stratixv_style_ram == 1)? {width_a{1'b0}} : {width_a{1'b1}};
        i_byteena_mask_reg_b_out = (family_has_stratixv_style_ram == 1)? {width_a{1'b0}} : {width_a{1'b1}};

        if ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
        begin
            i_rden_reg_a = 0;
            i_rden_reg_b = 0;
        end
        else
        begin
            i_rden_reg_a = 1;
            i_rden_reg_b = 1;
        end



        if (((i_ram_block_type == "M-RAM") ||
                (i_ram_block_type == "MEGARAM") ||
                ((i_ram_block_type == "AUTO") && ((cread_during_write_mode_mixed_ports == "DONT_CARE") || (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE")))) &&
                (family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))
        begin
            i_q_tmp_a = {width_a{1'b0 /* converted x or z to 1'b0 */}};
            i_q_tmp_b = {width_b{1'b0 /* converted x or z to 1'b0 */}};
            i_q_tmp2_a = {width_a{1'b0 /* converted x or z to 1'b0 */}};
            i_q_tmp2_b = {width_b{1'b0 /* converted x or z to 1'b0 */}};
            i_q_reg_a = {width_a{1'b0 /* converted x or z to 1'b0 */}};
            i_q_reg_b = {width_b{1'b0 /* converted x or z to 1'b0 */}};
        end
        else
        begin
            if (is_lutram == 1)
            begin
                i_q_tmp_a = mem_data[0];
                i_q_tmp2_a = mem_data[0];

                for (init_i = 0; init_i < width_b; init_i = init_i + 1)
                begin
                    init_temp = mem_data[init_i / width_a];
                    i_q_tmp_b[init_i] = init_temp[init_i % width_a];
                    i_q_tmp2_b[init_i] = init_temp[init_i % width_a];
                end

                i_q_reg_a = 0;
                i_q_reg_b = 0;
                i_q_output_latch = 0;
            end
            else
            begin
                i_q_tmp_a = 0;
                i_q_tmp_b = 0;
                i_q_tmp2_a = 0;
                i_q_tmp2_b = 0;
                i_q_reg_a = 0;
                i_q_reg_b = 0;
            end
        end

        good_to_go_a = 0;
        good_to_go_b = 0;

        same_clock_pulse0 = 1'b0;
        same_clock_pulse1 = 1'b0;

        i_byteena_count = 0;

        if (((family_hardcopyii == 1)) &&
            (ram_block_type == "M4K") && (operation_mode != "SINGLE_PORT"))
        begin
            i_good_to_write_a2 = 0;
            i_good_to_write_b2 = 0;
        end
        else
        begin
            i_good_to_write_a2 = 1;
            i_good_to_write_b2 = 1;
        end

    end


// SIGNAL ASSIGNMENT

    // Clock enable signal assignment

    // port a clock enable assignments:
    assign i_outdata_clken_a              = (clock_enable_output_a == "BYPASS") ?
                                            1'b1 : ((clock_enable_output_a == "ALTERNATE") && (outdata_reg_a == "CLOCK1")) ?
                                            clocken3 : ((clock_enable_output_a == "ALTERNATE") && (outdata_reg_a == "CLOCK0")) ?
                                            clocken2 : (outdata_reg_a == "CLOCK1") ?
                                            clocken1 : (outdata_reg_a == "CLOCK0") ?
                                            clocken0 : 1'b1;
    // port b clock enable assignments:
    assign i_outdata_clken_b              = (clock_enable_output_b == "BYPASS") ?
                                            1'b1 : ((clock_enable_output_b == "ALTERNATE") && (outdata_reg_b == "CLOCK1")) ?
                                            clocken3 : ((clock_enable_output_b == "ALTERNATE") && (outdata_reg_b == "CLOCK0")) ?
                                            clocken2 : (outdata_reg_b == "CLOCK1") ?
                                            clocken1 : (outdata_reg_b == "CLOCK0") ?
                                            clocken0 : 1'b1;

    // port a output latch clock enable assignments:
    assign i_outlatch_clken_a              = ((clock_enable_output_b == "NORMAL") && (outdata_reg_a == "UNREGISTERED") && (outdata_reg_b == "CLOCK0") &&
											(operation_mode == "BIDIR_DUAL_PORT") && (family_has_stratixv_style_ram == 1))?
                                            clocken0 : 1'b1;
    // port b clock enable assignments:
    assign i_outlatch_clken_b              = ((clock_enable_output_a == "NORMAL") && (outdata_reg_b == "UNREGISTERED") &&
											(operation_mode == "BIDIR_DUAL_PORT") && (family_has_stratixv_style_ram == 1))?
											(((address_reg_b == "CLOCK0") && (outdata_reg_a == "CLOCK0")) ? clocken0 :
											(((address_reg_b == "CLOCK1") && (outdata_reg_a == "CLOCK1")) ? clocken1 : 1'b1))
											: 1'b1;

    assign i_clocken0                     = (clock_enable_input_a == "BYPASS") ?
                                            1'b1 : (clock_enable_input_a == "NORMAL") ?
                                            clocken0 : clocken2;

    assign i_clocken0_b                   = (clock_enable_input_b == "BYPASS") ?
                                            1'b1 : (clock_enable_input_b == "NORMAL") ?
                                            clocken0 : clocken2;

    assign i_clocken1_b                   = (clock_enable_input_b == "BYPASS") ?
                                            1'b1 : (clock_enable_input_b == "NORMAL") ?
                                            clocken1 : clocken3;

    assign i_core_clocken_a              = (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))) ?
                                            i_clocken0 : ((clock_enable_core_a == "BYPASS") ?
                                            1'b1 : ((clock_enable_core_a == "USE_INPUT_CLKEN") ?
                                            i_clocken0 : ((clock_enable_core_a == "NORMAL") ?
                                            clocken0 : clocken2)));

    assign i_core_clocken0_b              = (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))) ?
                                            i_clocken0_b : ((clock_enable_core_b == "BYPASS") ?
                                            1'b1 : ((clock_enable_core_b == "USE_INPUT_CLKEN") ?
                                            i_clocken0_b : ((clock_enable_core_b == "NORMAL") ?
                                            clocken0 : clocken2)));

    assign i_core_clocken1_b              = (((family_has_stratixv_style_ram != 1) && (family_has_stratixiii_style_ram != 1))) ?
                                            i_clocken1_b : ((clock_enable_core_b == "BYPASS") ?
                                            1'b1 : ((clock_enable_core_b == "USE_INPUT_CLKEN") ?
                                            i_clocken1_b : ((clock_enable_core_b == "NORMAL") ?
                                            clocken1 : clocken3)));

    assign i_core_clocken_b               = (address_reg_b == "CLOCK0") ?
                                            i_core_clocken0_b : i_core_clocken1_b;

    // Async clear signal assignment

    // port a clear assigments:

    assign i_indata_aclr_a    = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1)) ?
                                1'b0 : ((indata_aclr_a == "CLEAR0") ? aclr0 : 1'b0);
    assign i_address_aclr_a   = (address_aclr_a == "CLEAR0") ? aclr0 : 1'b0;
    assign i_wrcontrol_aclr_a = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1))?
                                1'b0 : ((wrcontrol_aclr_a == "CLEAR0") ? aclr0 : 1'b0);
    assign i_byteena_aclr_a   = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1)) ?
                                1'b0 : ((byteena_aclr_a == "CLEAR0") ?
                                aclr0 : ((byteena_aclr_a == "CLEAR1") ?
                                aclr1 : 1'b0));
    assign i_outdata_aclr_a   = (outdata_aclr_a == "CLEAR0") ?
                                aclr0 : ((outdata_aclr_a == "CLEAR1") ?
                                aclr1 : 1'b0);
    // port b clear assignments:
    assign i_indata_aclr_b    = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1))?
                                1'b0 : ((indata_aclr_b == "CLEAR0") ?
                                aclr0 : ((indata_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0));
    assign i_address_aclr_b   = (address_aclr_b == "CLEAR0") ?
                                aclr0 : ((address_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0);
    assign i_wrcontrol_aclr_b = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1))?
                                1'b0 : ((wrcontrol_aclr_b == "CLEAR0") ?
                                aclr0 : ((wrcontrol_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0));
    assign i_rdcontrol_aclr_b = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1)) ?
                                1'b0 : ((rdcontrol_aclr_b == "CLEAR0") ?
                                aclr0 : ((rdcontrol_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0));
    assign i_byteena_aclr_b   = (((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)) ||
                                (family_base_stratixii == 1 || family_base_cycloneii == 1)) ?
                                1'b0 : ((byteena_aclr_b == "CLEAR0") ?
                                aclr0 : ((byteena_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0));
    assign i_outdata_aclr_b   = (outdata_aclr_b == "CLEAR0") ?
                                aclr0 : ((outdata_aclr_b == "CLEAR1") ?
                                aclr1 : 1'b0);

    assign i_byteena_a = byteena_a;
    assign i_byteena_b = byteena_b;


    // Ready to write setting

    assign i_good_to_write_a = (((is_bidir_and_wrcontrol_addb_clk0 == 1) || (dual_port_addreg_b_clk0 == 1)) && (i_core_clocken0_b) && (~clock0)) ?
                                    1'b1 : (((is_bidir_and_wrcontrol_addb_clk1 == 1) || (dual_port_addreg_b_clk1 == 1)) && (i_core_clocken1_b) && (~clock1)) ?
                                    1'b1 : i_good_to_write_a2;

    assign i_good_to_write_b = ((i_core_clocken0_b) && (~clock0)) ? 1'b1 : i_good_to_write_b2;

    always @(i_good_to_write_a)
    begin
        i_good_to_write_a2 = i_good_to_write_a;
    end

    always @(i_good_to_write_b)
    begin
        i_good_to_write_b2 = i_good_to_write_b;
    end


    // Port A inputs registered : indata, address, byeteena, wren
    // Aclr status flags get updated here for M-RAM ram_block_type

    always @(posedge clock0)
    begin

        if (i_force_reread_a && i_outlatch_clken_a)
        begin
            i_force_reread_a_signal <= ~ i_force_reread_a_signal;
            i_force_reread_a <= 0;
        end

        if (i_force_reread_b && ((is_bidir_and_wrcontrol_addb_clk0 == 1) || (dual_port_addreg_b_clk0 == 1)) && i_outlatch_clken_b)
        begin
            i_force_reread_b_signal <= ~ i_force_reread_b_signal;
            i_force_reread_b <= 0;
        end

        if (clock1)
            same_clock_pulse0 <= 1'b1;
        else
            same_clock_pulse0 <= 1'b0;

        if (i_address_aclr_a && (i_address_aclr_family_a == 0))
            i_address_reg_a <= 0;

        i_core_clocken_a_reg <= i_core_clocken_a;
        i_core_clocken0_b_reg <= i_core_clocken0_b;

        if (i_core_clocken_a)
        begin

            if (i_force_reread_a1)
            begin
                i_force_reread_a_signal <= ~ i_force_reread_a_signal;
                i_force_reread_a1 <= 0;
            end
            i_read_flag_a <= ~ i_read_flag_a;
            if (i_force_reread_b1 && ((is_bidir_and_wrcontrol_addb_clk0 == 1) || (dual_port_addreg_b_clk0 == 1)))
            begin
                i_force_reread_b_signal <= ~ i_force_reread_b_signal;
                i_force_reread_b1 <= 0;
            end
            if (is_write_on_positive_edge == 1)
            begin
                if (i_wren_reg_a || wren_a)
                begin
                    i_write_flag_a <= ~ i_write_flag_a;
                end
                if (operation_mode != "ROM")
                    i_nmram_write_a <= 1'b0;
            end
            else
            begin
                if (operation_mode != "ROM")
                    i_nmram_write_a <= 1'b1;
            end

            if (((family_has_stratixv_style_ram == 1) || (family_stratixiii == 1)) && (is_lutram != 1))
            begin
                good_to_go_a <= 1;

                i_rden_reg_a <= rden_a;

                if (i_wrcontrol_aclr_a)
                    i_wren_reg_a <= 0;
                else
                begin
                    i_wren_reg_a <= wren_a;
                end
            end
        end
        else
            i_nmram_write_a <= 1'b0;

        if (i_core_clocken_b)
            i_address_aclr_b_flag <= 0;

        if (is_lutram)
        begin
            if (i_wrcontrol_aclr_a)
                i_wren_reg_a <= 0;
            else if (i_core_clocken_a)
            begin
                i_wren_reg_a <= wren_a;
            end
        end

        if ((clock_enable_input_a == "BYPASS") ||
            ((clock_enable_input_a == "NORMAL") && clocken0) ||
            ((clock_enable_input_a == "ALTERNATE") && clocken2))
        begin

            // Port A inputs

            if (i_indata_aclr_a)
                i_data_reg_a <= 0;
            else
                i_data_reg_a <= data_a;

            if (i_address_aclr_a && (i_address_aclr_family_a == 0))
                i_address_reg_a <= 0;
            else if (!addressstall_a)
                i_address_reg_a <= address_a;

            if (i_byteena_aclr_a)
            begin
                i_byteena_mask_reg_a <= {width_a{1'b1}};
                i_byteena_mask_reg_a_out <= 0;
                i_byteena_mask_reg_a_x <= 0;
                i_byteena_mask_reg_a_out_b <= {width_a{1'b0 /* converted x or z to 1'b0 */}};
            end
            else
            begin

                if (width_byteena_a == 1)
                begin
                    i_byteena_mask_reg_a <= {width_a{i_byteena_a[0]}};
                    i_byteena_mask_reg_a_out <= (i_byteena_a[0])? {width_a{1'b0}} : {width_a{1'b0 /* converted x or z to 1'b0 */}};
                    i_byteena_mask_reg_a_out_b <= (i_byteena_a[0])? {width_a{1'b0 /* converted x or z to 1'b0 */}} : {width_a{1'b0}};
                    i_byteena_mask_reg_a_x <= ((i_byteena_a[0]) || (i_byteena_a[0] == 1'b0))? {width_a{1'b0}} : {width_a{1'b0 /* converted x or z to 1'b0 */}};
                end
                else
                    for (k = 0; k < width_a; k = k+1)
                    begin
                        i_byteena_mask_reg_a[k] <= i_byteena_a[k/i_byte_size];
                        i_byteena_mask_reg_a_out_b[k] <= (i_byteena_a[k/i_byte_size])? 1'b0 /* converted x or z to 1'b0 */: 1'b0;
                        i_byteena_mask_reg_a_out[k] <= (i_byteena_a[k/i_byte_size])? 1'b0: 1'b0 /* converted x or z to 1'b0 */;
                        i_byteena_mask_reg_a_x[k] <= ((i_byteena_a[k/i_byte_size]) || (i_byteena_a[k/i_byte_size] == 1'b0))? 1'b0: 1'b0 /* converted x or z to 1'b0 */;
                    end

            end

            if (((family_has_stratixv_style_ram == 0) && (family_stratixiii == 0)) ||
                (is_lutram == 1))
            begin
                good_to_go_a <= 1;

                i_rden_reg_a <= rden_a;

                if (i_wrcontrol_aclr_a)
                    i_wren_reg_a <= 0;
                else
                begin
                    i_wren_reg_a <= wren_a;
                end
            end

        end


        if (i_indata_aclr_a)
            i_data_reg_a <= 0;

        if (i_address_aclr_a && (i_address_aclr_family_a == 0))
            i_address_reg_a <= 0;

        if (i_byteena_aclr_a)
        begin
            i_byteena_mask_reg_a <= {width_a{1'b1}};
            i_byteena_mask_reg_a_out <= 0;
            i_byteena_mask_reg_a_x <= 0;
            i_byteena_mask_reg_a_out_b <= {width_a{1'b0 /* converted x or z to 1'b0 */}};
        end


        // Port B

        if (is_bidir_and_wrcontrol_addb_clk0)
        begin

            if (i_core_clocken0_b)
            begin
                if ((family_has_stratixv_style_ram == 1) || (family_stratixiii == 1))
                begin
                    good_to_go_b <= 1;

                    i_rden_reg_b <= rden_b;

                    if (i_wrcontrol_aclr_b)
                        i_wren_reg_b <= 0;
                    else
                    begin
                        i_wren_reg_b <= wren_b;
                    end
                end

                i_read_flag_b <= ~i_read_flag_b;

                if (is_write_on_positive_edge == 1)
                begin
                    if (i_wren_reg_b || wren_b)
                    begin
                        i_write_flag_b <= ~ i_write_flag_b;
                    end
                    i_nmram_write_b <= 1'b0;
                end
                else
                    i_nmram_write_b <= 1'b1;

            end
            else
                i_nmram_write_b <= 1'b0;


            if ((clock_enable_input_b == "BYPASS") ||
                ((clock_enable_input_b == "NORMAL") && clocken0) ||
                ((clock_enable_input_b == "ALTERNATE") && clocken2))
            begin

                // Port B inputs

                if (i_indata_aclr_b)
                    i_data_reg_b <= 0;
                else
                    i_data_reg_b <= data_b;


                if ((family_has_stratixv_style_ram == 0) && (family_stratixiii == 0))
                begin
                    good_to_go_b <= 1;

                    i_rden_reg_b <= rden_b;

                    if (i_wrcontrol_aclr_b)
                        i_wren_reg_b <= 0;
                    else
                    begin
                        i_wren_reg_b <= wren_b;
                    end
                end

                if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                    i_address_reg_b <= 0;
                else if (!addressstall_b)
                    i_address_reg_b <= address_b;

                if (i_byteena_aclr_b)
                begin
                    i_byteena_mask_reg_b <= {width_b{1'b1}};
                    i_byteena_mask_reg_b_out <= 0;
                    i_byteena_mask_reg_b_x <= 0;
                    i_byteena_mask_reg_b_out_a <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
                end
                else
                begin

                    if (width_byteena_b == 1)
                    begin
                        i_byteena_mask_reg_b <= {width_b{i_byteena_b[0]}};
                        i_byteena_mask_reg_b_out_a <= (i_byteena_b[0])? {width_b{1'b0 /* converted x or z to 1'b0 */}} : {width_b{1'b0}};
                        i_byteena_mask_reg_b_out <= (i_byteena_b[0])? {width_b{1'b0}} : {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        i_byteena_mask_reg_b_x <= ((i_byteena_b[0]) || (i_byteena_b[0] == 1'b0))? {width_b{1'b0}} : {width_b{1'b0 /* converted x or z to 1'b0 */}};
                    end
                    else
                        for (k2 = 0; k2 < width_b; k2 = k2 + 1)
                        begin
                            i_byteena_mask_reg_b[k2] <= i_byteena_b[k2/i_byte_size];
                            i_byteena_mask_reg_b_out_a[k2] <= (i_byteena_b[k2/i_byte_size])? 1'b0 /* converted x or z to 1'b0 */ : 1'b0;
                            i_byteena_mask_reg_b_out[k2] <= (i_byteena_b[k2/i_byte_size])? 1'b0 : 1'b0 /* converted x or z to 1'b0 */;
                            i_byteena_mask_reg_b_x[k2] <= ((i_byteena_b[k2/i_byte_size]) || (i_byteena_b[k2/i_byte_size] == 1'b0))? 1'b0 : 1'b0 /* converted x or z to 1'b0 */;
                        end

                end

            end


            if (i_indata_aclr_b)
                i_data_reg_b <= 0;

            if (i_wrcontrol_aclr_b)
                i_wren_reg_b <= 0;

            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

            if (i_byteena_aclr_b)
            begin
                i_byteena_mask_reg_b <= {width_b{1'b1}};
                i_byteena_mask_reg_b_out <= 0;
                i_byteena_mask_reg_b_x <= 0;
                i_byteena_mask_reg_b_out_a <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
            end
        end

        if (dual_port_addreg_b_clk0)
        begin
            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

            if (i_core_clocken0_b)
            begin
                if (((family_has_stratixv_style_ram == 1) || (family_stratixiii == 1)) && !is_lutram)
                begin
                    good_to_go_b <= 1;

                    if (i_rdcontrol_aclr_b)
                        i_rden_reg_b <= 1'b1;
                    else
                        i_rden_reg_b <= rden_b;
                end

                i_read_flag_b <= ~ i_read_flag_b;
            end

            if ((clock_enable_input_b == "BYPASS") ||
                ((clock_enable_input_b == "NORMAL") && clocken0) ||
                ((clock_enable_input_b == "ALTERNATE") && clocken2))
            begin
                if (((family_has_stratixv_style_ram == 0) && (family_stratixiii == 0)) || is_lutram)
                begin
                    good_to_go_b <= 1;

                    if (i_rdcontrol_aclr_b)
                        i_rden_reg_b <= 1'b1;
                    else
                        i_rden_reg_b <= rden_b;
                end

                if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                    i_address_reg_b <= 0;
                else if (!addressstall_b)
                    i_address_reg_b <= address_b;

            end


            if (i_rdcontrol_aclr_b)
                i_rden_reg_b <= 1'b1;

            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

        end

    end


    always @(negedge clock0)
    begin

        if (clock1)
            same_clock_pulse0 <= 1'b0;

        if (is_write_on_positive_edge == 0)
        begin
            if (i_nmram_write_a == 1'b1)
            begin
                i_write_flag_a <= ~ i_write_flag_a;

                if (is_lutram)
                    i_read_flag_a <= ~i_read_flag_a;
            end


            if (is_bidir_and_wrcontrol_addb_clk0)
            begin
                if (i_nmram_write_b == 1'b1)
                    i_write_flag_b <= ~ i_write_flag_b;
            end
        end

        if (i_core_clocken0_b && (lutram_dual_port_fast_read == 1) && (dual_port_addreg_b_clk0 == 1))
        begin
            i_read_flag_b <= ~i_read_flag_b;
        end

    end



    always @(posedge clock1)
    begin
        i_core_clocken1_b_reg <= i_core_clocken1_b;

        if (i_force_reread_b && ((is_bidir_and_wrcontrol_addb_clk1 == 1) || (dual_port_addreg_b_clk1 == 1)))
        begin
            i_force_reread_b_signal <= ~ i_force_reread_b_signal;
            i_force_reread_b <= 0;
        end

        if (clock0)
            same_clock_pulse1 <= 1'b1;
        else
            same_clock_pulse1 <= 1'b0;

        if (i_core_clocken_b)
            i_address_aclr_b_flag <= 0;

        if (is_bidir_and_wrcontrol_addb_clk1)
        begin

            if (i_core_clocken1_b)
            begin
                i_read_flag_b <= ~i_read_flag_b;

                if ((family_has_stratixv_style_ram == 1) || (family_stratixiii == 1))
                begin
                    good_to_go_b <= 1;

                    i_rden_reg_b <= rden_b;

                    if (i_wrcontrol_aclr_b)
                        i_wren_reg_b <= 0;
                    else
                    begin
                        i_wren_reg_b <= wren_b;
                    end
                end

                if (is_write_on_positive_edge == 1)
                begin
                    if (i_wren_reg_b || wren_b)
                    begin
                        i_write_flag_b <= ~ i_write_flag_b;
                    end
                    i_nmram_write_b <= 1'b0;
                end
                else
                    i_nmram_write_b <= 1'b1;
            end
            else
                i_nmram_write_b <= 1'b0;


            if ((clock_enable_input_b == "BYPASS") ||
                ((clock_enable_input_b == "NORMAL") && clocken1) ||
                ((clock_enable_input_b == "ALTERNATE") && clocken3))
            begin

                // Port B inputs

                if (address_reg_b == "CLOCK1")
                begin
                    if (i_indata_aclr_b)
                        i_data_reg_b <= 0;
                    else
                        i_data_reg_b <= data_b;
                end

                if ((family_has_stratixv_style_ram == 0) && (family_stratixiii == 0))
                begin
                    good_to_go_b <= 1;

                    i_rden_reg_b <= rden_b;

                    if (i_wrcontrol_aclr_b)
                        i_wren_reg_b <= 0;
                    else
                    begin
                        i_wren_reg_b <= wren_b;
                    end
                end

                if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                    i_address_reg_b <= 0;
                else if (!addressstall_b)
                    i_address_reg_b <= address_b;

                if (i_byteena_aclr_b)
                begin
                    i_byteena_mask_reg_b <= {width_b{1'b1}};
                    i_byteena_mask_reg_b_out <= 0;
                    i_byteena_mask_reg_b_x <= 0;
                    i_byteena_mask_reg_b_out_a <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
                end
                else
                begin
                    if (width_byteena_b == 1)
                    begin
                        i_byteena_mask_reg_b <= {width_b{i_byteena_b[0]}};
                        i_byteena_mask_reg_b_out_a <= (i_byteena_b[0])? {width_b{1'b0 /* converted x or z to 1'b0 */}} : {width_b{1'b0}};
                        i_byteena_mask_reg_b_out <= (i_byteena_b[0])? {width_b{1'b0}} : {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        i_byteena_mask_reg_b_x <= ((i_byteena_b[0]) || (i_byteena_b[0] == 1'b0))? {width_b{1'b0}} : {width_b{1'b0 /* converted x or z to 1'b0 */}};
                    end
                    else
                        for (k2 = 0; k2 < width_b; k2 = k2 + 1)
                        begin
                            i_byteena_mask_reg_b[k2] <= i_byteena_b[k2/i_byte_size];
                            i_byteena_mask_reg_b_out_a[k2] <= (i_byteena_b[k2/i_byte_size])? 1'b0 /* converted x or z to 1'b0 */ : 1'b0;
                            i_byteena_mask_reg_b_out[k2] <= (i_byteena_b[k2/i_byte_size])? 1'b0 : 1'b0 /* converted x or z to 1'b0 */;
                            i_byteena_mask_reg_b_x[k2] <= ((i_byteena_b[k2/i_byte_size]) || (i_byteena_b[k2/i_byte_size] == 1'b0))? 1'b0 : 1'b0 /* converted x or z to 1'b0 */;
                        end

                end

            end


            if (i_indata_aclr_b)
                i_data_reg_b <= 0;

            if (i_wrcontrol_aclr_b)
                i_wren_reg_b <= 0;

            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

            if (i_byteena_aclr_b)
            begin
                i_byteena_mask_reg_b <= {width_b{1'b1}};
                i_byteena_mask_reg_b_out <= 0;
                i_byteena_mask_reg_b_x <= 0;
                i_byteena_mask_reg_b_out_a <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
            end
        end

        if (dual_port_addreg_b_clk1)
        begin
            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

            if (i_core_clocken1_b)
            begin
                if (i_force_reread_b1)
                begin
                    i_force_reread_b_signal <= ~ i_force_reread_b_signal;
                    i_force_reread_b1 <= 0;
                end
                if (((family_has_stratixv_style_ram == 1) || (family_stratixiii == 1)) && !is_lutram)
                begin
                    good_to_go_b <= 1;

                    if (i_rdcontrol_aclr_b)
                    begin
                        i_rden_reg_b <= 1'b1;
                    end
                    else
                    begin
                        i_rden_reg_b <= rden_b;
                    end
                end

                i_read_flag_b <= ~i_read_flag_b;
            end

            if ((clock_enable_input_b == "BYPASS") ||
                ((clock_enable_input_b == "NORMAL") && clocken1) ||
                ((clock_enable_input_b == "ALTERNATE") && clocken3))
            begin
                if (((family_has_stratixv_style_ram == 0) && (family_stratixiii == 0)) || is_lutram)
                begin
                    good_to_go_b <= 1;

                    if (i_rdcontrol_aclr_b)
                    begin
                        i_rden_reg_b <= 1'b1;
                    end
                    else
                    begin
                        i_rden_reg_b <= rden_b;
                    end
                end

                if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                    i_address_reg_b <= 0;
                else if (!addressstall_b)
                    i_address_reg_b <= address_b;

            end


            if (i_rdcontrol_aclr_b)
                i_rden_reg_b <= 1'b1;

            if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                i_address_reg_b <= 0;

        end

    end

    always @(negedge clock1)
    begin

        if (clock0)
            same_clock_pulse1 <= 1'b0;

        if (is_write_on_positive_edge == 0)
        begin

            if (is_bidir_and_wrcontrol_addb_clk1)
            begin
                if (i_nmram_write_b == 1'b1)
                    i_write_flag_b <= ~ i_write_flag_b;
            end
        end

        if (i_core_clocken1_b && (lutram_dual_port_fast_read == 1) && (dual_port_addreg_b_clk1 ==1))
        begin
            i_read_flag_b <= ~i_read_flag_b;
        end

    end

    always @(posedge i_address_aclr_b)
    begin
        if ((is_lutram == 1) && (operation_mode == "DUAL_PORT") && (i_address_aclr_family_b == 0))
            i_read_flag_b <= ~i_read_flag_b;
    end

    always @(posedge i_address_aclr_a)
    begin
        if ((is_lutram == 1) && (operation_mode == "ROM") && (i_address_aclr_family_a == 0))
            i_read_flag_a <= ~i_read_flag_a;
    end

    always @(posedge i_outdata_aclr_a)
    begin
        if (((family_has_stratixv_style_ram == 1) || (family_cycloneiii == 1)) &&
            ((outdata_reg_a != "CLOCK0") && (outdata_reg_a != "CLOCK1")))
            i_read_flag_a <= ~i_read_flag_a;
    end

    always @(posedge i_outdata_aclr_b)
    begin
        if (((family_has_stratixv_style_ram == 1) || (family_cycloneiii == 1)) &&
            ((outdata_reg_b != "CLOCK0") && (outdata_reg_b != "CLOCK1")))
            i_read_flag_b <= ~i_read_flag_b;
    end

    // Port A writting -------------------------------------------------------------

    always @(posedge i_write_flag_a or negedge i_write_flag_a)
    begin
        if ((operation_mode == "BIDIR_DUAL_PORT") ||
            (operation_mode == "DUAL_PORT") ||
            (operation_mode == "SINGLE_PORT"))
        begin

            if ((i_wren_reg_a) && (i_good_to_write_a))
            begin
                i_aclr_flag_a = 0;

                if (i_indata_aclr_a)
                begin
                    if (i_data_reg_a != 0)
                    begin
                        mem_data[i_address_reg_a] = {width_a{1'b0 /* converted x or z to 1'b0 */}};

                        if (enable_mem_data_b_reading)
                        begin
                            j3 = i_address_reg_a * width_a;
                            for (i5 = 0; i5 < width_a; i5 = i5+1)
                            begin
                                    j3_plus_i5 = j3 + i5;
                                    temp_wb = mem_data_b[j3_plus_i5 / width_b];
                                    temp_wb[j3_plus_i5 % width_b] = {1'b0 /* converted x or z to 1'b0 */};
                                    mem_data_b[j3_plus_i5 / width_b] = temp_wb;
                            end
                        end
                        i_aclr_flag_a = 1;
                    end
                end
                else if (i_byteena_aclr_a)
                begin
                    if (i_byteena_mask_reg_a != {width_a{1'b1}})
                    begin
                        mem_data[i_address_reg_a] = {width_a{1'b0 /* converted x or z to 1'b0 */}};

                        if (enable_mem_data_b_reading)
                        begin
                            j3 = i_address_reg_a * width_a;
                            for (i5 = 0; i5 < width_a; i5 = i5+1)
                            begin
                                    j3_plus_i5 = j3 + i5;
                                    temp_wb = mem_data_b[j3_plus_i5 / width_b];
                                    temp_wb[j3_plus_i5 % width_b] = {1'b0 /* converted x or z to 1'b0 */};
                                    mem_data_b[j3_plus_i5 / width_b] = temp_wb;
                            end
                        end
                        i_aclr_flag_a = 1;
                    end
                end
                else if (i_address_aclr_a && (i_address_aclr_family_a == 0))
                begin
                    if (i_address_reg_a != 0)
                    begin
                        wa_mult_x_ii = {width_a{1'b0 /* converted x or z to 1'b0 */}};
                        for (i4 = 0; i4 < i_numwords_a; i4 = i4 + 1)
                            mem_data[i4] = wa_mult_x_ii;

                        if (enable_mem_data_b_reading)
                        begin
                            for (i4 = 0; i4 < i_numwords_b; i4 = i4 + 1)
                                mem_data_b[i4] = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        end

                        i_aclr_flag_a = 1;
                    end
                end

                if (i_aclr_flag_a == 0)
                begin
                    i_original_data_a = mem_data[i_address_reg_a];
                    i_original_address_a = i_address_reg_a;
                    i_data_write_time_a = $time;
                    temp_wa = mem_data[i_address_reg_a];

                    port_a_bit_count_low = i_address_reg_a * width_a;
                    port_b_bit_count_low = i_address_reg_b * width_b;
                    port_b_bit_count_high = port_b_bit_count_low + width_b;

                    for (i5 = 0; i5 < width_a; i5 = i5 + 1)
                    begin
                        i_byteena_count = port_a_bit_count_low % width_b;

                        if ((port_a_bit_count_low >= port_b_bit_count_low) && (port_a_bit_count_low < port_b_bit_count_high) &&
                            ((i_core_clocken0_b_reg && (is_bidir_and_wrcontrol_addb_clk0 == 1)) || (i_core_clocken1_b_reg && (is_bidir_and_wrcontrol_addb_clk1 == 1))) &&
                            (i_wren_reg_b) && ((same_clock_pulse0 && same_clock_pulse1) || (address_reg_b == "CLOCK0")) &&
                            (i_byteena_mask_reg_b[i_byteena_count]) && (i_byteena_mask_reg_a[i5]))
                            temp_wa[i5] = {1'b0 /* converted x or z to 1'b0 */};
                        else if (i_byteena_mask_reg_a[i5])
                            temp_wa[i5] = i_data_reg_a[i5];

                        if (enable_mem_data_b_reading)
                        begin
                            temp_wb = mem_data_b[port_a_bit_count_low / width_b];
                            temp_wb[port_a_bit_count_low % width_b] = temp_wa[i5];
                            mem_data_b[port_a_bit_count_low / width_b] = temp_wb;
                        end

                        port_a_bit_count_low = port_a_bit_count_low + 1;
                    end

                    mem_data[i_address_reg_a] = temp_wa;

                    if (((cread_during_write_mode_mixed_ports == "OLD_DATA") && (is_write_on_positive_edge == 1) && (address_reg_b == "CLOCK0")) ||
                        ((lutram_dual_port_fast_read == 1) && (operation_mode == "DUAL_PORT")))
                        i_read_flag_b = ~i_read_flag_b;

                    if ((read_during_write_mode_port_a == "OLD_DATA") ||
                        ((is_lutram == 1) && (read_during_write_mode_port_a == "DONT_CARE")))
                        i_read_flag_a = ~i_read_flag_a;
                end

            end
        end
    end    // Port A writting ----------------------------------------------------


    // Port B writting -----------------------------------------------------------

    always @(posedge i_write_flag_b or negedge i_write_flag_b)
    begin
        if (operation_mode == "BIDIR_DUAL_PORT")
        begin

            if ((i_wren_reg_b) && (i_good_to_write_b))
            begin

                i_aclr_flag_b = 0;

                // RAM content is following width_a
                // if Port B is of different width, need to make some adjustments

                if (i_indata_aclr_b)
                begin
                    if (i_data_reg_b != 0)
                    begin
                        if (enable_mem_data_b_reading)
                            mem_data_b[i_address_reg_b] = {width_b{1'b0 /* converted x or z to 1'b0 */}};

                        if (width_a == width_b)
                            mem_data[i_address_reg_b] = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        else
                        begin
                            j = i_address_reg_b * width_b;
                            for (i2 = 0; i2 < width_b; i2 = i2+1)
                            begin
                                    j_plus_i2 = j + i2;
                                    temp_wa = mem_data[j_plus_i2 / width_a];
                                    temp_wa[j_plus_i2 % width_a] = {1'b0 /* converted x or z to 1'b0 */};
                                    mem_data[j_plus_i2 / width_a] = temp_wa;
                            end
                        end
                        i_aclr_flag_b = 1;
                    end
                end
                else if (i_byteena_aclr_b)
                begin
                    if (i_byteena_mask_reg_b != {width_b{1'b1}})
                    begin
                        if (enable_mem_data_b_reading)
                            mem_data_b[i_address_reg_b] = {width_b{1'b0 /* converted x or z to 1'b0 */}};

                        if (width_a == width_b)
                            mem_data[i_address_reg_b] = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        else
                        begin
                            j = i_address_reg_b * width_b;
                            for (i2 = 0; i2 < width_b; i2 = i2+1)
                            begin
                                j_plus_i2 = j + i2;
                                j_plus_i2_div_a = j_plus_i2 / width_a;
                                temp_wa = mem_data[j_plus_i2_div_a];
                                temp_wa[j_plus_i2 % width_a] = {1'b0 /* converted x or z to 1'b0 */};
                                mem_data[j_plus_i2_div_a] = temp_wa;
                            end
                        end
                        i_aclr_flag_b = 1;
                    end
                end
                else if (i_address_aclr_b && (i_address_aclr_family_b == 0))
                begin
                    if (i_address_reg_b != 0)
                    begin

                        if (enable_mem_data_b_reading)
                        begin
                            for (i2 = 0; i2 < i_numwords_b; i2 = i2 + 1)
                            begin
                                mem_data_b[i2] = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                            end
                        end

                        wa_mult_x_iii = {width_a{1'b0 /* converted x or z to 1'b0 */}};
                        for (i2 = 0; i2 < i_numwords_a; i2 = i2 + 1)
                        begin
                            mem_data[i2] = wa_mult_x_iii;
                        end
                        i_aclr_flag_b = 1;
                    end
                end

                if (i_aclr_flag_b == 0)
                begin
                        port_b_bit_count_low = i_address_reg_b * width_b;
                        port_a_bit_count_low = i_address_reg_a * width_a;
                        port_a_bit_count_high = port_a_bit_count_low + width_a;
                        i_data_write_time_b = $time;
                        for (i2 = 0; i2 < width_b; i2 = i2 + 1)
                        begin
                            port_b_bit_count_high = port_b_bit_count_low + i2;
                            temp_wa = mem_data[port_b_bit_count_high / width_a];
                            i_original_data_b[i2] = temp_wa[port_b_bit_count_high % width_a];

                            if ((port_b_bit_count_high >= port_a_bit_count_low) && (port_b_bit_count_high < port_a_bit_count_high) &&
                                ((same_clock_pulse0 && same_clock_pulse1) || (address_reg_b == "CLOCK0")) &&
                                (i_core_clocken_a_reg) && (i_wren_reg_a) &&
                                (i_byteena_mask_reg_a[port_b_bit_count_high % width_a]) && (i_byteena_mask_reg_b[i2]))
                                temp_wa[port_b_bit_count_high % width_a] = {1'b0 /* converted x or z to 1'b0 */};
                            else if (i_byteena_mask_reg_b[i2])
                                temp_wa[port_b_bit_count_high % width_a] = i_data_reg_b[i2];

                            mem_data[port_b_bit_count_high / width_a] = temp_wa;
                            temp_wb[i2] = temp_wa[port_b_bit_count_high % width_a];
                        end

                        if (enable_mem_data_b_reading)
                            mem_data_b[i_address_reg_b] = temp_wb;

                    if ((read_during_write_mode_port_b == "OLD_DATA") && (is_write_on_positive_edge == 1))
                        i_read_flag_b = ~i_read_flag_b;

                    if ((cread_during_write_mode_mixed_ports == "OLD_DATA")&& (address_reg_b == "CLOCK0") && (is_write_on_positive_edge == 1))
                        i_read_flag_a = ~i_read_flag_a;

                end

            end

        end
    end


    // Port A reading

    always @(i_read_flag_a)
    begin
        if ((operation_mode == "BIDIR_DUAL_PORT") ||
            (operation_mode == "SINGLE_PORT") ||
            (operation_mode == "ROM"))
        begin
            if (~good_to_go_a && (is_lutram == 0))
            begin

                if (((i_ram_block_type == "M-RAM") || (i_ram_block_type == "MEGARAM") ||
                        ((i_ram_block_type == "AUTO") && ((cread_during_write_mode_mixed_ports == "DONT_CARE") || (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE")))) &&
                    (operation_mode != "ROM") &&
                    ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0)))
                    i_q_tmp2_a = {width_a{1'b0 /* converted x or z to 1'b0 */}};
                else
                    i_q_tmp2_a = 0;
            end
            else
            begin
                if (i_rden_reg_a)
                begin
                    // read from RAM content or flow through for write cycle
                    if (i_wren_reg_a)
                    begin
                        if (i_core_clocken_a)
                        begin
                            if (read_during_write_mode_port_a == "NEW_DATA_NO_NBE_READ")
                                if (is_lutram && clock0)
                                    i_q_tmp2_a = mem_data[i_address_reg_a];
                                else
                                    i_q_tmp2_a = ((i_data_reg_a & i_byteena_mask_reg_a) |
                                                ({width_a{1'b0 /* converted x or z to 1'b0 */}} & ~i_byteena_mask_reg_a));
                            else if (read_during_write_mode_port_a == "NEW_DATA_WITH_NBE_READ")
                                if (is_lutram && clock0)
                                    i_q_tmp2_a = mem_data[i_address_reg_a];
                                else
                                    i_q_tmp2_a = (i_data_reg_a & i_byteena_mask_reg_a) | (mem_data[i_address_reg_a] & ~i_byteena_mask_reg_a) ^ i_byteena_mask_reg_a_x;
                            else if (read_during_write_mode_port_a == "OLD_DATA")
                                i_q_tmp2_a = i_original_data_a;
                            else
                                if ((lutram_single_port_fast_read == 0) && (i_ram_block_type != "AUTO"))
                                begin
                                    if ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))
                                        i_q_tmp2_a = {width_a{1'b0 /* converted x or z to 1'b0 */}};
                                    else
                                        i_q_tmp2_a = i_original_data_a;
                                end
                                else
                                    if (is_lutram)
                                        i_q_tmp2_a = mem_data[i_address_reg_a];
                                    else
                                        i_q_tmp2_a = i_data_reg_a ^ i_byteena_mask_reg_a_out;
                        end
                        else
                            i_q_tmp2_a = mem_data[i_address_reg_a];
                    end
                    else
                        i_q_tmp2_a = mem_data[i_address_reg_a];

                    if (is_write_on_positive_edge == 1)
                    begin

                        if (is_bidir_and_wrcontrol_addb_clk0 || (same_clock_pulse0 && same_clock_pulse1))
                        begin
                            // B write, A read
                        if ((i_wren_reg_b & ~i_wren_reg_a) &
                            ((((is_bidir_and_wrcontrol_addb_clk0 & i_clocken0_b) ||
                            (is_bidir_and_wrcontrol_addb_clk1 & i_clocken1_b)) && ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1))) ||
                            (((is_bidir_and_wrcontrol_addb_clk0 & i_core_clocken0_b) ||
                            (is_bidir_and_wrcontrol_addb_clk1 & i_core_clocken1_b)) && ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)))) & (i_data_write_time_b == $time))
                            begin
                                add_reg_a_mult_wa = i_address_reg_a * width_a;
                                add_reg_b_mult_wb = i_address_reg_b * width_b;
                                add_reg_a_mult_wa_pl_wa = add_reg_a_mult_wa + width_a;
                                add_reg_b_mult_wb_pl_wb = add_reg_b_mult_wb + width_b;

                                if (
                                    ((add_reg_a_mult_wa >=
                                        add_reg_b_mult_wb) &&
                                    (add_reg_a_mult_wa <=
                                        (add_reg_b_mult_wb_pl_wb - 1)))
                                        ||
                                    (((add_reg_a_mult_wa_pl_wa - 1) >=
                                        add_reg_b_mult_wb) &&
                                    ((add_reg_a_mult_wa_pl_wa - 1) <=
                                        (add_reg_b_mult_wb_pl_wb - 1)))
                                        ||
                                    ((add_reg_b_mult_wb >=
                                        add_reg_a_mult_wa) &&
                                    (add_reg_b_mult_wb <=
                                        (add_reg_a_mult_wa_pl_wa - 1)))
                                        ||
                                    (((add_reg_b_mult_wb_pl_wb - 1) >=
                                        add_reg_a_mult_wa) &&
                                    ((add_reg_b_mult_wb_pl_wb - 1) <=
                                        (add_reg_a_mult_wa_pl_wa - 1)))
                                    )
                                        for (i3 = add_reg_a_mult_wa;
                                                i3 < add_reg_a_mult_wa_pl_wa;
                                                i3 = i3 + 1)
                                        begin
                                            if ((i3 >= add_reg_b_mult_wb) &&
                                                (i3 <= (add_reg_b_mult_wb_pl_wb - 1)))
                                            begin

                                                if (cread_during_write_mode_mixed_ports == "OLD_DATA")
                                                begin
                                                    i_byteena_count = i3 - add_reg_b_mult_wb;
                                                    i_q_tmp2_a_idx = (i3 - add_reg_a_mult_wa);
                                                    i_q_tmp2_a[i_q_tmp2_a_idx] = i_original_data_b[i_byteena_count];
                                                end
                                                else
                                                begin
                                                    i_byteena_count = i3 - add_reg_b_mult_wb;
                                                    i_q_tmp2_a_idx = (i3 - add_reg_a_mult_wa);
                                                    i_q_tmp2_a[i_q_tmp2_a_idx] = i_q_tmp2_a[i_q_tmp2_a_idx] ^ i_byteena_mask_reg_b_out_a[i_byteena_count];
                                                end

                                            end
                                        end
                            end
                        end
                    end
                end

                if ((is_lutram == 1) && i_address_aclr_a && (i_address_aclr_family_a == 0) && (operation_mode == "ROM"))
                    i_q_tmp2_a = mem_data[0];

                if (((family_has_stratixv_style_ram == 1) || (family_cycloneiii == 1)) &&
                    (is_lutram != 1) &&
                    (i_outdata_aclr_a || i_force_reread_a) &&
                    (outdata_reg_a != "CLOCK0") && (outdata_reg_a != "CLOCK1"))
                    i_q_tmp2_a = {width_a{1'b0}};
            end // end good_to_go_a
        end
    end


    // assigning the correct output values for i_q_tmp_a (non-registered output)
    always @(i_q_tmp2_a or i_wren_reg_a or i_data_reg_a or i_address_aclr_a or
             i_address_reg_a or i_byteena_mask_reg_a_out or i_numwords_a or i_outdata_aclr_a or i_force_reread_a_signal or i_original_data_a)
    begin
        if (i_address_reg_a >= i_numwords_a)
        begin
            if (i_wren_reg_a && i_core_clocken_a)
                i_q_tmp_a <= i_q_tmp2_a;
            else
                i_q_tmp_a <= {width_a{1'b0 /* converted x or z to 1'b0 */}};
            if (i_rden_reg_a == 1)
            begin
                $display("Warning : Address pointed at port A is out of bound!");
                $display("Time: %0t  Instance: %m", $time);
            end
        end
        else
            begin
                if (i_outdata_aclr_a_prev && ~ i_outdata_aclr_a &&
                    (family_has_stratixiii_style_ram == 1) &&
                    (is_lutram != 1))
                begin
                    i_outdata_aclr_a_prev = i_outdata_aclr_a;
                    i_force_reread_a <= 1;
                end
                else if (~i_address_aclr_a_prev && i_address_aclr_a && (i_address_aclr_family_a == 0) && s3_address_aclr_a)
                begin
                    if (i_rden_reg_a)
                        i_q_tmp_a <= {width_a{1'b0 /* converted x or z to 1'b0 */}};
                    i_force_reread_a1 <= 1;
                end
                else if ((i_force_reread_a == 0) && (i_force_reread_a1 == 0) && !(i_address_aclr_a_prev && ~i_address_aclr_a && (i_address_aclr_family_a == 0) && s3_address_aclr_a))
                begin
                    i_q_tmp_a <= i_q_tmp2_a;
                end
            end
            if ((i_outdata_aclr_a) && (s3_address_aclr_a))
            begin
                i_q_tmp_a <= {width_a{1'b0}};
                i_outdata_aclr_a_prev <= i_outdata_aclr_a;
            end
            i_address_aclr_a_prev <= i_address_aclr_a;
    end


    // Port A outdata output registered
    generate if (outdata_reg_a == "CLOCK1")
        begin: clk1_on_outa_gen
            always @(posedge clock1 or posedge i_outdata_aclr_a)
            begin
                if (i_outdata_aclr_a)
                    i_q_reg_a <= 0;
                else if (i_outdata_clken_a)
                begin
                    i_q_reg_a <= i_q_tmp_a;
                    if (i_core_clocken_a)
                    i_address_aclr_a_flag <= 0;
                end
                else if (i_core_clocken_a)
                    i_address_aclr_a_flag <= 0;
            end
        end
        else if (outdata_reg_a == "CLOCK0")
        begin: clk0_on_outa_gen
            always @(posedge clock0 or posedge i_outdata_aclr_a)
            begin
                if (i_outdata_aclr_a)
                    i_q_reg_a <= 0;
                else if (i_outdata_clken_a)
                begin
                    if ((i_address_aclr_a_flag == 1) &&
                        (family_has_stratixv_style_ram || family_stratixiii) && (is_lutram != 1))
                        i_q_reg_a <= 'bx;
                    else
                        i_q_reg_a <= i_q_tmp_a;
                    if (i_core_clocken_a)
                    i_address_aclr_a_flag <= 0;
                end
                else if (i_core_clocken_a)
                    i_address_aclr_a_flag <= 0;
            end
        end
    endgenerate

    // Latch for address aclr till outclock enabled
    always @(posedge i_address_aclr_a or posedge i_outdata_aclr_a)
    begin
        if (i_outdata_aclr_a)
            i_address_aclr_a_flag <= 0;
        else
            if (i_rden_reg_a && (i_address_aclr_family_a == 0))
                i_address_aclr_a_flag <= 1;
    end

    // Port A : assigning the correct output values for q_a
    assign q_a = (operation_mode == "DUAL_PORT") ?
                    {width_a{1'b0}} : (((outdata_reg_a == "CLOCK0") ||
                            (outdata_reg_a == "CLOCK1")) ?
                    i_q_reg_a : i_q_tmp_a);


    // Port B reading
    always @(i_read_flag_b)
    begin
        if ((operation_mode == "BIDIR_DUAL_PORT") ||
            (operation_mode == "DUAL_PORT"))
        begin
            if (~good_to_go_b && (is_lutram == 0))
            begin

                if ((check_simultaneous_read_write == 1) &&
                    ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0)) &&
                    (family_cycloneii == 0))
                    i_q_tmp2_b = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                else
                    i_q_tmp2_b = 0;
            end
            else
            begin
                if (i_rden_reg_b)
                begin
                    //If width_a is equal to b, no address calculation is needed
                    if (width_a == width_b)
                    begin

                        // read from memory or flow through for write cycle
                        if (i_wren_reg_b && (((is_bidir_and_wrcontrol_addb_clk0 == 1) && i_core_clocken0_b) ||
                            ((is_bidir_and_wrcontrol_addb_clk1 == 1) && i_core_clocken1_b)))
                        begin
                            if (read_during_write_mode_port_b == "NEW_DATA_NO_NBE_READ")
                                temp_wb = ((i_data_reg_b & i_byteena_mask_reg_b) |
                                            ({width_b{1'b0 /* converted x or z to 1'b0 */}} & ~i_byteena_mask_reg_b));
                            else if (read_during_write_mode_port_b == "NEW_DATA_WITH_NBE_READ")
                                temp_wb = (i_data_reg_b & i_byteena_mask_reg_b) | (mem_data[i_address_reg_b] & ~i_byteena_mask_reg_b) ^ i_byteena_mask_reg_b_x;
                            else if (read_during_write_mode_port_b == "OLD_DATA")
                                temp_wb = i_original_data_b;
                            else
                                temp_wb = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        end
                        else if ((i_data_write_time_a == $time) && (operation_mode == "DUAL_PORT")  &&
                            ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0)))
                        begin
                            // if A write to the same Ram address B is reading from
                            if ((i_address_reg_b == i_address_reg_a) && (i_original_address_a == i_address_reg_a))
                            begin
                                if (address_reg_b != "CLOCK0")
                                    temp_wb = mem_data[i_address_reg_b] ^ i_byteena_mask_reg_a_out_b;
                                else if (cread_during_write_mode_mixed_ports == "OLD_DATA")
                                begin
                                    if (mem_data[i_address_reg_b] === ((i_data_reg_a & i_byteena_mask_reg_a) | (mem_data[i_address_reg_a] & ~i_byteena_mask_reg_a) ^ i_byteena_mask_reg_a_x))
                                        temp_wb = i_original_data_a;
                                    else
                                        temp_wb = mem_data[i_address_reg_b];
                                end
                                else if ((cread_during_write_mode_mixed_ports == "DONT_CARE") || (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE"))
                                    temp_wb = mem_data[i_address_reg_b] ^ i_byteena_mask_reg_a_out_b;
                                else
                                    temp_wb = mem_data[i_address_reg_b];
                            end
                            else
                                temp_wb = mem_data[i_address_reg_b];
                        end
                        else
                            temp_wb = mem_data[i_address_reg_b];

                        if (is_write_on_positive_edge == 1)
                        begin
                            if ((dual_port_addreg_b_clk0 == 1) ||
                                (is_bidir_and_wrcontrol_addb_clk0 == 1) || (same_clock_pulse0 && same_clock_pulse1))
                            begin
                                // A write, B read
                                if ((i_wren_reg_a & ~i_wren_reg_b) &&
                                    ((i_clocken0 && ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0))) ||
                                    (i_core_clocken_a && ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)))))
                                begin
                                    // if A write to the same Ram address B is reading from
                                    if (i_address_reg_b == i_address_reg_a)
                                    begin
                                        if (lutram_dual_port_fast_read == 1)
                                            temp_wb = (i_data_reg_a & i_byteena_mask_reg_a) | (i_q_tmp2_a & ~i_byteena_mask_reg_a) ^ i_byteena_mask_reg_a_x;
                                        else
                                            if (cread_during_write_mode_mixed_ports == "OLD_DATA")
                                                if ((mem_data[i_address_reg_b] === ((i_data_reg_a & i_byteena_mask_reg_a) | (mem_data[i_address_reg_a] & ~i_byteena_mask_reg_a) ^ i_byteena_mask_reg_a_x))
                                                    && (i_data_write_time_a == $time))
                                                    temp_wb = i_original_data_a;
                                                else
                                                    temp_wb = mem_data[i_address_reg_b];
                                            else
												if(i_data_write_time_a == $time)
													temp_wb = mem_data[i_address_reg_b] ^ i_byteena_mask_reg_a_out_b;
												else
													temp_wb = mem_data[i_address_reg_b];
                                    end
                                end
                            end
                        end
                    end
                    else
                    begin
                        j2 = i_address_reg_b * width_b;

                        for (i5=0; i5<width_b; i5=i5+1)
                        begin
                            j2_plus_i5 = j2 + i5;
                            temp_wa2b = mem_data[j2_plus_i5 / width_a];
                            temp_wb[i5] = temp_wa2b[j2_plus_i5 % width_a];
                        end

                        if (i_wren_reg_b && ((is_bidir_and_wrcontrol_addb_clk0 && i_core_clocken0_b) ||
                            (is_bidir_and_wrcontrol_addb_clk1 && i_core_clocken1_b)))
                        begin
                            if (read_during_write_mode_port_b == "NEW_DATA_NO_NBE_READ")
                                temp_wb = i_data_reg_b ^ i_byteena_mask_reg_b_out;
                            else if (read_during_write_mode_port_b == "NEW_DATA_WITH_NBE_READ")
                                temp_wb = (i_data_reg_b & i_byteena_mask_reg_b) | (temp_wb & ~i_byteena_mask_reg_b) ^ i_byteena_mask_reg_b_x;
                            else if (read_during_write_mode_port_b == "OLD_DATA")
                                temp_wb = i_original_data_b;
                            else
                                temp_wb = {width_b{1'b0 /* converted x or z to 1'b0 */}};
                        end
                        else if ((i_data_write_time_a == $time) &&  (operation_mode == "DUAL_PORT") &&
                            ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0)))
                        begin
                            for (i5=0; i5<width_b; i5=i5+1)
                            begin
                                j2_plus_i5 = j2 + i5;
                                j2_plus_i5_div_a = j2_plus_i5 / width_a;

                                // if A write to the same Ram address B is reading from
                                if ((j2_plus_i5_div_a == i_address_reg_a) && (i_original_address_a == i_address_reg_a))
                                begin
                                    if (address_reg_b != "CLOCK0")
                                    begin
                                        temp_wa2b = mem_data[j2_plus_i5_div_a];
                                        temp_wa2b = temp_wa2b ^ i_byteena_mask_reg_a_out_b;
                                    end
                                    else if (cread_during_write_mode_mixed_ports == "OLD_DATA")
                                        temp_wa2b = i_original_data_a;
                                    else if ((cread_during_write_mode_mixed_ports == "DONT_CARE") || (cread_during_write_mode_mixed_ports == "CONSTRAINED_DONT_CARE"))
                                    begin
                                        temp_wa2b = mem_data[j2_plus_i5_div_a];
                                        temp_wa2b = temp_wa2b ^ i_byteena_mask_reg_a_out_b;
                                    end
                                    else
                                        temp_wa2b = mem_data[j2_plus_i5_div_a];
                                end
                                else
                                    temp_wa2b = mem_data[j2_plus_i5_div_a];

                                temp_wb[i5] = temp_wa2b[j2_plus_i5 % width_a];
                            end
                        end

                        if (is_write_on_positive_edge == 1)
                        begin
                            if (((address_reg_b == "CLOCK0") & dual_port_addreg_b_clk0) ||
                                ((wrcontrol_wraddress_reg_b == "CLOCK0") & is_bidir_and_wrcontrol_addb_clk0) || (same_clock_pulse0 && same_clock_pulse1))
                            begin
                                // A write, B read
                                if ((i_wren_reg_a & ~i_wren_reg_b) &&
                                    ((i_clocken0 && ((family_has_stratixv_style_ram == 0) && (family_has_stratixiii_style_ram == 0))) ||
                                    (i_core_clocken_a && ((family_has_stratixv_style_ram == 1) || (family_has_stratixiii_style_ram == 1)))))
                                begin

                                    for (i5=0; i5<width_b; i5=i5+1)
                                    begin
                                        j2_plus_i5 = j2 + i5;
                                        j2_plus_i5_div_a = j2_plus_i5 / width_a;

                                        // if A write to the same Ram address B is reading from
                                        if (j2_plus_i5_div_a == i_address_reg_a)
                                        begin
                                            if (lutram_single_port_fast_read == 1 && (i_data_write_time_a == $time))
                                                temp_wa2b = (i_data_reg_a & i_byteena_mask_reg_a) | (i_q_tmp2_a & ~i_byteena_mask_reg_a) ^ i_byteena_mask_reg_a_x;
                                            else
                                            begin
                                                if(i_data_write_time_a == $time) begin
                                                    if (cread_during_write_mode_mixed_ports == "OLD_DATA")
                                                        temp_wa2b = i_original_data_a;
                                                    else
                                                    begin
                                                        temp_wa2b = mem_data[j2_plus_i5_div_a];
                                                        temp_wa2b = temp_wa2b ^ i_byteena_mask_reg_a_out_b;
                                                    end
                                                end
                                                else
                                                begin
                                                    temp_wa2b = i_original_data_a;
                                                end
                                            end

                                            temp_wb[i5] = temp_wa2b[j2_plus_i5 % width_a];
                                        end

                                    end
                                end
                            end
                        end
                    end
                    //end of width_a != width_b

                    i_q_tmp2_b = temp_wb;

                end

                if ((is_lutram == 1) && i_address_aclr_b && (i_address_aclr_family_b == 0) && (operation_mode == "DUAL_PORT"))
                begin
                    for (init_i = 0; init_i < width_b; init_i = init_i + 1)
                    begin
                        init_temp = mem_data[init_i / width_a];
                        i_q_tmp_b[init_i] = init_temp[init_i % width_a];
                        i_q_tmp2_b[init_i] = init_temp[init_i % width_a];
                    end
                end
                else if ((is_lutram == 1) && (operation_mode == "DUAL_PORT"))
                begin
                    j2 = i_address_reg_b * width_b;

                    for (i5=0; i5<width_b; i5=i5+1)
                    begin
                        j2_plus_i5 = j2 + i5;
                        temp_wa2b = mem_data[j2_plus_i5 / width_a];
                        i_q_tmp2_b[i5] = temp_wa2b[j2_plus_i5 % width_a];
                    end
                end

                if ((i_outdata_aclr_b || i_force_reread_b) &&
                    ((family_has_stratixv_style_ram == 1) || (family_cycloneiii == 1)) &&
                    (is_lutram != 1) &&
                    (outdata_reg_b != "CLOCK0") && (outdata_reg_b != "CLOCK1"))
                    i_q_tmp2_b = {width_b{1'b0}};
            end
        end
    end


    // assigning the correct output values for i_q_tmp_b (non-registered output)
    always @(i_q_tmp2_b or i_wren_reg_b or i_data_reg_b or i_address_aclr_b or
                 i_address_reg_b or i_byteena_mask_reg_b_out or i_rden_reg_b or
                 i_numwords_b or i_outdata_aclr_b or i_force_reread_b_signal)
    begin
        if (i_address_reg_b >= i_numwords_b)
        begin
            if (i_wren_reg_b && ((i_core_clocken0_b && (is_bidir_and_wrcontrol_addb_clk0 == 1)) || (i_core_clocken1_b && (is_bidir_and_wrcontrol_addb_clk1 == 1))))
                i_q_tmp_b <= i_q_tmp2_b;
            else
                i_q_tmp_b <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
            if (i_rden_reg_b == 1)
            begin
                $display("Warning : Address pointed at port B is out of bound!");
                $display("Time: %0t  Instance: %m", $time);
            end
        end
        else
            if (operation_mode == "BIDIR_DUAL_PORT")
            begin

                if (i_outdata_aclr_b_prev && ~ i_outdata_aclr_b && (family_has_stratixiii_style_ram == 1) && (is_lutram != 1))
                begin
                    i_outdata_aclr_b_prev <= i_outdata_aclr_b;
                    i_force_reread_b <= 1;
                end
                else
                begin
					if( i_force_reread_b == 0)
						i_q_tmp_b <= i_q_tmp2_b;
                end
            end
            else if (operation_mode == "DUAL_PORT")
            begin
                if (i_outdata_aclr_b_prev && ~ i_outdata_aclr_b && (family_has_stratixiii_style_ram == 1) && (is_lutram != 1))
                begin
                    i_outdata_aclr_b_prev <= i_outdata_aclr_b;
                    i_force_reread_b <= 1;
                end
                else if (~i_address_aclr_b_prev && i_address_aclr_b && (i_address_aclr_family_b == 0) && s3_address_aclr_b)
                begin
                    if (i_rden_reg_b)
                        i_q_tmp_b <= {width_b{1'b0 /* converted x or z to 1'b0 */}};
                    i_force_reread_b1 <= 1;
                end
                else if ((i_force_reread_b1 == 0) && !(i_address_aclr_b_prev && ~i_address_aclr_b && (i_address_aclr_family_b == 0) && s3_address_aclr_b))
                begin


                if ((is_lutram == 1) && (is_write_on_positive_edge) && (cread_during_write_mode_mixed_ports == "OLD_DATA") && (width_a == width_b) && (i_address_reg_a == i_address_reg_b) && i_wren_reg_a && i_rden_reg_b)
                    i_q_tmp_b <= i_original_data_a;
		else
                    i_q_tmp_b <= i_q_tmp2_b;
                end
            end

        if ((i_outdata_aclr_b) && (s3_address_aclr_b))
        begin
            i_q_tmp_b <= {width_b{1'b0}};
            i_outdata_aclr_b_prev <= i_outdata_aclr_b;
        end
        i_address_aclr_b_prev <= i_address_aclr_b;
    end

    generate if (outdata_reg_b == "CLOCK1")
        begin: clk1_on_outb_fall_gen
            always @(negedge clock1)
            begin
                if (i_core_clocken_a)
		begin
                    if ((width_a == width_b) && (i_address_reg_a == i_address_reg_b) && i_wren_reg_a && i_rden_reg_b)
                        i_q_output_latch <= i_original_data_a;
	            else
                        i_q_output_latch <= i_q_tmp2_b;
                end
	    end
        end
        else if (outdata_reg_b == "CLOCK0")
        begin: clk0_on_outb_fall_gen
            always @(negedge clock0)
            begin
                if (i_core_clocken_a)
		begin
                    if ((width_a == width_b) && (i_address_reg_a == i_address_reg_b) && i_wren_reg_a && i_rden_reg_b)
                        i_q_output_latch <= i_original_data_a;
	            else
                        i_q_output_latch <= i_q_tmp2_b;
                end
            end
        end
    endgenerate

    // Port B outdata output registered
    generate if (outdata_reg_b == "CLOCK1")
        begin: clk1_on_outb_rise_gen
            always @(posedge clock1 or posedge i_outdata_aclr_b)
            begin
                if (i_outdata_aclr_b)
                    i_q_reg_b <= 0;
                else if (i_outdata_clken_b)
                begin
                    if ((i_address_aclr_b_flag == 1) && (family_has_stratixv_style_ram || family_stratixiii) &&
                        (is_lutram != 1))
                        i_q_reg_b <= 'bx;
                    else
                    i_q_reg_b <= i_q_tmp_b;
                end
            end
        end
        else if (outdata_reg_b == "CLOCK0")
        begin: clk0_on_outb_rise_gen
            always @(posedge clock0 or posedge i_outdata_aclr_b)
            begin
                if (i_outdata_aclr_b)
                    i_q_reg_b <= 0;
                else if (i_outdata_clken_b)
                begin
                    if ((is_lutram == 1) && (cread_during_write_mode_mixed_ports == "OLD_DATA"))
                        i_q_reg_b <= i_q_output_latch;
                    else
                    begin
                        if ((i_address_aclr_b_flag == 1) && (family_has_stratixv_style_ram || family_stratixiii) &&
                            (is_lutram != 1))
                            i_q_reg_b <= 'bx;
                        else
                        i_q_reg_b <= i_q_tmp_b;
                    end
                end
            end
        end
    endgenerate

	generate if (outdata_reg_b == "CLOCK0" && ecc_pipeline_stage_enabled == "TRUE")
	begin: clk0_on_ecc_pipeline_reg_rise_gen
		 always @(posedge clock0 or posedge i_outdata_aclr_b)
		begin
			if (i_outdata_aclr_b)
				i_q_ecc_reg_b <= 0;
			else if (i_outdata_clken_b)
			begin
				i_q_ecc_reg_b <= i_q_reg_b;
			end
		end
	end
			else if (outdata_reg_b == "CLOCK1" && ecc_pipeline_stage_enabled == "TRUE")
	begin: clk0_on_ecc_pipeline_reg_rise_gen
		always @(posedge clock1 or posedge i_outdata_aclr_b)
		begin
			if (i_outdata_aclr_b)
				i_q_ecc_reg_b <= 0;
			else if (i_outdata_clken_b)
			begin
				i_q_ecc_reg_b <= i_q_reg_b;
			end
		end
	end
	endgenerate

    // Latch for address aclr till outclock enabled
    always @(posedge i_address_aclr_b or posedge i_outdata_aclr_b)
        if (i_outdata_aclr_b)
            i_address_aclr_b_flag <= 0;
        else
        begin
            if (i_rden_reg_b)
                i_address_aclr_b_flag <= 1;
        end

    // Port B : assigning the correct output values for q_b
    assign q_b = ((operation_mode == "SINGLE_PORT") ||
                    (operation_mode == "ROM")) ?
                        {width_b{1'b0}} : (((outdata_reg_b == "CLOCK0") ||
                            (outdata_reg_b == "CLOCK1")) ?
							((ecc_pipeline_stage_enabled == "TRUE")?(i_q_ecc_reg_b) : (i_q_reg_b)):
							((ecc_pipeline_stage_enabled == "TRUE")?(i_q_ecc_tmp_b) : (i_q_tmp_b))); //i_q_ecc_tmp_b has 'x' output


    // ECC status
    assign eccstatus = {width_eccstatus{1'b0}};

endmodule // altsyncram_body

