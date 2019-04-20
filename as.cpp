
// RISC-V assembler.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <string>
#include <iomanip>

// Map from label name to address in bytes.
std::map<std::string,uint32_t> gLabels;

// Types of operators.
enum OpType {
    OP_TYPE_I,
};

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

std::string readFileContents(const std::string &pathname)
{
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

class Assembler {
private:
    // Map from operator to instruction type.
    std::map<std::string,OpType> operators;

    // Map from register name to register number.
    std::map<std::string,int> registers;

    std::string inPathname;
    std::string outPathname;
    bool verbose;

    // Line number in the input file.
    uint32_t lineNumber;

    // Pointer we walk through the file.
    const char *s;

    // Pointer to start of the current line.
    const char *lineStart;

    // Pointer to the token we just read.
    const char *previousToken;

    // Output binary.
    std::vector<uint32_t> bin;

public:
    Assembler(const std::string &inPathname, const std::string &outPathname)
        : inPathname(inPathname), outPathname(outPathname) {

        // Build our maps.
        operators["addi"] = OP_TYPE_I;

        for (int reg = 0; reg < 32; reg++) {
            std::ostringstream ss;
            ss << "x" << reg;
            registers[ss.str()] = reg;
        }
    }

    void setVerbose(bool verbose) {
        this->verbose = verbose;
    }

    // Assemble the file to the object file.
    void assemble() {
        // Read the whole assembly file at once.
        std::string in = readFileContents(inPathname);

        lineNumber = 1;
        s = in.c_str();
        lineStart = s;
        previousToken = nullptr;
        bin.clear();

        while (*s != '\0') {
            size_t startSize = bin.size();
            std::string line = currentLine();

            // Read one line.
            readLine();

            if (verbose && bin.size() == startSize) {
                // Didn't advance, must be comment or blank line.
                std::cout << std::string(14, ' ') << line << "\n";
            }
        }

        // Write binary.
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
    // Reads one line of input, either one character past
    // the newline, or on the nul if there is no newline.
    void readLine() {
        // Start of line. Skip whitespace.
        skipWhitespace();

        // Grab a token. This could be a label or an operator.
        std::string opOrLabel = readToken();

        // See if it's a label.
        if (!opOrLabel.empty()) {
            if (foundChar(':')) {
                if (gLabels.find(opOrLabel) != gLabels.end()) {
                    s = previousToken;
                    std::ostringstream ss;
                    ss << "Label \"" << opOrLabel << "\" is already defined";
                    error(ss.str());
                }

                // It's a label, record it.
                gLabels[opOrLabel] = bin.size()*4;

                // Read the operator.
                opOrLabel = readToken();
            }
        }

        // See if it's an operator.
        if (!opOrLabel.empty()) {
            // Parse parameters.
            auto opType = operators.find(opOrLabel);
            if (opType == operators.end()) {
                s = previousToken;
                std::ostringstream ss;
                ss << "Unknown operator \"" << opOrLabel << "\"";
                error(ss.str());
            }

            if (opType->second == OP_TYPE_I) {
                // Immediate.
                int rd = readRegister("destination");
                if (!foundChar(',')) {
                    error("Expected comma");
                }
                int rs = readRegister("source");
                if (!foundChar(',')) {
                    error("Expected comma");
                }
                uint32_t imm = readImmediate(12);
                emitI(0, rd, rs, imm);
            }
        }

        // See if there's a comment.
        if (*s == ';') {
            // Skip to end of line.
            while (*s != '\n' && *s != '\0') {
                s++;
            }
        }

        // Make sure the whole line was parsed properly.
        if (*s == '\n') {
            s++;
            lineStart = s;
            lineNumber++;
        } else if (*s != '\0') {
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

    // Return the next token, or an empty string if there isn't one.
    // A token is any sequence of alpha-numeric characters, or underscore.
    // Always skips subsequent whitespace.
    std::string readToken() {
        std::string token;

        // Keep track of where we started, for error reporting.
        previousToken = s;

        while (isalnum(*s) || *s == '_') {
            token += *s++;
        }

        skipWhitespace();

        return token;
    }

    // Read an integer immediate value. The immediate must fit in the specified
    // number of bits. Skips subsequent whitespace.
    uint32_t readImmediate(int bits) {
        uint32_t imm = 0;
        const char *start = s;

        while (isdigit(*s)) {
            imm = imm*10 + (*s - '0');
            s++;
        }

        if (s == start) {
            error("Expected immediate");
        }

        // Make sure we fit in 12 bits.
        if (imm >= (1 << bits)) {
            // Back up over immediate.
            s = start;
            std::ostringstream ss;
            ss << "Immediate " << imm << " does not fit in "
                << bits << (bits == 1 ? " bit" : " bits");
            error(ss.str());
        }

        skipWhitespace();

        return imm;
    }

    // Error function.
    void error(const std::string &message) {
        std::cerr << inPathname << ":" << lineNumber << ":"
            << (s - lineStart + 1) << ": " << message << "\n";
        std::cerr << currentLine() << "\n";
        std::cerr << std::string(s - lineStart, ' ') << "^\n";
        exit(EXIT_FAILURE);
    }

    // Return the current line (the one that "lineStart" points to), not including
    // the newline.
    std::string currentLine() {
        auto endOfLine = strchr(lineStart, '\n');

        return endOfLine == nullptr ? lineStart : std::string(lineStart, endOfLine - lineStart);
    }

    // Read a register name and return its number. Emits an error
    // if the token is missing or is not a register name.
    int readRegister(const std::string &role) {
        std::string regName = readToken();
        if (regName.empty()) {
            std::ostringstream ss;
            ss << "Expected " << role << " register";
            error(ss.str());
        }

        auto reg = registers.find(regName);
        if (reg == registers.end()) {
            s = previousToken;
            std::ostringstream ss;
            ss << "\"" << regName << "\" is not a register name";
            error(ss.str());
        }

        return reg->second;
    }

    void emitI(uint32_t opcode, int rd, int rs, uint32_t imm) {
        emit(opcode | (rd << 7) | (rs << 15) | (imm << 20));
    }

    void emit(uint32_t instruction) {
        if (verbose) {
            std::ios oldState(nullptr);
            oldState.copyfmt(std::cout);
            std::cout
                << std::hex << std::setw(4) << std::setfill('0') << (bin.size()*4)
                << " "
                << std::hex << std::setw(8) << std::setfill('0') << instruction
                << " " << currentLine() << "\n";
            std::cout.copyfmt(oldState);
        }

        bin.push_back(instruction);
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

    Assembler assembler(inPathname, outPathname);
    assembler.setVerbose(verbose);
    assembler.assemble();
}

