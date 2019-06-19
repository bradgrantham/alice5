#ifndef PCOPY_H
#define PCOPY_H

// Construct a sequence of instructions to copy a list of registers to
// another list as if they were done in parallel.

#include <inttypes.h>
#include <vector>

// Location we can copy to or from.
struct PCopyLocation {
    // Register number. These values are opaque to the algorithm.
    uint32_t mRegister;

    bool operator==(const PCopyLocation &other) const {
        return mRegister == other.mRegister;
    }
};

// One source and one destination location.
struct PCopyPair {
    PCopyLocation mSource;
    PCopyLocation mDestination;
};

// Operation of instruction.
enum PCopyOp {
    PCOPY_OP_MOVE,
    PCOPY_OP_EXCHANGE,
};

// Instruction that operates on locations.
struct PCopyInstruction {
    PCopyOp mOperation;
    PCopyPair mPair;
};

// Fills the instructions list with a sequence of instructions that, when executed
// in order, will copy the source to the destination as if all the copies had been
// done in parallel. Handles cycles in the copies.
void parallel_copy(const std::vector<PCopyPair> &pairs,
        std::vector<PCopyInstruction> &instructions);

#endif // PCOPY_H
