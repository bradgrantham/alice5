
// Instructions that map to the RISC-V ISA.

#include "basic_types.h"

enum {
    RiscVOpAddi = 16384, // XXX not sure what's a safe number to start with.
    RiscVOpLoad,
    RiscVOpStore,
};

// "addi" instruction.
struct RiscVAddi : public Instruction {
    RiscVAddi(LineInfo& lineInfo_, uint32_t type, uint32_t resultId, uint32_t rs1, uint32_t imm) : Instruction(lineInfo_, resultId), type(type), resultId(resultId), rs1(rs1), imm(imm) {
        addParameter(rs1);
    }
    uint32_t type; // result type
    uint32_t resultId; // SSA register for result value
    uint32_t rs1; // operand from register
    uint32_t imm; // 12-bit immediate
    virtual void step(Interpreter *interpreter) { assert(false); }
    virtual uint32_t opcode() const { return RiscVOpAddi; }
    virtual std::string name() const { return "addi"; }
    virtual void emit(Compiler *compiler);
};

// Load instruction (int or float).
struct RiscVLoad : public Instruction {
    RiscVLoad(const LineInfo& lineInfo_, uint32_t type, uint32_t resultId, uint32_t pointerId, uint32_t memoryAccess, uint32_t offset) : Instruction(lineInfo_, resultId), type(type), resultId(resultId), pointerId(pointerId), memoryAccess(memoryAccess), offset(offset) {
        addParameter(pointerId);
    }
    uint32_t type; // result type
    uint32_t resultId; // SSA register for result value
    uint32_t pointerId; // operand from register
    uint32_t memoryAccess; // MemoryAccess (optional)
    uint32_t offset; // In bytes.
    virtual void step(Interpreter *interpreter) { assert(false); }
    virtual uint32_t opcode() const { return RiscVOpLoad; }
    virtual std::string name() const { return "load"; }
    virtual void emit(Compiler *compiler);
};

// Store instruction (int or float).
struct RiscVStore : public Instruction {
    RiscVStore(const LineInfo& lineInfo_, uint32_t pointerId, uint32_t objectId, uint32_t memoryAccess, uint32_t offset) : Instruction(lineInfo_, NO_REGISTER), pointerId(pointerId), objectId(objectId), memoryAccess(memoryAccess), offset(offset) {
        addParameter(pointerId);
        addParameter(objectId);
    }
    uint32_t pointerId; // operand from register
    uint32_t objectId; // operand from register
    uint32_t memoryAccess; // MemoryAccess (optional)
    uint32_t offset; // In bytes.
    virtual void step(Interpreter *interpreter) { assert(false); }
    virtual uint32_t opcode() const { return RiscVOpStore; }
    virtual std::string name() const { return "store"; }
    virtual void emit(Compiler *compiler);
};
