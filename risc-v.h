
// Instructions that map to the RISC-V ISA.

#include "basic_types.h"

enum {
    RiscVOpAddi = 16384, // XXX not sure what's a safe number to start with.
    RiscVOpLoad,
    RiscVOpStore,
    RiscVOpCross,
};

// "addi" instruction.
struct RiscVAddi : public Instruction {
    RiscVAddi(LineInfo& lineInfo, uint32_t type, uint32_t resultId, uint32_t rs1, uint32_t imm) : Instruction(lineInfo), type(type), resultId(resultId), rs1(rs1), imm(imm) {
        addResult(resultId);
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
    RiscVLoad(const LineInfo& lineInfo, uint32_t type, uint32_t resultId, uint32_t pointerId, uint32_t memoryAccess, uint32_t offset) : Instruction(lineInfo), type(type), resultId(resultId), pointerId(pointerId), memoryAccess(memoryAccess), offset(offset) {
        addResult(resultId);
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
    RiscVStore(const LineInfo& lineInfo, uint32_t pointerId, uint32_t objectId, uint32_t memoryAccess, uint32_t offset) : Instruction(lineInfo), pointerId(pointerId), objectId(objectId), memoryAccess(memoryAccess), offset(offset) {
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

// Cross product instruction.
struct RiscVCross : public Instruction {
    RiscVCross(const LineInfo& lineInfo,
            uint32_t resultId0,
            uint32_t resultId1,
            uint32_t resultId2,
            uint32_t xId0,
            uint32_t xId1,
            uint32_t xId2,
            uint32_t yId0,
            uint32_t yId1,
            uint32_t yId2)
        : Instruction(lineInfo),
        resultId0(resultId0),
        resultId1(resultId1),
        resultId2(resultId2),
        xId0(xId0),
        xId1(xId1),
        xId2(xId2),
        yId0(yId0),
        yId1(yId1),
        yId2(yId2)
    {
        addResult(resultId0);
        addResult(resultId1);
        addResult(resultId2);
        addParameter(xId0);
        addParameter(xId1);
        addParameter(xId2);
        addParameter(yId0);
        addParameter(yId1);
        addParameter(yId2);
    }
    uint32_t resultId0;
    uint32_t resultId1;
    uint32_t resultId2;
    uint32_t xId0;
    uint32_t xId1;
    uint32_t xId2;
    uint32_t yId0;
    uint32_t yId1;
    uint32_t yId2;
    virtual void step(Interpreter *interpreter) { assert(false); }
    virtual uint32_t opcode() const { return RiscVOpCross; }
    virtual std::string name() const { return "cross"; }
    virtual void emit(Compiler *compiler);
};
