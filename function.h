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

    // Compute liveness and spill variables.
    void spill();

    // Take "mainImage(vf4;vf2;" and return "mainImage$v4f$vf2".
    static std::string cleanUpName(std::string name);

    // Debug dump.
    void dumpBlockInfo() const;
    void dumpGraph(const std::set<uint32_t> &unreached) const;
    void dumpInstructions(const std::string &description) const;
};

#endif // FUNCTION_H
