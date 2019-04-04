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

    const Program *pgm;

    Interpreter(const Program *pgm);

    virtual ~Interpreter()
    {
        delete[] memory;
    }

    Register& allocRegister(uint32_t id, uint32_t type);

    // Copy a variable of type "type" from "src" to "dst" in memory.
    void copy(uint32_t type, size_t src, size_t dst);

    // Pointer to object in memory in specified storage class at specified offset.
    template <class T>
    T& objectInClassAt(SpvStorageClass clss, size_t offset);

    // Get a register's data by ID as the specified type.
    template <class T>
    T& registerAs(int id);

    template <class T>
    void set(SpvStorageClass clss, size_t offset, const T& v);
    template <class T>
    void get(SpvStorageClass clss, size_t offset, T& v);

    void clearPrivateVariables();

    void step();
    void run();

    // Opcode step declarations.
#include "opcode_decl.h"
};

#endif // INTERPRETER_H
