#ifndef FUNCTION_H
#define FUNCTION_H

#include <string>
#include <map>
#include <set>

#include "risc-v.h"

struct Program;
struct Block;

// Info and blocks in a function.
struct Function {
    // ID of the function.
    uint32_t id;
    uint32_t resultType;
    uint32_t functionControl;
    uint32_t functionType;

    // Back pointer.
    Program *program;

    // Original and clean (assembly-friendly) name.
    std::string name;
    std::string cleanName;

    // ID of the start block.
    uint32_t startBlockId;

    // Map from label ID to Block object.
    std::map<uint32_t, std::shared_ptr<Block>> blocks;

    Function(uint32_t id, const std::string &name, uint32_t resultType,
            uint32_t functionControl, uint32_t functionType, Program *program) :

        id(id), resultType(resultType), functionControl(functionControl),
        functionType(functionType), program(program), name(name), cleanName(cleanUpName(name)),
        startBlockId(NO_BLOCK_ID) {

        // Nothing.
    }

    // Compute the dominance graph and immediate dominance tree.
    void computeDomTree(bool verbose);

    // Break up phi loops.
    void phiLifting();
    void phiLiftingForBlock(Block *block, RiscVPhi *phi);

    // Make sure that we don't use more registers than we have in hardware.
    void ensureMaxRegisters();

    // Compute live in and live out registers for each instruction.
    void computeLiveness();

    // Spill registers if they won't fit in our hardware registers. Returns
    // the ID of the spilled register, or NO_REGISTER if nothing was spilled.
    uint32_t spillIfNecessary(const std::set<uint32_t> &alreadySpilled);

    // Compute set of live ints and floats at this instruction.
    void computeLiveSets(Instruction *instruction,
            std::set<uint32_t> &liveInts,
            std::set<uint32_t> &liveFloats);

    // Spill one of the variables in the set of live floats at this instruction.
    // Returns the ID of the spilled variable.
    uint32_t spillVariable(Instruction *instruction, const std::set<uint32_t> &liveFloats,
            const std::set<uint32_t> &alreadySpilled);

    // Take "mainImage(vf4;vf2;" and return "mainImage$v4f$vf2".
    static std::string cleanUpName(std::string name);

    // Debug dump.
    void dumpBlockInfo() const;
    void dumpGraph(const std::set<uint32_t> &unreached) const;
    void dumpInstructions(const std::string &description) const;
};

#endif // FUNCTION_H
