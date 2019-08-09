#include <inttypes.h>
#include <iostream>

#include "VMain.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    VMain *top = new VMain;
    uint32_t counter = 0;

    while (!Verilated::gotFinish()) {
        if (!top->clock) {
            // About to toggle high. Set up our inputs.
            if (counter < 8) {
                top->ext_address = counter;
                top->ext_write = 1;
                top->ext_in_data = counter*3;
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

