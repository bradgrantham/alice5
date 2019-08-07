#ifndef COMPILER_H
#define COMPILER_H

#include <fstream>
#include "program.h"

// Virtual register used by the compiler.
struct CompilerRegister {
    // Type of the data.
    uint32_t type;

    // Number of words of data in this register.
    int count;

    // Physical register, or NO_REGISTER if not yet assigned.
    uint32_t phy;

    CompilerRegister()
        : type(0), count(0), phy(NO_REGISTER)
    {
        // Nothing.
    }

    CompilerRegister(uint32_t type, int count)
        : type(type), count(count), phy(NO_REGISTER)
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
    Program *pgm;
    // Map from virtual register to our own info about the register.
    std::map<uint32_t, CompilerRegister> registers;
    uint32_t localLabelCounter;

    // Map from each register to the class that it's in. There's only an
    // entry here if the register participates in a phi instruction.
    std::map<uint32_t,std::shared_ptr<PhiClass>> phiClassMap;

    std::ofstream outFile;

    Compiler(Program *pgm, const std::string &outputAssemblyPathname)
        : pgm(pgm),
          localLabelCounter(1),
          outFile(outputAssemblyPathname, std::ios::out)
    {
        if (!outFile.good()) {
            std::cerr << "Can't open file \"" << outputAssemblyPathname << "\".\n";
            exit(EXIT_FAILURE);
        }
    }

    void compile();
    void emitInstructions();
    void emitInstructionsForFunction(Function *function);
    void emitInstructionsForBlockTree(Block *block);
    void emitInstructionsForBlock(Block *block);
    void emitVariables();
    void emitConstants();
    void emitLibrary();

    // Emit constant value for the specified constant ID.
    void emitConstant(uint32_t id, uint32_t typeId, unsigned char *data);

    // If the virtual register "id" points to an integer constant, returns it
    // in "value" and returns true. Otherwise returns false and leaves value
    // untouched.
    bool asIntegerConstant(uint32_t id, uint32_t &value) const;

    // Returns true if the register is a float, false if it's an integer,
    // otherwise asserts.
    bool isRegFloat(uint32_t id) const;

    // Return the name of the variable with the specified ID. If no name was in
    // the SPIR-V file, a unique name is generated based on the ID.
    std::string getVariableName(uint32_t id) const;

    // Make a new label that can be used for local jumps.
    std::string makeLocalLabel();

    // Transform SPIR-V instructions to RISC-V instructions.
    void transformInstructions(InstructionList &inList);

    // Get rid of phi instructions.
    void translateOutOfSsa();

    // Compute the phi equivalent classes.
    void computePhiClassMap();

    // Add register to phi class and phi class map, replacing phiClass if
    // the register is already part of a class.
    void processPhiRegister(uint32_t regId, std::shared_ptr<PhiClass> &phiClass);

    // Assign all physical registers.
    void assignRegisters();

    // Assigns registers for this function.
    void assignRegistersForFunction(const Function *function,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy);

    // Assigns registers for this block tree.
    void assignRegistersForBlockTree(Block *block,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy);

    // Assigns registers for this block.
    void assignRegistersForBlock(Block *block,
            const std::set<uint32_t> &allIntPhy,
            const std::set<uint32_t> &allFloatPhy);

    // Return the physical register for the virtual register, or NO_REGISTER
    // if it hasn't been assigned yet. If required, the program fails if
    // the ID has no physical register.
    uint32_t physicalRegisterFor(uint32_t id, bool required = false) const;

    // Determine whether these two virtual register IDs are currently assigned
    // to the same physical register. Returns false if one or both aren't
    // assigned at all.
    bool isSamePhysicalRegister(uint32_t id1, uint32_t id2) const;

    // Returns the SSA ID as a compiler register, or null if it's not stored
    // in a compiler register.
    const CompilerRegister *asRegister(uint32_t id) const;

    // String for a virtual register ("r12" or more).
    std::string reg(uint32_t id) const;

    void emitNotImplemented(const std::string &op);
    void emitUnaryOp(const std::string &opName, int result, int op);
    void emitBinaryOp(const std::string &opName, int result, int op1, int op2);
    void emitBinaryImmOp(const std::string &opName, int result, int op, uint32_t imm);
    void emitLabel(const std::string &label);
    void emitCopyVariable(uint32_t dst, uint32_t src, const std::string &comment);
    void emitCopyRegister(uint32_t dst, uint32_t src, const std::string &comment);

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

    // Emit a call to a three-parameter floating-point routine.
    void emitTerCall(const std::string &functionName, uint32_t resultId,
            uint32_t operand1Id, uint32_t operand2Id, uint32_t operand3Id);

    void emit(const std::string &op, const std::string &comment);

    // Just before a Branch or BranchConditional instruction, copy any
    // registers that a target OpPhi instruction might need. Instruction
    // is the branch; labelId is the target whose block has a phi.
    void emitPhiCopy(Instruction *instruction, uint32_t labelId);
};

#endif // COMPILER_H
