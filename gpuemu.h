#include <functional>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <cfenv>

typedef std::map<std::string, uint32_t> SymbolTable;

const int RM_RNE = 0b000;
const int RM_RTZ = 0b001;
const int RM_RDN = 0b010;
const int RM_RUP = 0b011;
const int RM_TONEAREST_MAX = 0b100;

struct GPUCore
{
    // Loosely modeled on RISC-V RVI, RVA, RVM, RVF

    struct Registers {
        uint32_t x[32]; // but 0 not accessed because it's readonly 0
        uint32_t pc;
        float f[32];
    } regs;

    // XXX CSRs
    // XXX FCSRs

    enum SubstituteFunction {
        SUBST_SIN,
        SUBST_ATAN,
        SUBST_REFLECT1,
        SUBST_REFLECT2,
        SUBST_REFLECT3,
        SUBST_REFLECT4,
        SUBST_POW,
        SUBST_MAX,
        SUBST_MIN,
        SUBST_NORMALIZE1,
        SUBST_NORMALIZE2,
        SUBST_NORMALIZE3,
        SUBST_NORMALIZE4,
        SUBST_COS,
        SUBST_LOG2,
        SUBST_EXP,
        SUBST_MOD,
        SUBST_INVERSESQRT,
        SUBST_ASIN,
        SUBST_LENGTH1,
        SUBST_LENGTH2,
        SUBST_LENGTH3,
        SUBST_LENGTH4,
        SUBST_CROSS,
        SUBST_LOG,
        SUBST_FACEFORWARD1,
        SUBST_FACEFORWARD2,
        SUBST_FACEFORWARD3,
        SUBST_FACEFORWARD4,
        SUBST_ACOS,
        SUBST_RADIANS,
        SUBST_DEGREES,
        SUBST_EXP2,
        SUBST_TAN,
        SUBST_ATAN2,
        SUBST_REFRACT1,
        SUBST_REFRACT2,
        SUBST_REFRACT3,
        SUBST_REFRACT4,
        SUBST_DISTANCE1,
        SUBST_DISTANCE2,
        SUBST_DISTANCE3,
        SUBST_DISTANCE4,
        SUBST_FRACT,
        SUBST_FLOOR,
        SUBST_STEP,
        SUBST_DOT1,
        SUBST_DOT2,
        SUBST_DOT3,
        SUBST_DOT4,
        SUBST_ANY1,
        SUBST_ANY2,
        SUBST_ANY3,
        SUBST_ANY4,
        SUBST_ALL1,
        SUBST_ALL2,
        SUBST_ALL3,
        SUBST_ALL4,
    };
    std::map<uint32_t, SubstituteFunction> substFunctions;
    std::map<uint32_t, std::string> substFunctionNames;

    std::set<std::string> substitutedFunctions;

    GPUCore(const SymbolTable& librarySymbols)
    {
        std::fill(regs.x, regs.x + 32, 0);
        std::fill(regs.f, regs.f + 32, 0.0f);
        regs.pc = 0;

        std::vector<std::pair<std::string, SubstituteFunction> > substitutions = {
            { ".sin", SUBST_SIN },
            { ".atan", SUBST_ATAN },
            { ".reflect1", SUBST_REFLECT1 },
            { ".reflect2", SUBST_REFLECT2 },
            { ".reflect3", SUBST_REFLECT3 },
            { ".reflect4", SUBST_REFLECT4 },
            { ".pow", SUBST_POW },
            { ".max", SUBST_MAX },
            { ".min", SUBST_MIN },
            { ".normalize1", SUBST_NORMALIZE1 },
            { ".normalize2", SUBST_NORMALIZE2 },
            { ".normalize3", SUBST_NORMALIZE3 },
            { ".normalize4", SUBST_NORMALIZE4 },
            { ".cos", SUBST_COS },
            { ".log2", SUBST_LOG2 },
            { ".exp", SUBST_EXP },
            { ".mod", SUBST_MOD },
            { ".inversesqrt", SUBST_INVERSESQRT },
            { ".asin", SUBST_ASIN },
            { ".length1", SUBST_LENGTH1 },
            { ".length2", SUBST_LENGTH2 },
            { ".length3", SUBST_LENGTH3 },
            { ".length4", SUBST_LENGTH4 },
            { ".cross", SUBST_CROSS },
            { ".log", SUBST_LOG },
            { ".faceforward1", SUBST_FACEFORWARD1 },
            { ".faceforward2", SUBST_FACEFORWARD2 },
            { ".faceforward3", SUBST_FACEFORWARD3 },
            { ".faceforward4", SUBST_FACEFORWARD4 },
            { ".acos", SUBST_ACOS },
            { ".radians", SUBST_RADIANS },
            { ".degrees", SUBST_DEGREES },
            { ".exp2", SUBST_EXP2 },
            { ".tan", SUBST_TAN },
            { ".atan2", SUBST_ATAN2 },
            { ".refract1", SUBST_REFRACT1 },
            { ".refract2", SUBST_REFRACT2 },
            { ".refract3", SUBST_REFRACT3 },
            { ".refract4", SUBST_REFRACT4 },
            { ".distance1", SUBST_DISTANCE1 },
            { ".distance2", SUBST_DISTANCE2 },
            { ".distance3", SUBST_DISTANCE3 },
            { ".distance4", SUBST_DISTANCE4 },
            { ".fract", SUBST_FRACT },
            { ".floor", SUBST_FLOOR },
            { ".step", SUBST_STEP },
            { ".dot1", SUBST_DOT1 },
            { ".dot2", SUBST_DOT2 },
            { ".dot3", SUBST_DOT3 },
            { ".dot4", SUBST_DOT4 },
            { ".any1", SUBST_ANY1 },
            { ".any2", SUBST_ANY2 },
            { ".any3", SUBST_ANY3 },
            { ".any4", SUBST_ANY4 },
            { ".all1", SUBST_ALL1 },
            { ".all2", SUBST_ALL2 },
            { ".all3", SUBST_ALL3 },
            { ".all4", SUBST_ALL4 },
        };
        for(auto& [name, subst]: substitutions) {
            if(librarySymbols.find(name) != librarySymbols.end()) {
                substFunctions[librarySymbols.at(name)] = subst;
                substFunctionNames[librarySymbols.at(name)] = name;
            }
        }
    }

    enum Status {RUNNING, BREAK, UNIMPLEMENTED_OPCODE};

    // Expected memory functions
    // uint8_t read8(uint32_t addr);
    // uint16_t read16(uint32_t addr);
    // uint32_t read32(uint32_t addr);
    // void write8(uint32_t addr, uint8_t v);
    // void write16(uint32_t addr, uint16_t v);
    // void write32(uint32_t addr, uint32_t v);
    
    template <class T>
    Status step(T& memory);

    template <class T>
    Status stepUntilException(T& memory)
    {
        Status status;
        while((status = step(memory)) == RUNNING);
        return status;
    }
};

uint32_t extendSign(uint32_t v, int width)
{
    uint32_t sign = v & (1 << (width - 1));
    uint32_t extended = 0 - sign;
    return v | extended;
}

uint32_t getBits(uint32_t v, int hi, int lo)
{
    return (v >> lo) & ((1u << (hi + 1 - lo)) - 1);
}

constexpr uint32_t makeOpcode(uint32_t bits14_12, uint32_t bits6_2, uint32_t bits1_0)
{
    return (bits14_12 << 12) | (bits6_2 << 2) | bits1_0;
}

const bool dump = false;

#define CASE_MAKE_OPCODE_ALL_FUNCT3(a, b) \
        case makeOpcode(0, (a), (b)): \
        case makeOpcode(1, (a), (b)): \
        case makeOpcode(2, (a), (b)): \
        case makeOpcode(3, (a), (b)): \
        case makeOpcode(4, (a), (b)): \
        case makeOpcode(5, (a), (b)): \
        case makeOpcode(6, (a), (b)): \
        case makeOpcode(7, (a), (b)):

template <class T>
GPUCore::Status GPUCore::step(T& memory)
{
    uint32_t insn = memory.read32(regs.pc);

    Status status = RUNNING;

    uint32_t fmt = getBits(insn, 26, 25);
    uint32_t rd = getBits(insn, 11, 7);
    uint32_t funct3 = getBits(insn, 14, 12);
    uint32_t funct7 = getBits(insn, 31, 25);
    uint32_t ffunct = getBits(insn, 31, 27);
    uint32_t rs1 = getBits(insn, 19, 15);
    uint32_t rs2 = getBits(insn, 24, 20);
    uint32_t rs3 = getBits(insn, 31, 27);
    uint32_t rm = getBits(insn, 14, 12);
    uint32_t immI = extendSign(getBits(insn, 31, 20), 12);
    uint32_t shamt = getBits(insn, 34, 20);
    uint32_t immS = extendSign(
        (getBits(insn, 31, 25) << 5) |
        getBits(insn, 11, 7),
        12);
    uint32_t immSB = extendSign((getBits(insn, 31, 31) << 12) |
        (getBits(insn, 7, 7) << 11) | 
        (getBits(insn, 30, 25) << 5) | 
        (getBits(insn, 11, 8) << 1),
        12);
    uint32_t immU = getBits(insn, 31, 12) << 12;
    uint32_t immUJ = extendSign((getBits(insn, 31, 31) << 20) |
        (getBits(insn, 19, 12) << 12) | 
        (getBits(insn, 20, 20) << 11) | 
        (getBits(insn, 30, 21) << 1),
        21);

    std::function<float(void)> popf = [&]() { float v = memory.readf(regs.x[2]); regs.x[2] += 4; return v; };
    std::function<void(float)> pushf = [&](float f) { regs.x[2] -= 4; memory.writef(regs.x[2], f); };
    std::function<uint32_t(void)> pop32 = [&]() { uint32_t v = memory.read32(regs.x[2]); regs.x[2] += 4; return v; };
    std::function<void(uint32_t)> push32 = [&](uint32_t f) { regs.x[2] -= 4; memory.write32(regs.x[2], f); };

    std::function<void(void)> unimpl = [&]() {
        std::cerr << "unimplemented instruction " << std::hex << std::setfill('0') << std::setw(8) << insn;
        std::cerr << " with 14..12=" << std::dec << std::setfill('0') << std::setw(1) << ((insn & 0x7000) >> 12);
        std::cerr << " and 6..2=0x" << std::hex << std::setfill('0') << std::setw(2) << ((insn & 0x7F) >> 2) << std::dec << '\n';
        status = UNIMPLEMENTED_OPCODE;
    };

    std::function<void(void)> unimpl_subst = [&]() {
        std::cerr << "unimplemented substitution " << substFunctions[regs.pc] << "\n";
        status = UNIMPLEMENTED_OPCODE;
    };

    int previous_rounding_mode;
    std::function<void(void)> setrm = [&]() {
        previous_rounding_mode = fegetround();
        switch(rm) {
            case RM_RDN: fesetround(FE_DOWNWARD); break;
            case RM_RNE: fesetround(FE_TONEAREST); break;
            case RM_RTZ: fesetround(FE_TOWARDZERO); break;
            case RM_RUP: fesetround(FE_UPWARD); break;
            default:
                std::cerr << "unimplemented rounding mode " << rm << " (ignored)\n";
                break;
        }
    };
    std::function<void(void)> restorerm = [&]() {
        fesetround(previous_rounding_mode);
    };

    switch(insn & 0x707F) {
        // flw       rd rs1 imm12 14..12=2 6..2=0x01 1..0=3
        case makeOpcode(2, 0x01, 3): {
            if(dump) std::cout << "flw\n";
            regs.f[rd] = memory.readf(regs.x[rs1] + immI);
            regs.pc += 4;
            break;
        }

        // fsw       imm12hi rs1 rs2 imm12lo 14..12=2 6..2=0x09 1..0=3
        case makeOpcode(2, 0x09, 3): {
            if(dump) std::cout << "fsw\n";
            memory.writef(regs.x[rs1] + immS, regs.f[rs2]);
            regs.pc += 4;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x10, 3)
        {
            if(dump) std::cout << "fmadd\n";
            if(fmt == 0x0) { // size = .s
                setrm();
                regs.f[rd] = regs.f[rs1] * regs.f[rs2] + regs.f[rs3];
                restorerm();
            } else {
                unimpl();
            }
            regs.pc += 4;
            break;
        }

        // fadd.s    rd rs1 rs2      31..27=0x00 rm       26..25=0 6..2=0x14 1..0=3
        // fmul.s    rd rs1 rs2      31..27=0x02 rm       26..25=0 6..2=0x14 1..0=3
        // fsub.s    rd rs1 rs2      31..27=0x01 rm       26..25=0 6..2=0x14 1..0=3
        // fdiv.s    rd rs1 rs2      31..27=0x03 rm       26..25=0 6..2=0x14 1..0=3
        // fmin.s    rd rs1 rs2      31..27=0x05 14..12=0 26..25=0 6..2=0x14 1..0=3
        // fmax.s    rd rs1 rs2      31..27=0x05 14..12=1 26..25=0 6..2=0x14 1..0=3
        // fsqrt.s   rd rs1 24..20=0 31..27=0x0B rm       26..25=0 6..2=0x14 1..0=3
        // fle.s     rd rs1 rs2      31..27=0x14 14..12=0 26..25=0 6..2=0x14 1..0=3
        // flt.s     rd rs1 rs2      31..27=0x14 14..12=1 26..25=0 6..2=0x14 1..0=3
        // feq.s     rd rs1 rs2      31..27=0x14 14..12=2 26..25=0 6..2=0x14 1..0=3

        // fcvt.w.s  rd rs1 24..20=0 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3
        // fcvt.wu.s rd rs1 24..20=1 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3

        // XXX haven't implemented anything but "S" (single) size
        CASE_MAKE_OPCODE_ALL_FUNCT3(0x14, 3)
        {
            if(dump) std::cout << "fadd etc\n";
            if(fmt == 0x0) { // size = .s
                switch(ffunct) {
                    case 0x00: /* fadd */ setrm(); regs.f[rd] = regs.f[rs1] + regs.f[rs2]; restorerm(); break;
                    case 0x01: /* fsub */ setrm(); regs.f[rd] = regs.f[rs1] - regs.f[rs2]; restorerm(); break;
                    case 0x02: /* fmul */ setrm(); regs.f[rd] = regs.f[rs1] * regs.f[rs2]; restorerm(); break;
                    case 0x03: /* fdiv */ setrm(); regs.f[rd] = regs.f[rs1] / regs.f[rs2]; restorerm(); break;
                    case 0x05: /* fmin or fmax */ regs.f[rd] = (funct3 == 0) ? fminf(regs.f[rs1], regs.f[rs2]) : fmaxf(regs.f[rs1], regs.f[rs2]); break;
                    case 0x0B: /* fsqrt */ setrm(); regs.f[rd] = sqrtf(regs.f[rs1]); restorerm(); break;
                    case 0x14:  { // fp comparison
                        if(rd > 0) {
                            if(funct3 == 0x0) { // fle 
                                regs.x[rd] = (regs.f[rs1] <= regs.f[rs2]) ? 1 : 0;
                            } else if(funct3 == 0x1) { // flt
                                regs.x[rd] = (regs.f[rs1] < regs.f[rs2]) ? 1 : 0;
                            } else { // feq
                                regs.x[rd] = (regs.f[rs1] == regs.f[rs2]) ? 1 : 0;
                            }
                        }
                        break;
                    }
                    case 0x18: {
                        if(rd > 0) {
                            // ignoring setting valid flags
                            if(rs2 == 0) {      // fcvt.w.s
                                setrm();
                                regs.x[rd] = std::rint(std::clamp(regs.f[rs1], -2147483648.0f, 2147483647.0f));
                                restorerm();
                            } else if(rs2 == 1) {       // fcvt.wu.s
                                setrm();
                                regs.x[rd] = std::rint(std::clamp(regs.f[rs1], 0.0f, 4294967295.0f));
                                restorerm();
                            }
                        }
                        break;
                    }

                    // fmv.w.x   rd rs1 24..20=0 31..27=0x1E 14..12=0 26..25=0 6..2=0x14 1..0=3
                    case 0x1E: {
                        if(funct3 == 0) {
                            if(fmt == 0) {
                                regs.f[rd] = *reinterpret_cast<float*>(&regs.x[rs1]);
                            } else {
                                unimpl();
                            }
                        } else {
                            unimpl();
                        }
                        break;
                    }

                    case 0x1A: {
                        if(rs2 == 0) {          // fcvt.s.w
                            if(fmt == 0) {
                                setrm();
                                regs.f[rd] = *reinterpret_cast<int32_t*>(&regs.x[rs1]);
                                restorerm();
                            } else {
                                printf("fmt = %d\n", fmt);
                                unimpl();
                            }
                        } else if(rs2 == 1) {   // fcvt.s.wu
                            if(fmt == 0) {
                                setrm();
                                regs.f[rd] = regs.x[rs1];
                                restorerm();
                            } else {
                                printf("fmt = %d\n", fmt);
                                unimpl();
                            }
                        } else {
                            printf("rs2 = %d\n", rs2);
                            unimpl();
                        }
                        break;
                    }

                    // fsgnj.s   rd rs1 rs2      31..27=0x04 14..12=0 26..25=0 6..2=0x14 1..0=3
                    // fsgnjn.s  rd rs1 rs2      31..27=0x04 14..12=1 26..25=0 6..2=0x14 1..0=3
                    // fsgnjx.s  rd rs1 rs2      31..27=0x04 14..12=2 26..25=0 6..2=0x14 1..0=3
                    case 0x04: {
                        uint32_t& fs1 = *reinterpret_cast<uint32_t*>(&regs.f[rs1]);
                        uint32_t& fs2 = *reinterpret_cast<uint32_t*>(&regs.f[rs2]);
                        uint32_t& fd = *reinterpret_cast<uint32_t*>(&regs.f[rd]);
                        if(funct3 == 0) {
                            fd = (fs1 & 0x7FFFFFFF) | (fs2 & 0x80000000);
                        } else if(funct3 == 1) {
                            fd = (fs1 & 0x7FFFFFFF) | ((fs2 & 0x80000000) ^ 0x80000000);
                        } else if(funct3 == 2) {
                            fd = (fs1 & 0x7FFFFFFF) | ((fs2 & 0x80000000) ^ (fs1 & 0x80000000));
                        } else {
                            unimpl();
                        }
                        break;
                    }
                    default: unimpl(); break;
                }
            } else {
                unimpl();
            }
            regs.pc += 4;
            break;
        }

        // fmv.x.w   rd rs1 24..20=0 31..27=0x1C 14..12=0 26..25=0 6..2=0x14 1..0=3
        // fmv.x.s
        // fclass.s

        case makeOpcode(0, 0x1C, 3): { // ebreak
            if(dump) std::cout << "ebreak\n";
            if(insn == 0x00100073) {
                status = BREAK;
            } else {
                unimpl();
            }
            break;
        }

        case makeOpcode(0, 0x08, 3):
        case makeOpcode(1, 0x08, 3):
        case makeOpcode(2, 0x08, 3):
        { // sb, sh, sw
            if(dump) std::cout << "sw\n";
            switch(funct3) {
                case 0: memory.write8(regs.x[rs1] + immS, regs.x[rs2] & 0xFF); break;
                case 1: memory.write16(regs.x[rs1] + immS, regs.x[rs2] & 0xFFFF); break;
                case 2: memory.write32(regs.x[rs1] + immS, regs.x[rs2]); break;
            }
            regs.pc += 4;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x00, 3)
        { // lb, lh, lw, lbu, lhw
            if(dump) std::cout << "load\n";
            if(rd != 0) {
                switch(funct3) {
                    case 0: regs.x[rd] = extendSign(memory.read8(regs.x[rs1] + immI), 8); break;
                    case 1: regs.x[rd] = extendSign(memory.read16(regs.x[rs1] + immI), 16); break;
                    case 2: regs.x[rd] = memory.read32(regs.x[rs1] + immI); break;
                    case 4: regs.x[rd] = memory.read8(regs.x[rs1] + immI); break;
                    case 5: regs.x[rd] = memory.read16(regs.x[rs1] + immI); break;
                    default: unimpl(); break;
                }
            }
            regs.pc += 4;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x0D, 3)
        { // lui
            if(dump) std::cout << "lui\n";
            if(rd > 0) {
                regs.x[rd] = immU;
            }
            regs.pc += 4;
            break;
        }
 
        CASE_MAKE_OPCODE_ALL_FUNCT3(0x18, 3)
        { // beq, bne, blt, bge, bltu, bgeu etc
            if(dump) std::cout << "bge\n";
            switch(funct3) {
                case 0: regs.pc += (regs.x[rs1] == regs.x[rs2]) ? immSB : 4; break;
                case 1: regs.pc += (regs.x[rs1] != regs.x[rs2]) ? immSB : 4; break;
                case 4: regs.pc += (static_cast<int32_t>(regs.x[rs1]) < static_cast<int32_t>(regs.x[rs2])) ? immSB : 4; break;
                case 5: regs.pc += (static_cast<int32_t>(regs.x[rs1]) >= static_cast<int32_t>(regs.x[rs2])) ? immSB : 4; break;
                case 6: regs.pc += (regs.x[rs1] < regs.x[rs2]) ? immSB : 4; break;
                case 7: regs.pc += (regs.x[rs1] >= regs.x[rs2]) ? immSB : 4; break;
                default: unimpl(); break;
            }
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x0C, 3)
        {
            if(dump) std::cout << "add etc\n";
            if(rd > 0) {
                switch(funct3) {
                    case 0: {
                        if(funct7 == 0) {
                            regs.x[rd] = regs.x[rs1] + regs.x[rs2]; // add
                        } else if(funct7 == 32) {
                            regs.x[rd] = regs.x[rs1] - regs.x[rs2]; // sub
                        } else {
                            unimpl();
                        }
                        break;
                    }
                    case 1: regs.x[rd] = regs.x[rs1] << (regs.x[rs2] & 0x1F); break;        // sll
                    case 5: {
                        if(funct7 == 0) {
                            regs.x[rd] = regs.x[rs1] >> (regs.x[rs2] & 0x1F); break;        // srl
                        } else if(funct7 == 32) {
                            // whether right-shifting a negative signed
                            // int extends the sign bit is implementation
                            // defined in C but I'll risk it.
                            regs.x[rd] = ((int32_t)regs.x[rs1]) >> (regs.x[rs2] & 0x1F); break;     // sra
                        } else {
                            unimpl();
                        }
                        break;
                    }
                    case 2: regs.x[rd] = ((int32_t)regs.x[rs1] < (int32_t)regs.x[rs2]); break;      // sltu
                    case 3: regs.x[rd] = (regs.x[rs1] < regs.x[rs2]); break;        // slt
                    case 4: regs.x[rd] = regs.x[rs1] ^ regs.x[rs2]; break;  // xor
                    case 6: regs.x[rd] = regs.x[rs1] | regs.x[rs2]; break;  // or
                    case 7: regs.x[rd] = regs.x[rs1] & regs.x[rs2]; break;  // and
                    default: {
                        unimpl();
                    }
                }
            }
            regs.pc += 4;
            break;
        }

        case makeOpcode(0, 0x19, 3): { // jalr
            if(dump) std::cout << "jalr\n";
            uint32_t ra = (regs.x[rs1] + immI) & ~0x00000001; // spec says set least significant bit to zero
            if(rd > 0) {
                regs.x[rd] = regs.pc + 4;
            }
            regs.pc = ra;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x1b, 3)
        { // jal
            if(dump) std::cout << "jal\n";
            if(rd > 0) {
                regs.x[rd] = regs.pc + 4;
            }
            regs.pc += immUJ;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x04, 3)
        { // addi
            if(dump) std::cout << "addi etc\n";
            if(rd > 0) {
                switch(funct3) {
                    case 0: regs.x[rd] = regs.x[rs1] + immI; break;     // addi
                    case 1: regs.x[rd] = regs.x[rs1] << shamt; break;   // slli
                    case 4: regs.x[rd] = regs.x[rs1] ^ immI; break;     // xori
                    case 6: regs.x[rd] = regs.x[rs1] | immI; break;     // ori
                    case 7: regs.x[rd] = regs.x[rs1] & immI; break;     // andi
                    default: unimpl(); break;
                }
            } else {
                if((funct3 == 0) && (rs1 == 0) && (immI == 0)) {
                    if(substFunctions.find(regs.pc) != substFunctions.end()) {
                        substitutedFunctions.insert(substFunctionNames[regs.pc]);
                        switch(substFunctions[regs.pc]) {
                            case SUBST_SIN: {
                                pushf(sinf(popf()));
                                break;
                            }
                            case SUBST_ATAN: {
                                pushf(atanf(popf()));
                                break;
                            }
                            case SUBST_POW: {
                                float x = popf();
                                float y = popf();
                                pushf(powf(x, y));
                                break;
                            }
                            case SUBST_MAX: {
                                float x = popf();
                                float y = popf();
                                pushf(x > y ? x : y);
                                break;
                            }
                            case SUBST_MIN: {
                                float x = popf();
                                float y = popf();
                                pushf(x < y ? x : y);
                                break;
                            }
                            case SUBST_COS: {
                                pushf(cosf(popf()));
                                break;
                            }
                            case SUBST_LOG2: {
                                pushf(log2f(popf()));
                                break;
                            }
                            case SUBST_EXP: {
                                pushf(expf(popf()));
                                break;
                            }
                            case SUBST_MOD: {
                                float x = popf();
                                float y = popf();
                                // fmodf() isn't right for us, it's the remainder after
                                // rounding toward zero, but we need to floor.
                                float q = floorf(x/y);
                                pushf(x - q*y);
                                break;
                            }
                            case SUBST_INVERSESQRT: {
                                pushf(1.0 / sqrtf(popf()));
                                break;
                            }
                            case SUBST_ASIN: {
                                pushf(asinf(popf()));
                                break;
                            }
                            case SUBST_LOG: {
                                pushf(logf(popf()));
                                break;
                            }
                            case SUBST_ACOS: {
                                pushf(acosf(popf()));
                                break;
                            }
                            case SUBST_RADIANS: {
                                pushf(popf() / 180.0 * M_PI);
                                break;
                            }
                            case SUBST_DEGREES: {
                                pushf(popf() * 180.0 / M_PI);
                                break;
                            }
                            case SUBST_EXP2: {
                                pushf(exp2f(popf()));
                                break;
                            }
                            case SUBST_TAN: {
                                pushf(tanf(popf()));
                                break;
                            }
                            case SUBST_ATAN2: {
                                float y = popf();
                                float x = popf();
                                pushf(atan2f(y, x));
                                break;
                            }
                            case SUBST_CROSS: {
                                float x[3], y[3];
                                x[0] = popf();
                                x[1] = popf();
                                x[2] = popf();
                                y[0] = x[1] * y[2] - y[1] * x[2];
                                y[1] = x[2] * y[0] - y[2] * x[0];
                                y[2] = x[0] * y[1] - y[0] * x[1];
                                pushf(y[2]);
                                pushf(y[1]);
                                pushf(y[0]);
                                break;
                            }
                            case SUBST_NORMALIZE1: {
                                float x = popf();
                                pushf(x < 0.0 ? -1.0f : 1.0f);
                                break;
                            }
                            case SUBST_NORMALIZE2: {
                                float x = popf();
                                float y = popf();
                                float d = 1.0f / sqrtf(x * x + y * y);
                                pushf(x * d);
                                pushf(y * d);
                                break;
                            }
                            case SUBST_NORMALIZE3: {
                                float x = popf();
                                float y = popf();
                                float z = popf();
                                float d = 1.0f / sqrtf(x * x + y * y + z * z);
                                pushf(x * d);
                                pushf(y * d);
                                pushf(z * d);
                                break;
                            }
                            case SUBST_NORMALIZE4: {
                                float x = popf();
                                float y = popf();
                                float z = popf();
                                float w = popf();
                                float d = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
                                pushf(x * d);
                                pushf(y * d);
                                pushf(z * d);
                                pushf(w * d);
                                break;
                            }
                            case SUBST_FLOOR: {
                                float f = popf();
                                pushf(floorf(f));
                                break;
                            }
                            case SUBST_DOT1: {
                                float x1 = popf();
                                float x2 = popf();
                                float v = x1 * x2;
                                pushf(v);
                                break;
                            }
                            case SUBST_DOT2: {
                                float x1 = popf();
                                float y1 = popf();
                                float x2 = popf();
                                float y2 = popf();
                                float v = x1 * x2 + y1 * y2;
                                pushf(v);
                                break;
                            }
                            case SUBST_DOT3: {
                                float x1 = popf();
                                float y1 = popf();
                                float z1 = popf();
                                float x2 = popf();
                                float y2 = popf();
                                float z2 = popf();
                                float v = x1 * x2 + y1 * y2 + z1 * z2;
                                pushf(v);
                                break;
                            }
                            case SUBST_DOT4: {
                                float x1 = popf();
                                float y1 = popf();
                                float z1 = popf();
                                float w1 = popf();
                                float x2 = popf();
                                float y2 = popf();
                                float z2 = popf();
                                float w2 = popf();
                                float v = x1 * x2 + y1 * y2 + z1 * z2 + w1 * w2;
                                pushf(v);
                                break;
                            }
                            case SUBST_ALL1: {
                                uint32_t x = pop32();
                                push32(x);
                                break;
                            }
                            case SUBST_ALL2: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                push32(x && y);
                                break;
                            }
                            case SUBST_ALL3: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                uint32_t z = pop32();
                                push32(x && y && z);
                                break;
                            }
                            case SUBST_ALL4: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                uint32_t z = pop32();
                                uint32_t w = pop32();
                                push32(x && y && z && w);
                                break;
                            }
                            case SUBST_ANY1: {
                                uint32_t x = pop32();
                                push32(x);
                                break;
                            }
                            case SUBST_ANY2: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                push32(x || y);
                                break;
                            }
                            case SUBST_ANY3: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                uint32_t z = pop32();
                                push32(x || y || z);
                                break;
                            }
                            case SUBST_ANY4: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                uint32_t z = pop32();
                                uint32_t w = pop32();
                                push32(x || y || z || w);
                                break;
                            }
                            case SUBST_STEP: {
                                float edge = popf();
                                float x = popf();
                                float y = (x < edge) ? 0.0f : 1.0f;
                                pushf(y);
                                break;
                            }
                            case SUBST_FRACT: {
                                float f = popf();
                                pushf(f - floorf(f));
                                break;
                            }
                            case SUBST_REFRACT1: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFRACT2: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFRACT3: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFRACT4: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_DISTANCE1: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_DISTANCE2: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_DISTANCE3: {
                                uint32_t x = pop32();
                                uint32_t y = pop32();
                                uint32_t z = pop32();
                                push32(sqrtf(x*x + y*y + z*z));
                                break;
                            }
                            case SUBST_DISTANCE4: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFLECT1: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFLECT2: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFLECT3: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFLECT4: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_FACEFORWARD1: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_FACEFORWARD2: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_FACEFORWARD3: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_FACEFORWARD4: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_LENGTH1: {
                                float x = popf();
                                pushf(fabsf(x));
                                break;
                            }
                            case SUBST_LENGTH2: {
                                float x = popf();
                                float y = popf();
                                float d = sqrtf(x * x + y * y);
                                pushf(d);
                                break;
                            }
                            case SUBST_LENGTH3: {
                                float x = popf();
                                float y = popf();
                                float z = popf();
                                float d = sqrtf(x * x + y * y + z * z);
                                pushf(d);
                                break;
                            }
                            case SUBST_LENGTH4: {
                                float x = popf();
                                float y = popf();
                                float z = popf();
                                float w = popf();
                                float d = sqrtf(x * x + y * y + z * z + w * w);
                                pushf(d);
                                break;
                            }
                        }
                        regs.pc += 4;
                        return RUNNING;
                    }
                }
            }

            regs.pc += 4;
            break;
        }

        default: {
            unimpl();
        }
    }
    return status;
}

struct RunHeader
{
    // Little-endian
    uint32_t magic = 0x30354c41;        // 'AL51', version 1 of Alice 5 header
    uint32_t initialPC;                 // Initial value PC is set to
    uint32_t gl_FragCoordAddress;       // address of vec4 gl_FragCoord input
    uint32_t colorAddress;              // address of vec4 color output
    uint32_t iTimeAddress;              // address of float uniform
    uint32_t iMouseAddress;             // address of ivec4 uniform
    uint32_t iResolutionAddress;        // address of vec2 iResolution uniform
    // Bytes to follow are loaded at 0
};

const uint32_t RunHeader1MagicExpected = 0x31354c41;
struct RunHeader1
{
    // Little-endian
    uint32_t magic = RunHeader1MagicExpected;        // 'AL51', version 1 of Alice 5 header
    uint32_t initialPC;                 // Initial value PC is set to
    uint32_t symbolCount;
    // symbolCount symbols follow that are of the following layout:
    //       uint32_t address
    //       uint32_t stringLength
    //       stringLength bytes for symbol name including NUL
    // Program and data bytes follow.  Bytes are loaded at 0
};
