#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "basic_types.h"
#include "opcode_struct_decl.h"

struct Program;

// Dynamic state of the program (registers, call stack, ...).
struct Interpreter
{
    uint32_t pc;
    std::vector<size_t> callstack;
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
    void checkMemory(size_t offset, size_t size);
    // Mark this memory region as initialized.
    void markMemory(size_t offset, size_t size);

    // Pointer to object in memory in specified storage class at specified offset.
    template <class T>
    T& objectInClassAt(SpvStorageClass clss, size_t offset, bool reading, size_t size);

    template <class T>
    T& objectInClassAt(size_t addr, bool reading, size_t size);

    // Get a register's data by ID as the specified type.
    template <class T>
    T& registerAs(int id);

    template <class T>
    void set(SpvStorageClass clss, size_t offset, const T& v);
    template <class T>
    void get(SpvStorageClass clss, size_t offset, T& v);

    template <class T>
    void set(const std::string& name, const T& v);

    void clearPrivateVariables();

    void step();
    void run();

    // Opcode step declarations.
#include "opcode_decl.h"
};

#endif // INTERPRETER_H
