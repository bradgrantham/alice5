#include <inttypes.h>
#include <iomanip>
#include <iostream>

#include "objectfile.h"

#include "VMain.h"
#include "VMain_Main.h"
#include "VMain_Registers.h"
#include "VMain_BlockRam__A5.h"
#include "VMain_BlockRam__A10.h"

void usage(const char* progname)
{
    printf("usage: %s shader.bin\n", progname);
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

    // Run one clock cycle.
    for(int i = 0; i < 2; i++) {
        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();
    }

    // Write inst bytes to inst memory
    for(uint32_t address = 0; address < inst_bytes.size(); address += 4) {
        top->inst_ext_address = address / 4;
        top->inst_ext_write = 1;
        uint32_t inst_word = 
            inst_bytes[address + 0] <<  0 |
            inst_bytes[address + 1] <<  8 |
            inst_bytes[address + 2] << 16 |
            inst_bytes[address + 3] << 24;
        top->inst_ext_in_data = inst_word;

        if(beVerbose) {
            std::cout << std::setfill('0');
            std::cout << "Writing to inst " << address << " value 0x" << std::hex << std::setw(8) << top->inst_ext_in_data << std::dec << std::setw(0) << "\n";
        }

        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();

        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();
    }

    // Write data bytes to data memory
    for(uint32_t address = 0; address < data_bytes.size(); address += 4) {
        top->data_ext_address = address;
        top->data_ext_write = 1;
        uint32_t data_word = 
            data_bytes[address + 0] <<  0 |
            data_bytes[address + 1] <<  8 |
            data_bytes[address + 2] << 16 |
            data_bytes[address + 3] << 24;
        top->data_ext_in_data = data_word;

        if(beVerbose) {
            std::cout << std::setfill('0');
            std::cout << "Writing to data " << address << " value 0x" << std::hex << std::setw(8) << top->data_ext_in_data << std::dec << std::setw(0) << "\n";
        }

        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();

        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();
    }

    // decode program instructions to see what decoder says
    for(uint32_t address = 0; address < inst_bytes.size(); address += 4) {
        uint32_t inst_word = 
            inst_bytes[address + 0] <<  0 |
            inst_bytes[address + 1] <<  8 |
            inst_bytes[address + 2] << 16 |
            inst_bytes[address + 3] << 24;
        top->insn_to_decode = inst_word; // addi    x1, x0, 314
        for(int i = 0; i < 2; i++) {
            top->clock = top->clock ^ 1;
            top->eval();
        }
        if(top->decode_opcode_is_ALU_reg_imm)  {
            printf("0x%08X : 0x%08X - opcode_is_ALU_reg_imm %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
        } else if(top->decode_opcode_is_branch) {
            printf("0x%08X : 0x%08X - opcode_is_branch cmp %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rs1, top->decode_rs2, top->decode_imm_branch);
        } else if(top->decode_opcode_is_ALU_reg_reg) {
            printf("0x%08X : 0x%08X - opcode_is_ALU_reg_reg op %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_jal) {
            printf("0x%08X : 0x%08X - opcode_is_jal %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_imm_jump);
        } else if(top->decode_opcode_is_jalr) {
            printf("0x%08X : 0x%08X - opcode_is_jalr %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_imm_jump);
        } else if(top->decode_opcode_is_lui) {
            printf("0x%08X : 0x%08X - opcode_is_lui %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_imm_upper);
        } else if(top->decode_opcode_is_auipc) {
            printf("0x%08X : 0x%08X - opcode_is_auipc %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_imm_upper);
        } else if(top->decode_opcode_is_load) {
            printf("0x%08X : 0x%08X - opcode_is_load size %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
        } else if(top->decode_opcode_is_store) {
            printf("0x%08X : 0x%08X - opcode_is_store size %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rs1, top->decode_rs2, top->decode_imm_store);
        } else if(top->decode_opcode_is_system) {
            printf("0x%08X : 0x%08X - opcode_is_system %d\n", address, top->insn_to_decode, top->decode_imm_alu_load);
        } else if(top->decode_opcode_is_fadd) {
            printf("0x%08X : 0x%08X - opcode_is_fadd rm %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fsub) {
            printf("0x%08X : 0x%08X - opcode_is_fsub rm %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fmul) {
            printf("0x%08X : 0x%08X - opcode_is_fmul rm %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fdiv) {
            printf("0x%08X : 0x%08X - opcode_is_fdiv rm %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fsgnj) {
            printf("0x%08X : 0x%08X - opcode_is_fsgnj mode %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fminmax) {
            printf("0x%08X : 0x%08X - opcode_is_fminmax mode %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fsqrt) {
            printf("0x%08X : 0x%08X - opcode_is_fsqrt rm %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
        } else if(top->decode_opcode_is_fcmp) {
            printf("0x%08X : 0x%08X - opcode_is_fcmp cmp %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1, top->decode_rs2);
        } else if(top->decode_opcode_is_fcvt_f2i) {
            printf("0x%08X : 0x%08X - opcode_is_f2i type %d, rm %d, %d, %d\n", address, top->insn_to_decode, top->decode_shamt_ftype, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
        } else if(top->decode_opcode_is_fmv_f2i) {
            printf("0x%08X : 0x%08X - opcode_is_fmv_f2i funct3 %d, %d, %d\n", address, top->insn_to_decode, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
        } else if(top->decode_opcode_is_fcvt_i2f) {
            printf("0x%08X : 0x%08X - opcode_is_fcvt_i2f type %d, rm %d, %d, %d\n", address, top->insn_to_decode, top->decode_shamt_ftype, top->decode_funct3_rm, top->decode_rd, top->decode_rs1);
        } else if(top->decode_opcode_is_fmv_i2f) {
            printf("0x%08X : 0x%08X - opcode_is_fmv_i2f %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1);
        } else if(top->decode_opcode_is_flw) {
            printf("0x%08X : 0x%08X - opcode_is_flw %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_imm_alu_load);
        } else if(top->decode_opcode_is_fsw) {
            printf("0x%08X : 0x%08X - opcode_is_fsw %d, %d, %d\n", address, top->insn_to_decode, top->decode_rs1, top->decode_rs2, top->decode_imm_store);
        } else if(top->decode_opcode_is_fmadd) {
            printf("0x%08X : 0x%08X - opcode_is_fmadd %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
        } else if(top->decode_opcode_is_fmsub) {
            printf("0x%08X : 0x%08X - opcode_is_fmsub %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
        } else if(top->decode_opcode_is_fnmsub) {
            printf("0x%08X : 0x%08X - opcode_is_fnmsub %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
        } else if(top->decode_opcode_is_fnmadd) {
            printf("0x%08X : 0x%08X - opcode_is_fnmadd %d, %d, %d, %d\n", address, top->insn_to_decode, top->decode_rd, top->decode_rs1, top->decode_rs2, top->decode_rs3);
        } else {
            printf("0x%08X : 0x%08X - undecoded instruction\n", address, top->insn_to_decode);
        }
    }

    uint32_t counter = 0;

    while (!Verilated::gotFinish()) {

        if (!top->clock) {
            if(counter < 16) {
                top->inst_ext_address = counter;
                top->inst_ext_write = 0;
            } else if(counter < 32) {
                top->data_ext_address = counter - 16;
                top->data_ext_write = 0;
            } else if(counter < 64) {
                top->reg_ext_address = counter - 32;
            } else {
                break;
            }

        }

        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();

        // After request. Get result of read.
        if (top->clock) {
            /// std::cout << "LED: " << bool(top->led) << "\n";

            if(counter < 16) {
                std::cout << std::setfill('0');
                std::cout << "Reading " << counter << " to get inst value 0x" << std::hex << std::setw(8) << top->inst_ext_out_data << std::dec << std::setw(0) << "\n";
            } else if(counter < 32) {
                std::cout << std::setfill('0');
                std::cout << "Reading " << (counter - 16) << " to get data value 0x" << std::hex << std::setw(8) << top->data_ext_out_data << std::dec << std::setw(0) << "\n";
            } else if(counter < 64) {
                std::cout << std::setfill('0');
                std::cout << "Reading register " << (counter - 32) << " to get value 0x" << std::hex << std::setw(8) << top->reg_ext_out_data << std::dec << std::setw(0) << "\n";
            }

            counter++;
        }

        if (top->clock) {
            std::cout
                << (int) top->Main->test_write_address << " "
                << (int) top->Main->test_write << " "
                << (int) top->Main->test_write_data << ", "
                << (int) top->Main->test_read_address << " "
                << (int) top->Main->test_read_data << ", "
                << (int) top->Main->test_state << "\n";
            // Dump register contents.
            for (int i = 0; i < 32; i++) {
                // Draw in columns.
                int r = i%4*8 + i/4;
                std::cout << std::setfill('0');
                std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x" << std::hex << std::setw(8)
                    << top->Main->registers->bank1->memory[r] << std::dec << std::setw(0) << "   ";
                if (i % 4 == 3) {
                    std::cout << "\n";
                }
            }
            std::cout << "---\n";
        }
    }

    top->final();
    delete top;
}

