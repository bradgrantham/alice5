#include <iostream>

#include "VMain.h"

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    VMain *top = new VMain;

    while (!Verilated::gotFinish()) {
        // Toggle clock.
        top->clock = top->clock ^ 1;

        // Eval all Verilog code.
        top->eval();

        std::cout << "LED: " << bool(top->led) << "\n";
    }

    top->final();
    delete top;
}

