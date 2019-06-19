// Test for the parallel_copy() function in pcopy.cpp.

#include <iostream>
#include "pcopy.h"

constexpr bool DEBUG_PRINT = false;

void verify_results(const std::string &name, const std::vector<PCopyPair> &pairs) {
    if (DEBUG_PRINT) {
        std::cout << name << ":\n";
    }

    std::vector<PCopyInstruction> instructions;

    // See what the algorithm says to do.
    parallel_copy(pairs, instructions);

    // Make up fake registers where the value is the register number.
    std::vector<uint32_t> r;
    for (int i = 0; i < 64; i++) {
        r.push_back(i);
    }

    // Execute the instructions.
    for (auto &instruction : instructions) {
        switch (instruction.mOperation) {
            case PCOPY_OP_MOVE:
                if (DEBUG_PRINT) {
                    std::cout << "    Executing move r"
                        << instruction.mPair.mDestination.mRegister 
                        << " <- r"
                        << instruction.mPair.mSource.mRegister
                        << "\n";
                }
                r[instruction.mPair.mDestination.mRegister] = 
                    r[instruction.mPair.mSource.mRegister];
                break;

            case PCOPY_OP_EXCHANGE: {
                if (DEBUG_PRINT) {
                    std::cout << "    Executing exchange r"
                        << instruction.mPair.mDestination.mRegister 
                        << " <-> r"
                        << instruction.mPair.mSource.mRegister
                        << "\n";
                }
                uint32_t tmp = r[instruction.mPair.mDestination.mRegister];
                r[instruction.mPair.mDestination.mRegister] = 
                    r[instruction.mPair.mSource.mRegister];
                r[instruction.mPair.mSource.mRegister] = tmp;
                break;
            }

            default:
                assert(false);
                break;
        }
    }

    // Make sure the changes worked.
    for (auto &pair : pairs) {
        uint32_t actual = r[pair.mDestination.mRegister];
        uint32_t expected = pair.mSource.mRegister;

        if (actual != expected) {
            std::cout << name << ": expected " << expected << " but got " << actual
                << " in register " << pair.mDestination.mRegister << std::endl;
        }
    }
}

int main() {
    std::vector<PCopyPair> pairs;

    pairs.clear();
    pairs.push_back({{1}, {1}});
    pairs.push_back({{2}, {2}});
    pairs.push_back({{3}, {3}});
    verify_results("simple", pairs);

    pairs.clear();
    pairs.push_back({{2}, {1}});
    pairs.push_back({{2}, {2}});
    pairs.push_back({{2}, {3}});
    verify_results("fan-out", pairs);

    pairs.clear();
    pairs.push_back({{2}, {1}});
    pairs.push_back({{2}, {2}});
    pairs.push_back({{1}, {3}});
    verify_results("ordering", pairs);

    pairs.clear();
    pairs.push_back({{1}, {1}});
    verify_results("cycle-1", pairs);

    pairs.clear();
    pairs.push_back({{1}, {2}});
    pairs.push_back({{2}, {1}});
    verify_results("cycle-2", pairs);

    pairs.clear();
    pairs.push_back({{1}, {2}});
    pairs.push_back({{2}, {3}});
    pairs.push_back({{3}, {1}});
    verify_results("cycle-3", pairs);

    pairs.clear();
    pairs.push_back({{1}, {2}});
    pairs.push_back({{2}, {3}});
    pairs.push_back({{3}, {1}});
    pairs.push_back({{5}, {4}});
    pairs.push_back({{5}, {5}});
    pairs.push_back({{4}, {6}});
    verify_results("complex", pairs);

    // Try random things.
    for (int i = 0; i < 1000; i++) {
        pairs.clear();
        for (uint32_t dst = 1; dst <= 60; dst++) {
            uint32_t src = rand()%60 + 1;
            pairs.push_back({{src}, {dst}});
        }
        verify_results("random", pairs);
    }
}

