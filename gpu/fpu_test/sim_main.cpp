#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <ctgmath>

#include "VMain.h"

void usage(const char* progname)
{
    printf("usage: %s\n", progname);
}

template<typename T>
std::string to_hex(T i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << std::uppercase;
    stream << uint64_t(i);
    return stream.str();
}

union FloatBits {
    float f;
    uint32_t b;
};

uint32_t float_to_bits(float f) {
    FloatBits x;

    x.f = f;

    return x.b;
}

float bits_to_float(uint32_t b) {
    FloatBits x;

    x.b = b;

    return x.f;
}

bool floats_equal(float a, float b, float epsilon) {
    return fabs(a - b) <= (fabs(a) < fabs(b) ? fabs(b) : fabs(a))*epsilon;
}

float rand_float() {
    return float(rand()) / RAND_MAX;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        {
            usage(progname);
            exit(EXIT_FAILURE);
        }
    }

    VMain *top = new VMain;

    // Initial toggle.
    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    top->clock = 1;
    top->eval();

    // Test basic operators.
    float opa = 123.456;
    float opb = 3.1415;

    std::cout << "--------------------------- Basic operators\n";

    for (int fpu_op = 0; fpu_op < 7; fpu_op++) {
        // Set input parameters.
        top->rmode = 0; // round_nearest_even
        top->fpu_op = fpu_op;
        top->opa = float_to_bits(opa);
        top->opb = float_to_bits(opb);

        float expected;
        switch (fpu_op) {
            case 0: expected = 0; break;
            case 1: expected = 0; break;
            case 2: expected = 0; break;
            case 3: expected = opa + opb; break;
            case 4: expected = opa - opb; break;
            case 5: expected = opa * opb; break;
            case 6: expected = opa / opb; break;
        }

        top->clock = 0;
        top->eval();
        top->clock = 1;
        top->eval();

        std::cout << "Operator " << fpu_op << ", output: " << bits_to_float(top->out)
            << " (should be " << expected << ") "
            << (top->inf ? "inf ": "")
            << (top->snan ? "snan " : "")
            << (top->qnan ? "qnan " : "")
            << (top->ine ? "ine " : "")
            << (top->overflow ? "overflow " : "")
            << (top->underflow ? "underflow " : "")
            << (top->zero ? "zero " : "")
            << (top->div_by_zero ? "div_by_zero" : "") << "\n";
    }

    std::cout << "--------------------------- Comparisons\n";

    // Test comparison.
    for (int i = 0; i < 10; i++) {
        float opa = rand_float()*1000 - 500;
        float opb = rand_float()*1000 - 500;
        if (i == 5) {
            opb = opa;
        }

        top->cmp_opa = float_to_bits(opa);
        top->cmp_opb = float_to_bits(opb);

        // Unclocked.
        top->eval();

        std::cout << opa
            << (top->cmp_unordered ? " unordered " : "")
            << (top->cmp_altb ? " < " : "")
            << (top->cmp_blta ? " > " : "")
            << (top->cmp_aeqb ? " == " : "")
            << (top->cmp_inf ? " inf " : "")
            << (top->cmp_zero ? " zero " : "")
            << opb
            << ", should be "
            << (opa < opb ? "<" : opa > opb ? ">" : opa == opb ? "==" : "?")
            << "\n";
    }

    std::cout << "--------------------------- Float to int\n";

    // Test float-to-int.
    for (int i = 0; i < 25; i++) {
        float op;
        switch (i) {
            case 0:
                op = NAN;
                break;

            case 1:
                op = INFINITY;
                break;

            case 2:
                op = -INFINITY;
                break;

            case 3:
                // Overflow positive.
                op = 2000.0*2000.0*2000.0;
                break;

            case 4:
                // Overflow negative.
                op = -2000.0*2000.0*2000.0;
                break;

            case 5:
                // Underflow positive.
                op = 0.1;
                break;

            case 6:
                // Underflow negative.
                op = -0.1;
                break;

            case 8:
                // Positive zero.
                op = 0.0;
                break;

            case 9:
                // Negative zero.
                op = -0.0;
                break;

            case 10:
                op = -0.896246;
                break;

            default:
                op = rand_float()*1000 - 500;
                break;
        }

        top->float_to_int_op = float_to_bits(op);
        top->float_to_int_rmode = 1;

        // Unclocked.
        top->eval();

        std::cout << op
            << " -> "
            << int32_t(top->float_to_int_res)
            << " should be "
            << int32_t(op)
            << (int32_t(op) != int32_t(top->float_to_int_res) ? " WRONG!" : "")
            << "\n";
    }

    std::cout << "--------------------------- Int to float\n";

    // Test float-to-int.
    for (int i = 0; i < 20; i++) {
        int32_t op;
        switch (i) {
            case 0:
                op = 0;
                break;

            case 1:
                op = 1;
                break;

            case 2:
                op = -1;
                break;

            case 3:
                op = 0x80000000;
                break;

            case 4:
                op = 0x7FFFFFFF;
                break;

            default:
                op = int32_t(rand() % (1 << (rand()%29 + 2)));
                if (rand() % 2 == 0) {
                    op = -op;
                }
                break;
        }

        top->int_to_float_op = op;

        // Unclocked.
        top->eval();

        printf("%12d -> %25.10f should be %25.10f%s\n",
                op,
                bits_to_float(top->int_to_float_res),
                float(op),
                (!floats_equal(op, bits_to_float(top->int_to_float_res), 0.0001) ? " WRONG!" : ""));
    }

    top->final();
    delete top;
}

