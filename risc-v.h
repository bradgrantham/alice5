
// Instructions that map to the RISC-V ISA.

#include "basic_types.h"

enum {
    RiscVOpAddi = 16384, // XXX not sure what's a safe number to start with.
};

// "addi" instruction.
struct RiscVAddi : public Instruction {
    RiscVAddi(LineInfo& lineInfo_, uint32_t type, uint32_t resultId, uint32_t rs1, uint32_t imm) : Instruction(lineInfo_, resultId), type(type), resultId(resultId), rs1(rs1), imm(imm) {
        argIds.insert(rs1);
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
