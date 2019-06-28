#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "basic_types.h"
#include "opcode_struct_decl.h"

struct Program;

const size_t MEMORY_CHECK_OKAY = 0xFFFFFFFF;

// Dynamic state of the program (registers, call stack, ...).
struct Interpreter
{
    Instruction *instruction;
    std::vector<Instruction *> returnStack;
    std::vector<uint32_t> parameterStack;
    std::map<uint32_t, Register> registers;
    std::map<uint32_t, Pointer> pointers;

    // These values are label IDs identifying blocks within a function. The current block
    // is the block we're executing. The previous block was the block we came from.
    // These are NO_BLOCK_ID if not yet set.
    uint32_t currentBlockId;
    uint32_t previousBlockId;

    unsigned char *memory;
    bool *memoryInitialized;

    const Program *pgm;

    Interpreter(const Program *pgm);

    virtual ~Interpreter()
    {
        delete[] memory;
        delete[] memoryInitialized;
    }

    // Check that this memory region has been initialized.
    size_t checkMemory(size_t offset, size_t size);
    // Mark this memory region as initialized.
    void markMemory(size_t offset, size_t size);

    // Pointer to object in memory at specified address.
    template <class T>
    T& objectInMemoryAt(size_t addr, bool reading, size_t size);

    // Pointer to object in memory in specified storage class at specified offset.
    template <class T>
    T& objectInClassAt(SpvStorageClass clss, size_t offset, bool reading, size_t size);

    // For reading from a register.
    template <class T>
    const T& fromRegister(int id);
    // For writing to a register.
    template <class T>
    T& toRegister(int id);

    template <class T>
    void set(SpvStorageClass clss, size_t offset, const T& v);
    template <class T>
    void get(SpvStorageClass clss, size_t offset, T& v);

    template <class T>
    void set(const std::string& name, const T& v);

    void clearPrivateVariables();

    // Jump to the specified block in the specified function, or in the current
    // function if "function" is nullptr.
    void jumpToBlock(uint32_t blockId) {
        assert (instruction != nullptr);

        Function *function = instruction->list->block->function;
        instruction = function->blocks.at(blockId)->instructions.head.get();
    }

    void jumpToFunction(const Function *function) {
        instruction = function->blocks.at(function->startBlockId)->instructions.head.get();
    }

    void step();
    void run();

    // Opcode step declarations.
#include "opcode_decl.h"
};

#endif // INTERPRETER_H
