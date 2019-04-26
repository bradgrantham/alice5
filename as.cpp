
// RISC-V assembler.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <string>
#include <iomanip>

extern "C" {
#include "riscv-disas.h"
}

// Return the pathname without the extension ("file.x" becomes "file").
// If the pathname does not have an extension, it is returned unchanged.
std::string stripExtension(const std::string &pathname) {
    auto slash = pathname.rfind('/');
    auto dot = pathname.rfind('.');
    if (dot == pathname.npos || (slash != pathname.npos && dot < slash)) {
        // No extension.
        return pathname;
    }

    return pathname.substr(0, dot);
}

std::string readFileContents(const std::string &pathname) {
    std::ifstream file(pathname, std::ios::in | std::ios::ate);
    if (!file.good()) {
        std::cerr << "Can't open file \"" << pathname << "\".\n";
        exit(EXIT_FAILURE);
    }
    std::ifstream::pos_type size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    file.read(&text[0], size);

    return text;
}

// ----------------------------------------------------------------------

// Instruction formats.
enum Format {
    FORMAT_R,
    FORMAT_R2,  // Two-operand (one source).
    FORMAT_I,
    FORMAT_IL,  // For loads: same binary as FORMAT_I but different assembly.
    FORMAT_IZ,  // For system instructions. No parameters, they're all zero.
    FORMAT_S,
    FORMAT_SB,
    FORMAT_U,
    FORMAT_UJ,
};

// Information about each type of operator.
struct Operator {
    Format format;

    // Bits 0 through 6.
    uint32_t opcode;

    // Bits 12 through 14.
    uint32_t funct3;

    // Bits 25 through 31.
    uint32_t funct7;

    // Max number of bits in immediate.
    int bits;

    // Whether destination, source1, or source2 are floating point registers.
    bool dIsFloat;
    bool s1IsFloat;
    bool s2IsFloat;

    // Hard-coded rs2 value, for FORMAT_R2 formats.
    int r2;

    Operator() {
        // Nothing.
    }

    Operator(Format format, uint32_t opcode, uint32_t funct3, uint32_t funct7, int bits = 0)
        : format(format), opcode(opcode), funct3(funct3), funct7(funct7), bits(bits),
          dIsFloat(false), s1IsFloat(false), s2IsFloat(false), r2(0)
    {
        // Nothing.
    }

    Operator(const Operator &other) {
        format = other.format;
        opcode = other.opcode;
        funct3 = other.funct3;
        funct7 = other.funct7;
        bits = other.bits;
        dIsFloat = other.dIsFloat;
        s1IsFloat = other.s1IsFloat;
        s2IsFloat = other.s2IsFloat;
    }

    Operator &setDFloat() {
        dIsFloat = true;
        return *this;
    }

    Operator &setS1Float() {
        s1IsFloat = true;
        return *this;
    }

    Operator &setS2Float() {
        s2IsFloat = true;
        return *this;
    }

    Operator &setSFloat() {
        s1IsFloat = true;
        s2IsFloat = true;
        return *this;
    }

    Operator &setAllFloat() {
        dIsFloat = true;
        s1IsFloat = true;
        s2IsFloat = true;
        return *this;
    }

    Operator &setR2(int r2) {
        this->r2 = r2;
        return *this;
    }
};

// ----------------------------------------------------------------------

// Main assembler class.
class Assembler {
private:
    // Map from operator name to operator information.
    std::map<std::string,Operator> operators;

    // Map from register name to register number.
    std::map<std::string,int> registers;

    std::string inPathname;
    bool verbose;

    // Which pass we're doing (0 or 1).
    int pass;

    // Lines of source code.
    std::vector<std::string> lines;

    // Line number in the input file (0-based).
    uint32_t lineNumber;

    // Pointer we walk through the file.
    const char *s;

    // Pointer to the token we just read.
    const char *previousToken;

    // Output binary.
    std::vector<uint32_t> bin;

    // Parallel array to "bin" indicating which source line emitted the instruction.
    std::vector<size_t> binLine;

    // Map from label name to address in bytes.
    std::map<std::string,uint32_t> labels;

public:
    Assembler()
        : inPathname("unspecified") {

        // Build our maps.

        // Basic arithmetic.
        operators["add"]       = Operator{FORMAT_R,  0b0110011, 0b000, 0b0000000};
        operators["sub"]       = Operator{FORMAT_R,  0b0110011, 0b000, 0b0100000};
        operators["sll"]       = Operator{FORMAT_R,  0b0110011, 0b001, 0b0000000};
        operators["slt"]       = Operator{FORMAT_R,  0b0110011, 0b010, 0b0000000};
        operators["sltu"]      = Operator{FORMAT_R,  0b0110011, 0b011, 0b0000000};
        operators["xor"]       = Operator{FORMAT_R,  0b0110011, 0b100, 0b0000000};
        operators["srl"]       = Operator{FORMAT_R,  0b0110011, 0b101, 0b0000000};
        operators["sra"]       = Operator{FORMAT_R,  0b0110011, 0b101, 0b0100000};
        operators["or"]        = Operator{FORMAT_R,  0b0110011, 0b110, 0b0000000};
        operators["and"]       = Operator{FORMAT_R,  0b0110011, 0b111, 0b0000000};

        // Immediates.
        operators["addi"]      = Operator{FORMAT_I,  0b0010011, 0b000, 0b0000000, 12};
        operators["andi"]      = Operator{FORMAT_I,  0b0010011, 0b111, 0b0000000, 12};
        operators["ori"]       = Operator{FORMAT_I,  0b0010011, 0b110, 0b0000000, 12};
        operators["xori"]      = Operator{FORMAT_I,  0b0010011, 0b100, 0b0000000, 12};
        operators["slti"]      = Operator{FORMAT_I,  0b0010011, 0b010, 0b0000000, 12};
        operators["sltiu"]     = Operator{FORMAT_I,  0b0010011, 0b011, 0b0000000, 12};

        // Shifts.
        operators["slli"]      = Operator{FORMAT_I,  0b0010011, 0b001, 0b0000000, 5};
        operators["srli"]      = Operator{FORMAT_I,  0b0010011, 0b101, 0b0000000, 5};
        operators["srai"]      = Operator{FORMAT_I,  0b0010011, 0b101, 0b0100000, 5};

        // Uppers.
        operators["lui"]       = Operator{FORMAT_U,  0b0110111, 0b000, 0b0000000, 22};
        operators["auipc"]     = Operator{FORMAT_U,  0b0010111, 0b000, 0b0000000, 22};

        // Loads.
        operators["lb"]        = Operator{FORMAT_IL, 0b0000011, 0b000, 0b0000000, 12};
        operators["lbu"]       = Operator{FORMAT_IL, 0b0000011, 0b100, 0b0000000, 12};
        operators["lh"]        = Operator{FORMAT_IL, 0b0000011, 0b001, 0b0000000, 12};
        operators["lhu"]       = Operator{FORMAT_IL, 0b0000011, 0b101, 0b0000000, 12};
        operators["lw"]        = Operator{FORMAT_IL, 0b0000011, 0b010, 0b0000000, 12};

        // Stores.
        operators["sb"]        = Operator{FORMAT_S,  0b0100011, 0b000, 0b0100000, 12};
        operators["sh"]        = Operator{FORMAT_S,  0b0100011, 0b001, 0b0100000, 12};
        operators["sw"]        = Operator{FORMAT_S,  0b0100011, 0b010, 0b0100000, 12};

        // Branches and jumps.
        operators["beq"]       = Operator{FORMAT_SB, 0b1100011, 0b000, 0b0000000, 13};
        operators["bne"]       = Operator{FORMAT_SB, 0b1100011, 0b001, 0b0000000, 13};
        operators["blt"]       = Operator{FORMAT_SB, 0b1100011, 0b100, 0b0000000, 13};
        operators["bge"]       = Operator{FORMAT_SB, 0b1100011, 0b101, 0b0000000, 13};
        operators["bltu"]      = Operator{FORMAT_SB, 0b1100011, 0b110, 0b0000000, 13};
        operators["bgeu"]      = Operator{FORMAT_SB, 0b1100011, 0b111, 0b0000000, 13};
        operators["jal"]       = Operator{FORMAT_UJ, 0b1101111, 0b000, 0b0000000, 21};
        operators["jalr"]      = Operator{FORMAT_I,  0b1100111, 0b000, 0b0000000, 12};

        // Floating point loads and stores.
        operators["flw"]       = Operator{FORMAT_IL, 0b0000111, 0b010, 0b0000000, 12}.setDFloat();
        operators["fsw"]       = Operator{FORMAT_S,  0b0100111, 0b010, 0b0000000, 12}.setS2Float();

        // Float point move to/from integer register.
        operators["fmv.x.s"]   = Operator{FORMAT_R2, 0b1010011, 0b000, 0b1110000}.setS1Float()
            .setR2(0b00000);
        operators["fmv.s.x"]   = Operator{FORMAT_R2, 0b1010011, 0b000, 0b1111000}.setDFloat()
            .setR2(0b00000);

        // Floating point sign manipulation.
        operators["fsgnj.s"]   = Operator{FORMAT_R,  0b1010011, 0b000, 0b0010000}.setAllFloat();
        operators["fsgnjn.s"]  = Operator{FORMAT_R,  0b1010011, 0b001, 0b0010000}.setAllFloat();
        operators["fsgnjx.s"]  = Operator{FORMAT_R,  0b1010011, 0b010, 0b0010000}.setAllFloat();

        // Floating point conversion.
        operators["fcvt.w.s"]  = Operator{FORMAT_R2, 0b1010011, 0b111, 0b1100000}.setS1Float()
            .setR2(0b00000);
        operators["fcvt.wu.s"] = Operator{FORMAT_R2, 0b1010011, 0b111, 0b1100000}.setS1Float()
            .setR2(0b00001);
        operators["fcvt.s.w"]  = Operator{FORMAT_R2, 0b1010011, 0b111, 0b1101000}.setDFloat()
            .setR2(0b00000);
        operators["fcvt.s.wu"] = Operator{FORMAT_R2, 0b1010011, 0b111, 0b1101000}.setDFloat()
            .setR2(0b00001);

        // Floating point comparison
        operators["feq.s"]     = Operator{FORMAT_R,  0b1010011, 0b010, 0b1010000}.setSFloat();
        operators["flt.s"]     = Operator{FORMAT_R,  0b1010011, 0b001, 0b1010000}.setSFloat();
        operators["fle.s"]     = Operator{FORMAT_R,  0b1010011, 0b000, 0b1010000}.setSFloat();
        operators["fmin.s"]    = Operator{FORMAT_R,  0b1010011, 0b000, 0b0010100}.setAllFloat();
        operators["fmax.s"]    = Operator{FORMAT_R,  0b1010011, 0b001, 0b0010100}.setAllFloat();
        operators["fclass.s"]  = Operator{FORMAT_R2, 0b1010011, 0b001, 0b1110000}.setS1Float()
            .setR2(0b00000);

        // Floating point math.
        operators["fadd.s"]    = Operator{FORMAT_R,  0b1010011, 0b010, 0b0000000}.setAllFloat();
        operators["fsub.s"]    = Operator{FORMAT_R,  0b1010011, 0b010, 0b0000100}.setAllFloat();
        operators["fmul.s"]    = Operator{FORMAT_R,  0b1010011, 0b010, 0b0001000}.setAllFloat();
        operators["fdiv.s"]    = Operator{FORMAT_R,  0b1010011, 0b010, 0b0001100}.setAllFloat();
        operators["fsqrt.s"]   = Operator{FORMAT_R2, 0b1010011, 0b010, 0b0101100}.setAllFloat()
            .setR2(0b00000);

        // Environment.
        operators["ebreak"]    = Operator{FORMAT_IZ, 0b1110011, 0b000, 0b0000000}.
            setR2(0b00001);

        // Registers.
        addRegisters("x", 0, 31, 0);
        registers["ra"] = 1;
        registers["sp"] = 2;
        registers["gp"] = 3;
        registers["tp"] = 4;
        addRegisters("t", 0, 2, 5);
        registers["fp"] = 8;
        addRegisters("s", 0, 1, 8);
        addRegisters("a", 0, 7, 10);
        addRegisters("s", 2, 11, 18);
        addRegisters("t", 3, 6, 28);
        addRegisters("f", 0, 31, 32);
    }

    void setVerbose(bool verbose) {
        this->verbose = verbose;
    }

    // Load the assembly file.
    void load(const std::string &inPathname) {
        this->inPathname = inPathname;

        // Read the whole assembly file at once.
        std::string in = readFileContents(inPathname);

        // Convert to lines.
        lines.clear();
        const char *s = in.c_str();
        while (true) {
            auto endOfLine = strchr(s, '\n');
            if (endOfLine == nullptr) {
                lines.push_back(s);
                break;
            }
            lines.push_back(std::string(s, endOfLine - s));
            s = endOfLine + 1;
        }
    }

    // Assemble the file to a binary array.
    void assemble() {
        // We do two passes through the code. The first ignores
        // references to labels it doesn't know, but keeps track
        // of where each label ends up in the binary output. The
        // second pass generates an error if it finds a references
        // to an unknown label.
        for (pass = 0; pass < 2; pass++) {
            // Clear output.
            bin.clear();
            binLine.clear();

            // Process each line.
            for (lineNumber = 0; lineNumber < lines.size(); lineNumber++) {
                parseLine();
            }
        }

        emit(0);// XXX remove.
        emit(0);
        emit(0);
        emit(0);
    }

    // Dump the assembly/binary listing.
    void dumpListing() {
        std::ios oldState(nullptr);
        oldState.copyfmt(std::cout);

        // Next instruction to display.
        size_t binIndex = 0;

        // Assume that the source and the binary are in the same order.
        for (size_t sourceLine = 0; sourceLine < lines.size(); sourceLine++) {
            // Next source line to display with an instruction.
            size_t displaySourceLine;

            // Catch up to this source line, if necessary.
            while (true) {
                // See what source line corresponds to the next instruction to display.
                displaySourceLine = binIndex < bin.size()
                    ? binLine[binIndex]
                    : lines.size();

                // See if previous source line generated multiple instructions.
                if (displaySourceLine < sourceLine) {
                    dumpInstructionListing(binIndex, "");
                    binIndex++;
                } else {
                    // No more catching up to do.
                    break;
                }
            }

            if (displaySourceLine == sourceLine) {
                // Found matching source line.
                dumpInstructionListing(binIndex, lines[sourceLine]);
                binIndex++;
            } else {
                // Source line with no instruction. Must be comment, label, blank line, etc.
                std::cout
                    << std::string(15, ' ')
                    << lines[sourceLine] << "\n";
            }
        }

        std::cout.copyfmt(oldState);
    }

    // Dump one instruction with optional source code.
    void dumpInstructionListing(size_t binIndex, const std::string &source) {
        uint32_t pc = binIndex*4;
        uint32_t instruction = bin[binIndex];

        // Print out original code.
        std::cout
            << std::hex << std::setw(4) << std::setfill('0') << pc
            << " "
            << std::hex << std::setw(8) << std::setfill('0') << instruction
            << "  " << source << "\n";

        // Print disassembled code, for comparison.
        char buf[128];
        disasm_inst(buf, sizeof(buf), rv32, pc, instruction);
        std::cout
            << std::string(5, ' ')
            << buf << "\n";
    }

    // Save the binary file.
    void save(const std::string &outPathname) {
        std::ofstream outFile(outPathname, std::ios::out | std::ios::binary);
        if (!outFile.good()) {
            std::cerr << "Can't open file \"" << outPathname << "\".\n";
            exit(EXIT_FAILURE);
        }
        for (uint32_t instruction : bin) {
            // Force little endian.
            outFile
                << uint8_t((instruction >> 0) & 0xFF)
                << uint8_t((instruction >> 8) & 0xFF)
                << uint8_t((instruction >> 16) & 0xFF)
                << uint8_t((instruction >> 24) & 0xFF);
        }
        outFile.close();
    }

private:
    // Add known registers with prefix from "first" to "last" inclusive, starting
    // at physical register "start".
    void addRegisters(const std::string &prefix, int first, int last, int start) {
        for (int i = first; i <= last; i++) {
            std::ostringstream ss;
            ss << prefix << i;
            registers[ss.str()] = i - first + start;
        }
    }

    // Reads one line of input.
    void parseLine() {
        s = currentLine().c_str();
        previousToken = nullptr;

        // Skip initial whitespace.
        skipWhitespace();

        // Grab an identifier. This could be a label or an operator.
        std::string opOrLabel = readIdentifier();

        // See if it's a label.
        if (!opOrLabel.empty()) {
            if (foundChar(':')) {
                // Only keep track of labels in the first pass. We keep
                // them around for the second pass.
                if (pass == 0) {
                    // See if it's been defined before.
                    if (labels.find(opOrLabel) != labels.end()) {
                        s = previousToken;
                        std::ostringstream ss;
                        ss << "Label \"" << opOrLabel << "\" is already defined";
                        error(ss.str());
                    }

                    // It's a label, record it.
                    labels[opOrLabel] = pc();
                } else {
                    // Make sure it hasn't changed.
                    assert(labels.at(opOrLabel) == pc());
                }

                // Read the operator, if any.
                opOrLabel = readIdentifier();
            }
        }

        // See if it's an operator.
        if (!opOrLabel.empty()) {
            // Parse parameters.
            auto opItr = operators.find(opOrLabel);
            if (opItr == operators.end()) {
                s = previousToken;
                std::ostringstream ss;
                ss << "Unknown operator \"" << opOrLabel << "\"";
                error(ss.str());
            }
            const Operator &op = opItr->second;

            switch (op.format) {
                case FORMAT_R: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int rs2 = readRegister(op.s2IsFloat, "source");
                    emitR(op, rd, rs1, rs2);
                    break;
                }

                case FORMAT_R2: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    emitR(op, rd, rs1, op.r2);
                    break;
                }

                case FORMAT_I: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediate(op.bits);
                    emitI(op, rd, rs1, imm);
                    break;
                }

                case FORMAT_IL: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediateOrLabel(op, false);
                    if (!foundChar('(')) {
                        error("Expected open parenthesis");
                    }
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    if (!foundChar(')')) {
                        error("Expected close parenthesis");
                    }
                    emitI(op, rd, rs1, imm);
                    break;
                }

                case FORMAT_IZ: {
                    // No parameters.
                    emitR(op, 0, 0, op.r2);
                    break;
                }

                case FORMAT_S: {
                    int rs2 = readRegister(op.s2IsFloat, "source");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediateOrLabel(op, false);
                    if (!foundChar('(')) {
                        error("Expected open parenthesis");
                    }
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    if (!foundChar(')')) {
                        error("Expected close parenthesis");
                    }
                    emitS(op, rs2, imm, rs1);
                    break;
                }

                case FORMAT_SB: {
                    int rs1 = readRegister(op.s1IsFloat, "source");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int rs2 = readRegister(op.s2IsFloat, "source");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediateOrLabel(op, true);
                    emitSB(op, rs1, rs2, imm);
                    break;
                }

                case FORMAT_U: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediate(op.bits);
                    emitU(op, rd, imm);
                    break;
                }

                case FORMAT_UJ: {
                    int rd = readRegister(op.dIsFloat, "destination");
                    if (!foundChar(',')) {
                        error("Expected comma");
                    }
                    int32_t imm = readImmediateOrLabel(op, true);
                    emitUJ(op, rd, imm);
                    break;
                }

                default: {
                    assert(false);
                }
            }
        }

        // See if there's a comment.
        if (*s == ';') {
            // Skip to end of line.
            s += strlen(s);
        }

        // Make sure the whole line was parsed properly.
        if (*s != '\0') {
            // Unknown error.
            error("Error");
        }
    }

private:
    // Skip non-newline whitespace.
    void skipWhitespace() {
        while (*s == ' ' || *s == '\t' || *s == '\r') {
            s++;
        }
    }

    // Skips a character and subsequent whitespace. Returns whether it found the character.
    bool foundChar(char c) {
        if (*s == c) {
            s++;
            skipWhitespace();
            return true;
        }

        return false;
    }

    // Return the next identifier, or an empty string if there isn't one.
    // An identifier is any sequence of alpha-numeric characters, underscore, or dot, not
    // starting with a digit or dot. Skips subsequent whitespace.
    std::string readIdentifier() {
        std::string id;

        // Keep track of where we started, for error reporting.
        previousToken = s;

        while (isalnum(*s) || *s == '_' || *s == '.') {
            if ((isdigit(*s) || *s == '.') && s == previousToken) {
                // Can't start with digit or dot; this isn't an identifier.
                return "";
            }

            id += *s++;
        }

        skipWhitespace();

        return id;
    }

    // Read a signed integer immediate value. The immediate must fit in the specified
    // number of bits. Skips subsequent whitespace. The immediate can be in
    // decimal or hex (with a 0x prefix).
    int32_t readImmediate(int bits) {
        bool found = false;
        int32_t imm = 0;
        previousToken = s;

        bool negative = *s == '-';
        if (negative) {
            s++;
        }

        if (s[0] == '0' && tolower(s[1]) == 'x') {
            // Hex.
            s += 2;
            while (true) {
                char c = tolower(*s);

                uint32_t value;
                if (isdigit(c)) {
                    value = c - '0';
                } else if (c >= 'a' && c <= 'f') {
                    value = c - 'a' + 10;
                } else {
                    break;
                }
                imm = imm*16 + value;
                found = true;

                s++;
            }
        } else {
            // Decimal.
            while (isdigit(*s)) {
                imm = imm*10 + (*s - '0');
                found = true;
                s++;
            }
        }

        if (!found) {
            s = previousToken;
            error("Expected immediate");
        }

        if (negative) {
            imm = -imm;
        }

        // Make sure we fit.
        int32_t limit = 1 << (bits - 1);
        if (negative ? -imm > limit : imm >= limit) {
            // Back up over immediate.
            s = previousToken;
            std::ostringstream ss;
            ss << "Immediate " << imm << " (0x"
                << std::hex << imm << std::dec << ") does not fit in "
                << bits << (bits == 1 ? " bit" : " bits");
            error(ss.str());
        }

        skipWhitespace();

        return imm;
    }

    // Read a signed immediate or a label. If label, returns
    // as a PC-relative immediate (if pcRelative is true) or
    // an absolute value (if pcRelative is false).
    int32_t readImmediateOrLabel(const Operator &op, bool pcRelative) {
        int32_t imm;

        // See if we're branching to a label or an immediate.
        std::string label = readIdentifier();
        if (label.empty()) {
            imm = readImmediate(op.bits);
        } else {
            uint32_t target;

            // Look up label.
            if (labels.find(label) == labels.end()) {
                if (pass == 0) {
                    // Use anything, it doesn't matter.
                    target = pc();
                } else {
                    s = previousToken;
                    std::ostringstream ss;
                    ss << "Unknown label \"" << label << "\"";
                    error(ss.str());
                }
            } else {
                target = labels.at(label);
            }

            if (pcRelative) {
                // Jump labels are PC-relative.
                imm = target - (pc() + 4);
            } else {
                imm = target;
            }
            // XXX check that it can fit in op.bits.
        }
        if ((imm & 0x1) != 0) {
            // XXX I think this is only true of jump targets.
            s = previousToken;
            error("Immediate must be even");
        }

        return imm;
    }

    // Error function.
    [[noreturn]] void error(const std::string &message) {
        int col = s - currentLine().c_str();

        std::cerr << inPathname << ":" << (lineNumber + 1) << ":"
            << (col + 1) << ": " << message << "\n";
        std::cerr << currentLine() << "\n";
        std::cerr << std::string(col, ' ') << "^\n";
        exit(EXIT_FAILURE);
    }

    // Return a reference to the current line.
    const std::string &currentLine() {
        return lines[lineNumber];
    }

    // Return the PC of the instruction being assembled, in bytes.
    uint32_t pc() {
        return bin.size()*4;
    }

    // Read a register name and return its number. Emits an error
    // if the identifier is missing or is not a register name.
    int readRegister(bool isFloat, const std::string &role) {
        std::string regName = readIdentifier();
        if (regName.empty()) {
            std::ostringstream ss;
            ss << "Expected " << role << " register";
            error(ss.str());
        }

        auto regItr = registers.find(regName);
        if (regItr == registers.end()) {
            s = previousToken;
            std::ostringstream ss;
            ss << "\"" << regName << "\" is not a register name";
            error(ss.str());
        }

        int reg = regItr->second;

        // Check that register is of the right type.
        if (isFloat) {
            if (reg < 32) {
                s = previousToken;
                std::ostringstream ss;
                ss << "Expected float register for " << role;
                error(ss.str());
            } else {
                reg -= 32;
            }
        } else {
            if (reg >= 32) {
                s = previousToken;
                std::ostringstream ss;
                ss << "Expected integer register for " << role;
                error(ss.str());
            }
        }

        return reg;
    }

    // Emit a FORMAT_R instruction.
    void emitR(const Operator &op, int rd, int rs1, int rs2) {
        emit(op.opcode
                | rd << 7
                | op.funct3 << 12
                | rs1 << 15
                | rs2 << 20
                | op.funct7 << 25);
    }

    // Emit a FORMAT_I instruction.
    void emitI(const Operator &op, int rd, int rs1, int32_t imm) {
        emit(op.opcode
                | rd << 7
                | op.funct3 << 12
                | rs1 << 15
                | imm << 20
                | op.funct7 << 25);
    }

    // Emit a FORMAT_S instruction.
    void emitS(const Operator &op, int rs2, int32_t imm, int rs1) {
        emit(op.opcode
                | (imm & 0x1F) << 7
                | op.funct3 << 12
                | rs1 << 15
                | rs2 << 20
                | ((imm >> 5) & 0x7F) << 25);
    }

    // Emit a FORMAT_SB instruction.
    void emitSB(const Operator &op, int rs1, int rs2, int32_t imm) {
        emit(op.opcode
                | ((imm >> 11) & 0x1) << 7
                | (imm & 0x1E) << 7
                | op.funct3 << 12
                | rs1 << 15
                | rs2 << 20
                | ((imm >> 5) & 0x3F) << 25
                | ((imm >> 12) & 0x1) << 31);
    }

    // Emit a FORMAT_U instruction.
    void emitU(const Operator &op, int rd, int32_t imm) {
        emit(op.opcode
                | rd << 7
                | imm << 12);
    }

    // Emit a FORMAT_UJ instruction.
    void emitUJ(const Operator &op, int rd, int32_t imm) {
        emit(op.opcode
                | rd << 7
                | ((imm >> 12) & 0xFF) << 12
                | ((imm >> 11) & 0x1) << 20
                | ((imm >> 1) & 0x3FF) << 21
                | ((imm >> 20) & 0x1) << 31);
    }

    // Emit an instruction for this source line.
    void emit(uint32_t instruction) {
        bin.push_back(instruction);
        binLine.push_back(lineNumber);
    }
};

// ----------------------------------------------------------------------

void usage(char *progname) {
    std::cerr << "Usage: " << progname << " [options] file.s\n";
    std::cerr << "Options:\n";
    std::cerr << "    -v         verbose output\n";
    std::cerr << "    -o file.o  output object file\n";
}

int main(int argc, char *argv[]) {
    bool verbose = false;
    std::string inPathname;
    std::string outPathname;

    char *progname = argv[0];
    argv++; argc--;

    // Parse parameters.
    while (argc > 0 && argv[0][0] == '-') {
        if (strcmp(argv[0], "-v") == 0) {
            verbose = true;
            argv++; argc--;
        } else if (strcmp(argv[0], "-o") == 0) {
            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            outPathname = argv[1];
            argv += 2; argc -= 2;
        } else {
            usage(progname);
            exit(EXIT_FAILURE);
        }
    }

    if (argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    inPathname = argv[0];

    if (outPathname.empty()) {
        outPathname = stripExtension(inPathname) + ".o";
    }

    Assembler assembler;
    assembler.setVerbose(verbose);
    assembler.load(inPathname);
    assembler.assemble();
    assembler.save(outPathname);
    if (verbose) {
        assembler.dumpListing();
    }
}

