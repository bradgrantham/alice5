#include <inttypes.h>
#include <iomanip>
#include <iostream>

#include "objectfile.h"

#include "VMain.h"

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

    // Run one clock to propagate reset (??)
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
            inst_bytes[address + 0] << 24 |
            inst_bytes[address + 1] << 16 |
            inst_bytes[address + 2] <<  8 |
            inst_bytes[address + 3] <<  0;
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
            data_bytes[address + 0] << 24 |
            data_bytes[address + 1] << 16 |
            data_bytes[address + 2] <<  8 |
            data_bytes[address + 3] <<  0;
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

    uint32_t counter = 0;

    while (!Verilated::gotFinish()) {

        if (!top->clock) {
            if(counter < 16) {
                top->inst_ext_address = counter;
                top->inst_ext_write = 0;
            } else if(counter < 32) {
                top->data_ext_address = counter - 16;
                top->data_ext_write = 0;
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
            std::cout << "LED: " << bool(top->led) << "\n";

            if(counter < 16) {
                std::cout << std::setfill('0');
                std::cout << "Reading " << counter << " to get inst value 0x" << std::hex << std::setw(8) << top->inst_ext_out_data << std::dec << std::setw(0) << "\n";
            } else if(counter < 32) {
                std::cout << std::setfill('0');
                std::cout << "Reading " << (counter - 16) << " to get data value 0x" << std::hex << std::setw(8) << top->data_ext_out_data << std::dec << std::setw(0) << "\n";
            }

            counter++;
        }
    }

    top->final();
    delete top;
}

