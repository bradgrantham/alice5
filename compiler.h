#ifndef COMPILER_H
#define COMPILER_H

#include "program.h"

// Virtual register used by the compiler.
struct CompilerRegister {
    // Type of the data.
    uint32_t type;

    // Number of words of data in this register.
    int count;

    // Physical registers.
    std::vector<uint32_t> phy;

    CompilerRegister()
        : type(NO_REGISTER)
    {
        // Nothing.
    }

    CompilerRegister(uint32_t type, int count)
        : type(type), count(count)
    {
        // Nothing.
    }
};

// Class of all virtual registers that participate (transitively) in
// the same phi instruction.
struct PhiClass {
    // Non-empty set of virtual registers in this class.
    std::set<uint32_t> registers;

    // Return an arbitrary (but stable) register for this class.
    uint32_t getId() const {
        return *registers.begin();
    }
};

// Compiles a Program to our ISA.
struct Compiler {
    const Program *pgm;
    std::map<uint32_t, CompilerRegister> registers;
    uint32_t localLabelCounter;

    // Compiler's own list of instructions, with its modifications.
    InstructionList instructions;

    // Map from each register to the class that it's in. There's only an
    // entry here if the register participates in a phi instruction.
    std::map<uint32_t,std::shared_ptr<PhiClass>> phiClassMap;

    Compiler(const Program *pgm)
        : pgm(pgm), localLabelCounter(1)
    {
        // Nothing.
    }

    void compile();

    // Emit constant value for the specified constant ID.
    void emitConstant(uint32_t id, uint32_t typeId, unsigned char *data);

    // If the virtual register "id" points to an integer constant, returns it
    // in "value" and returns true. Otherwise returns false and leaves value
    // untouched.
    bool asIntegerConstant(uint32_t id, uint32_t &value) const;

    // Returns true if the register is a float, false if it's an integer,
    // otherwise asserts.
    bool isRegFloat(uint32_t id) const;

    // Make a new label that can be used for local jumps.
    std::string makeLocalLabel();

    // Transform SPIR-V instructions to RISC-V instructions. Creates a new
    // "instructions" array that's local to the compiler and is closer to
    // machine instructions, but still using SSA. inList and outList
    // may be the same list.
    void transformInstructions(const InstructionList &inList, InstructionList &outList);

    // Get rid of phi instructions.
    void translateOutOfSsa();

    // Add register to phi class and phi class map, replacing phiClass if
    // the register is already part of a class.
    void processPhiRegister(uint32_t regId, std::shared_ptr<PhiClass> &phiClass);

    // Assign all physical registers.
    void assignRegisters();

    // Assigns registers for this block.
    void assignRegisters(Block *block,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy);

    // Determine whether these two virtual register IDs are currently assigned
    // to the same physical register. Returns false if one or both aren't
    // assigned at all. Index is the index into the vector, if it is one.
    bool isSamePhysicalRegister(uint32_t id1, uint32_t id2, int index) const;

    // Returns the SSA ID as a compiler register, or null if it's not stored
    // in a compiler register.
    const CompilerRegister *asRegister(uint32_t id) const;

    // String for a virtual register ("r12" or more).
    std::string reg(uint32_t id, int index = 0) const;
    void emitNotImplemented(const std::string &op);
    void emitUnaryOp(const std::string &opName, int result, int op);
    void emitBinaryOp(const std::string &opName, int result, int op1, int op2);
    void emitBinaryImmOp(const std::string &opName, int result, int op, uint32_t imm);
    void emitLabel(const std::string &label);

    // Return the label passed in, unless it's an empty string, in which case
    // it returns an appropriate non-empty string.
    std::string notEmptyLabel(const std::string &label) const;

    // Emit a call to a floating-point routine.
    void emitCall(const std::string &functionName,
            const std::vector<uint32_t> resultIds,
            const std::vector<uint32_t> operandIds);

    // Emit a call to a single-parameter floating-point routine.
    void emitUniCall(const std::string &functionName, uint32_t resultId, uint32_t operandId);

    // Emit a call to a two-parameter floating-point routine.
    void emitBinCall(const std::string &functionName, uint32_t resultId,
            uint32_t operand1Id, uint32_t operand2Id);
    void emit(const std::string &op, const std::string &comment);

    // Just before a Branch or BranchConditional instruction, copy any
    // registers that a target OpPhi instruction might need.
    /// XXX delete void emitPhiCopy(Instruction *instruction, uint32_t labelId);

    // Assert that the block at the label ID does not start with a Phi instruction.
    void assertNoPhi(uint32_t labelId);
};

#endif // COMPILER_H
