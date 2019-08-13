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
    std::vector<uint8_t> text_bytes;
    std::vector<uint8_t> data_bytes;
    SymbolTable text_symbols;
    SymbolTable data_symbols;

    std::ifstream binaryFile(argv[0], std::ios::in | std::ios::binary);
    if(!binaryFile.good()) {
        throw std::runtime_error(std::string("couldn't open file ") + argv[0] + " for reading");
    }

    if(!ReadBinary(binaryFile, header, text_symbols, data_symbols, text_bytes, data_bytes)) {
        exit(EXIT_FAILURE);
    }
    assert(text_bytes.size() % 4 == 0);

    VMain *top = new VMain;

    // Run one clock to propagate reset (??)
    for(int i = 0; i < 2; i++) {
        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();
    }

    // Write text bytes to text memory
    for(uint32_t address = 0; address < text_bytes.size(); address += 4) {
        top->ext_address = address;
        top->ext_write = 1;
        uint32_t text_word = 
            text_bytes[address + 0] << 24 |
            text_bytes[address + 1] << 16 |
            text_bytes[address + 2] <<  8 |
            text_bytes[address + 3] <<  0;
        top->ext_in_data = text_word;

        if(beVerbose) {
            std::cout << std::setfill('0');
            std::cout << "Writing to " << address << " value 0x" << std::hex << std::setw(8) << top->ext_in_data << std::dec << std::setw(0) << "\n";
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
            // About to toggle high. Set up our inputs.
            if (counter < 8) {
                top->ext_address = counter;
                top->ext_write = 1;
                top->ext_in_data = counter*3 + 1;
                std::cout << "Writing to " << counter << " value " << top->ext_in_data << "\n";
            } else if (counter < 16) {
                top->ext_address = counter - 8;
                top->ext_write = 0;
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

            if (counter >= 8) {
                std::cout << "Reading " << (counter - 8) << " to get " << top->ext_out_data << "\n";
            }

            counter++;
        }
    }

    top->final();
    delete top;
}

