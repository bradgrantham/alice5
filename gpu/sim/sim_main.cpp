#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "objectfile.h"

#include "VMain.h"
#include "VMain_Main.h"
// #include "VMain_Registers.h"
// #include "VMain_BlockRam__A5.h"
#include "VMain_BlockRam__A10.h"

void usage(const char* progname)
{
    printf("usage: %s shader.bin\n", progname);
}

template<typename T>
std::string to_hex(T i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << std::uppercase;
    stream << uint64_t(i);
    return stream.str();
}

const char *stateToString(int state)
{
    switch(state) {
        case VMain_Main::STATE_INIT: return "STATE_INIT"; break;
        case VMain_Main::STATE_FETCH: return "STATE_FETCH"; break;
        case VMain_Main::STATE_FETCH2: return "STATE_FETCH2"; break;
        case VMain_Main::STATE_DECODE: return "STATE_DECODE"; break;
        case VMain_Main::STATE_ALU: return "STATE_ALU"; break;
        case VMain_Main::STATE_STEPLOADSTORE: return "STATE_STEPLOADSTORE"; break;
        default : return "unknown state"; break;
    }
}

void print_decoded_inst(uint32_t address, uint32_t inst, VMain *top)
{
    if(top->decode_opcode_is_ALU_reg_imm)  {
        printf("0x%08X : 0x%08X - opcode_is_ALU_reg_imm %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
    } else if(top->decode_opcode_is_branch) {
        printf("0x%08X : 0x%08X - opcode_is_branch cmp %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rs1, top->decode_rs2, top->decode_imm_branch);
    } else if(top->decode_opcode_is_ALU_reg_reg) {
        printf("0x%08X : 0x%08X - opcode_is_ALU_reg_reg op %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_jal) {
        printf("0x%08X : 0x%08X - opcode_is_jal %d, %d\n", address, inst, top->decode_rd, top->decode_imm_jump);
    } else if(top->decode_opcode_is_jalr) {
        printf("0x%08X : 0x%08X - opcode_is_jalr %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_imm_jump);
    } else if(top->decode_opcode_is_lui) {
        printf("0x%08X : 0x%08X - opcode_is_lui %d, %d\n", address, inst, top->decode_rd, top->decode_imm_upper);
    } else if(top->decode_opcode_is_auipc) {
        printf("0x%08X : 0x%08X - opcode_is_auipc %d, %d\n", address, inst, top->decode_rd, top->decode_imm_upper);
    } else if(top->decode_opcode_is_load) {
        printf("0x%08X : 0x%08X - opcode_is_load size %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
    } else if(top->decode_opcode_is_store) {
        printf("0x%08X : 0x%08X - opcode_is_store size %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rs1, top->decode_rs2, top->decode_imm_store);
    } else if(top->decode_opcode_is_system) {
        printf("0x%08X : 0x%08X - opcode_is_system %d\n", address, inst, top->decode_imm_alu_load);
    } else if(top->decode_opcode_is_fadd) {
        printf("0x%08X : 0x%08X - opcode_is_fadd rm %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fsub) {
        printf("0x%08X : 0x%08X - opcode_is_fsub rm %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fmul) {
        printf("0x%08X : 0x%08X - opcode_is_fmul rm %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fdiv) {
        printf("0x%08X : 0x%08X - opcode_is_fdiv rm %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fsgnj) {
        printf("0x%08X : 0x%08X - opcode_is_fsgnj mode %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fminmax) {
        printf("0x%08X : 0x%08X - opcode_is_fminmax mode %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fsqrt) {
        printf("0x%08X : 0x%08X - opcode_is_fsqrt rm %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
    } else if(top->decode_opcode_is_fcmp) {
        printf("0x%08X : 0x%08X - opcode_is_fcmp cmp %d, %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
    } else if(top->decode_opcode_is_fcvt_f2i) {
        printf("0x%08X : 0x%08X - opcode_is_f2i type %d, rm %d, %d, %d\n", address, inst, top->decode_shamt_ftype, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
    } else if(top->decode_opcode_is_fmv_f2i) {
        printf("0x%08X : 0x%08X - opcode_is_fmv_f2i funct3 %d, %d, %d\n", address, inst, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
    } else if(top->decode_opcode_is_fcvt_i2f) {
        printf("0x%08X : 0x%08X - opcode_is_fcvt_i2f type %d, rm %d, %d, %d\n", address, inst, top->decode_shamt_ftype, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
    } else if(top->decode_opcode_is_fmv_i2f) {
        printf("0x%08X : 0x%08X - opcode_is_fmv_i2f %d, %d\n", address, inst, top->decode_rd, top->decode_rs1);
    } else if(top->decode_opcode_is_flw) {
        printf("0x%08X : 0x%08X - opcode_is_flw %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
    } else if(top->decode_opcode_is_fsw) {
        printf("0x%08X : 0x%08X - opcode_is_fsw %d, %d, %d\n", address, inst, top->decode_rs1, top->decode_rs2, top->decode_imm_store);
    } else if(top->decode_opcode_is_fmadd) {
        printf("0x%08X : 0x%08X - opcode_is_fmadd %d, %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
    } else if(top->decode_opcode_is_fmsub) {
        printf("0x%08X : 0x%08X - opcode_is_fmsub %d, %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
    } else if(top->decode_opcode_is_fnmsub) {
        printf("0x%08X : 0x%08X - opcode_is_fnmsub %d, %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
    } else if(top->decode_opcode_is_fnmadd) {
        printf("0x%08X : 0x%08X - opcode_is_fnmadd %d, %d, %d, %d\n", address, inst, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
    } else {
        printf("0x%08X : 0x%08X - undecoded instruction\n", address, inst);
    }
}

int main(int argc, char **argv) {

    bool beVerbose = false;

    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }

    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    RunHeader2 header;
    std::vector<uint8_t> inst_bytes;
    std::vector<uint8_t> data_bytes;
    SymbolTable inst_symbols;
    SymbolTable data_symbols;

    std::ifstream binaryFile(argv[0], std::ios::in | std::ios::binary);
    if(!binaryFile.good()) {
        throw std::runtime_error(std::string("couldn't open file ") + argv[0] + " for reading");
    }

    if(!ReadBinary(binaryFile, header, inst_symbols, data_symbols, inst_bytes, data_bytes)) {
        exit(EXIT_FAILURE);
    }
    assert(inst_bytes.size() % 4 == 0);

    VMain *top = new VMain;

    if(false)
        std::cout << "clock " << int(top->clock) << ", should be 0\n";

    // Run one clock cycle in reset to process STATE_INIT
    top->reset_n = 0;
    top->run = 0;

    top->data_ext_write = 0;
    top->inst_ext_write = 0;
    top->ext_decode_inst = 0;

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();

    // Release reset
    top->reset_n = 1;

    // run == 0, nothing should be happening here.
    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();

    // Write inst bytes to inst memory
    for(uint32_t byteaddr = 0; byteaddr < inst_bytes.size(); byteaddr += 4) {
        uint32_t inst_word = 
            inst_bytes[byteaddr + 0] <<  0 |
            inst_bytes[byteaddr + 1] <<  8 |
            inst_bytes[byteaddr + 2] << 16 |
            inst_bytes[byteaddr + 3] << 24;

        top->inst_ext_address = byteaddr;
        top->inst_ext_in_data = inst_word;
        top->inst_ext_write = 1;

        if(beVerbose) {
            std::cout << "Writing to inst " << byteaddr << " value 0x" << to_hex(top->inst_ext_in_data) << "\n";
        }

        top->clock = 1;
        top->eval();
        top->clock = 0;
        top->eval();

        top->inst_ext_write = 0;

        top->clock = 1;
        top->eval();
        top->clock = 0;
        top->eval();
    }

    // Write data bytes to data memory
    for(uint32_t byteaddr = 0; byteaddr < data_bytes.size(); byteaddr += 4) {
        uint32_t data_word = 
            data_bytes[byteaddr + 0] <<  0 |
            data_bytes[byteaddr + 1] <<  8 |
            data_bytes[byteaddr + 2] << 16 |
            data_bytes[byteaddr + 3] << 24;

        top->data_ext_address = byteaddr;
        top->data_ext_in_data = data_word;
        top->data_ext_write = 1;

        if(beVerbose) {
            std::cout << "Writing to data " << byteaddr << " value 0x" << to_hex(top->data_ext_in_data) << "\n";
        }

        top->clock = 1;
        top->eval();
        top->clock = 0;
        top->eval();

        top->data_ext_write = 0;

        top->clock = 1;
        top->eval();
        top->clock = 0;
        top->eval();
    }

    for(uint32_t byteaddr = 0; byteaddr < 16; byteaddr += 4) {
        uint32_t inst_word = 
            inst_bytes[byteaddr + 0] <<  0 |
            inst_bytes[byteaddr + 1] <<  8 |
            inst_bytes[byteaddr + 2] << 16 |
            inst_bytes[byteaddr + 3] << 24;
        std::cout << "inst memory[" << byteaddr << "] = "
            << to_hex(top->Main->instRam->memory[byteaddr >> 2]) << ", should be "
            << to_hex(inst_word) << "\n";
    }

    bool test_decoder = true;
    if(test_decoder) {

        // decode program instructions to see what decoder says
        for(uint32_t address = 0; address < inst_bytes.size(); address += 4) {

            uint32_t inst_word = 
                inst_bytes[address + 0] <<  0 |
                inst_bytes[address + 1] <<  8 |
                inst_bytes[address + 2] << 16 |
                inst_bytes[address + 3] << 24;

            top->ext_inst_to_decode = inst_word;
            top->ext_decode_inst = 1;

            top->clock = 1;
            top->eval();
            top->clock = 0;
            top->eval();

            print_decoded_inst(address, inst_word, top);

            top->ext_decode_inst = 0;

            top->clock = 1;
            top->eval();
            top->clock = 0;
            top->eval();

        }
    }

    // Run
    top->run = 1;

    std::string pad = "                     ";

    while (!Verilated::gotFinish()) {

        top->clock = 0;
        top->eval();

        // right side of nonblocking assignments
        std::cout << pad << "between clock 0 and clock 1\n";
        std::cout << pad << "CPU in state " << stateToString(top->Main->state) << " (" << int(top->Main->state) << ")\n";
        std::cout << pad << "pc = 0x" << to_hex(top->Main->PC) << "\n";
        std::cout << pad << "inst_ram_address = 0x" << to_hex(top->Main->inst_ram_address) << "\n";
        std::cout << pad << "inst_ram_out_data = 0x" << to_hex(top->Main->inst_ram_out_data) << "\n";
        std::cout << pad << "inst_to_decode = 0x" << to_hex(top->Main->inst_to_decode) << "\n";

        top->clock = 1;
        top->eval();

        // left side of nonblocking assignments
        std::cout << "between clock 1 and clock 0\n";
        std::cout << "CPU in state " << stateToString(top->Main->state) << " (" << int(top->Main->state) << ")\n";
        std::cout << "pc = 0x" << to_hex(top->Main->PC) << "\n";
        std::cout << "inst_ram_address = 0x" << to_hex(top->Main->inst_ram_address) << "\n";
        std::cout << "inst_ram_out_data = 0x" << to_hex(top->Main->inst_ram_out_data) << "\n";
        std::cout << "inst_to_decode = 0x" << to_hex(top->Main->inst_to_decode) << "\n";

        if(top->Main->state == top->Main->STATE_ALU) {
            print_decoded_inst(top->Main->PC, top->Main->inst_to_decode, top);
        }

        if(false) {

            // Dump register contents.
#if 0
            for (int i = 0; i < 32; i++) {
                // Draw in columns.
                int r = i%4*8 + i/4;
                std::cout << std::setfill('0');
                std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
                    << to_hex(top->Main->registers->bank1->memory[r]) << "   ";
                if (i % 4 == 3) {
                    std::cout << "\n";
                }
            }
#endif
            std::cout << std::setfill('0');
            std::cout << " pc = 0x" << to_hex(top->Main->PC) << "\n";
        }
        std::cout << "---\n";
    }

    top->final();
    delete top;
}

