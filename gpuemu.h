#include <algorithm>
#include <cstdint>
#include <cmath>
#include <map>
#include <string>

typedef std::map<std::string, uint32_t> SymbolTable;

struct GPUCore
{
    // Loosely modeled on RISC-V RVI, RVA, RVM, RVF

    uint32_t x[32]; // but 0 not accessed because it's readonly 0
    uint32_t pc;
    float f[32]; // but 0 not accessed because it's readonly 0

    // XXX CSRs
    // XXX FCSRs

    enum SubstituteFunction {
        SUBST_SIN,
        SUBST_ATAN,
        SUBST_REFLECT,
        SUBST_POW,
        SUBST_NORMALIZE1,
        SUBST_NORMALIZE2,
        SUBST_NORMALIZE3,
        SUBST_NORMALIZE4,
        SUBST_COS,
        SUBST_LOG2,
        SUBST_EXP,
        SUBST_INVERSESQRT,
        SUBST_ASIN,
        SUBST_LENGTH1,
        SUBST_LENGTH2,
        SUBST_LENGTH3,
        SUBST_LENGTH4,
        SUBST_CROSS,
        SUBST_LOG,
        SUBST_FACEFORWARD,
        SUBST_ACOS,
        SUBST_RADIANS,
        SUBST_DEGREES,
        SUBST_EXP2,
        SUBST_TAN,
        SUBST_ATAN2,
        SUBST_REFRACT,
        SUBST_DISTANCE,
        SUBST_FRACT,
    };
    std::map<uint32_t, SubstituteFunction> substFunctions;

    GPUCore(const SymbolTable& librarySymbols)
    {
        std::fill(x, x + 32, 0);
        std::fill(f, f + 32, 0.0f);
        pc = 0;

        std::vector<std::pair<std::string, SubstituteFunction> > substitutions = {
            { ".sin", SUBST_SIN },
            { ".atan", SUBST_ATAN },
            { ".reflect", SUBST_REFLECT },
            { ".pow", SUBST_POW },
            { ".normalize1", SUBST_NORMALIZE1 },
            { ".normalize2", SUBST_NORMALIZE2 },
            { ".normalize3", SUBST_NORMALIZE3 },
            { ".normalize4", SUBST_NORMALIZE4 },
            { ".cos", SUBST_COS },
            { ".log2", SUBST_LOG2 },
            { ".exp", SUBST_EXP },
            { ".inversesqrt", SUBST_INVERSESQRT },
            { ".asin", SUBST_ASIN },
            { ".length1", SUBST_LENGTH1 },
            { ".length2", SUBST_LENGTH2 },
            { ".length3", SUBST_LENGTH3 },
            { ".length4", SUBST_LENGTH4 },
            { ".cross", SUBST_CROSS },
            { ".log", SUBST_LOG },
            { ".faceforward", SUBST_FACEFORWARD },
            { ".acos", SUBST_ACOS },
            { ".radians", SUBST_RADIANS },
            { ".degrees", SUBST_DEGREES },
            { ".exp2", SUBST_EXP2 },
            { ".tan", SUBST_TAN },
            { ".atan2", SUBST_ATAN2 },
            { ".refract", SUBST_REFRACT },
            { ".distance", SUBST_DISTANCE },
            { ".fract", SUBST_FRACT },
        };
        for(auto& [name, subst]: substitutions) {
            if(librarySymbols.find(name) != librarySymbols.end()) {
                substFunctions[librarySymbols.at(name)] = subst;
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
    uint32_t insn = memory.read32(pc);

    Status status = RUNNING;

    uint32_t fmt = getBits(insn, 26, 25);
    uint32_t rd = getBits(insn, 11, 7);
    uint32_t funct3 = getBits(insn, 14, 12);
    // uint32_t funct7 = getBits(insn, 31, 25);
    uint32_t ffunct = getBits(insn, 31, 27);
    uint32_t rs1 = getBits(insn, 19, 15);
    uint32_t rs2 = getBits(insn, 24, 20);
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

    std::function<float(void)> popf = [&]() { float v = memory.readf(x[2]); x[2] += 4; return v; };
    std::function<void(float)> pushf = [&](float f) { x[2] -= 4; memory.writef(x[2], f); };

    std::function<void(void)> unimpl = [&]() {
        std::cerr << "unimplemented instruction " << std::hex << std::setfill('0') << std::setw(8) << insn;
        std::cerr << " with 14..12=" << std::dec << std::setfill('0') << std::setw(1) << ((insn & 0x7000) >> 12);
        std::cerr << " and 6..2=0x" << std::hex << std::setfill('0') << std::setw(2) << ((insn & 0x7F) >> 2) << std::dec << '\n';
        status = UNIMPLEMENTED_OPCODE;
    };

    std::function<void(void)> unimpl_subst = [&]() {
        std::cerr << "unimplemented substitution " << substFunctions[pc] << "\n";
        status = UNIMPLEMENTED_OPCODE;
    };

    switch(insn & 0x707F) {
        // flw       rd rs1 imm12 14..12=2 6..2=0x01 1..0=3
        case makeOpcode(2, 0x01, 3): {
            if(dump) std::cout << "flw\n";
            f[rd] = memory.readf(x[rs1] + immI);
            pc += 4;
            break;
        }

        // fsw       imm12hi rs1 rs2 imm12lo 14..12=2 6..2=0x09 1..0=3
        case makeOpcode(2, 0x09, 3): {
            if(dump) std::cout << "fsw\n";
            memory.writef(x[rs1] + immS, f[rs2]);
            pc += 4;
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
        // fcvt.s.w  rd rs1 24..20=0 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3
        // fcvt.s.wu rd rs1 24..20=1 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3

        // XXX ignoring the rounding-mode bits
        // XXX haven't implemented anything but "S" (single) size
        CASE_MAKE_OPCODE_ALL_FUNCT3(0x14, 3)
        {
            if(dump) std::cout << "fadd etc\n";
            if(fmt != 0x0) {
                unimpl();
            }
            switch(ffunct) {
                case 0x00: /* fadd */ f[rd] = f[rs1] + f[rs2]; break;
                case 0x01: /* fsub */ f[rd] = f[rs1] - f[rs2]; break;
                case 0x02: /* fmul */ f[rd] = f[rs1] * f[rs2]; break;
                case 0x03: /* fdiv */ f[rd] = f[rs1] / f[rs2]; break;
                case 0x05: /* fmin or fmax */ f[rd] = (funct3 == 0) ? fminf(f[rs1], f[rs2]) : fmaxf(f[rs1], f[rs2]); break;
                case 0x0B: /* fsqrt */ f[rd] = sqrtf(f[rs1]); break;
                case 0x14:  {
                    if(rd > 0) {
                        if(funct3 == 0x0) {
                            x[rd] = (f[rs1] <= f[rs2]) ? 1 : 0;
                        } else if(funct3 == 0x1) {
                            x[rd] = (f[rs1] < f[rs2]) ? 1 : 0;
                        } else {
                            x[rd] = (f[rs1] == f[rs2]) ? 1 : 0;
                        }
                    }
                    break;
                }
                case 0x18: {
                // fcvt.w.s  rd rs1 24..20=0 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3
                // fcvt.wu.s rd rs1 24..20=1 31..27=0x18 rm       26..25=0 6..2=0x14 1..0=3
                    if(rd > 0) {
                        // ignoring setting valid flags
                        if(rs2 == 0) {
                            x[rd] = std::clamp(f[rs1] + .5, -2147483648.0, 2147483647.0);
                        } else if(rs2 == 1) {
                            x[rd] = std::clamp(f[rs1] + .5, 0.0, 4294967295.0);
                        }
                    }
                    break;
                }
                // fsgnj.s   rd rs1 rs2      31..27=0x04 14..12=0 26..25=0 6..2=0x14 1..0=3
                // fsgnjn.s  rd rs1 rs2      31..27=0x04 14..12=1 26..25=0 6..2=0x14 1..0=3
                // fsgnjx.s  rd rs1 rs2      31..27=0x04 14..12=2 26..25=0 6..2=0x14 1..0=3
                case 0x04: {
                    uint32_t& fs1 = *reinterpret_cast<uint32_t*>(&f[rs1]);
                    uint32_t& fs2 = *reinterpret_cast<uint32_t*>(&f[rs2]);
                    uint32_t& fd = *reinterpret_cast<uint32_t*>(&f[rd]);
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
            pc += 4;
            break;
        }

        // fcvt.s.w  rd rs1 24..20=0 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3
        // fcvt.s.wu rd rs1 24..20=1 31..27=0x1A rm       26..25=0 6..2=0x14 1..0=3
        // fmv.x.s
        // fmv.s.x
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
                case 0: memory.write8(x[rs1] + immS, x[rs2] & 0xFF); break;
                case 1: memory.write16(x[rs1] + immS, x[rs2] & 0xFFFF); break;
                case 2: memory.write32(x[rs1] + immS, x[rs2]); break;
            }
            pc += 4;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x00, 3)
        { // lb, lh, lw, lbu, lhw
            if(dump) std::cout << "load\n";
            if(rd != 0) {
                switch(funct3) {
                    case 0: x[rd] = extendSign(memory.read8(x[rs1] + immI), 8); break;
                    case 1: x[rd] = extendSign(memory.read16(x[rs1] + immI), 16); break;
                    case 2: x[rd] = memory.read32(x[rs1] + immI); break;
                    case 4: x[rd] = memory.read8(x[rs1] + immI); break;
                    case 5: x[rd] = memory.read16(x[rs1] + immI); break;
                    default: unimpl(); break;
                }
            }
            pc += 4;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x0D, 3)
        { // lui
            if(dump) std::cout << "lui\n";
            if(rd > 0) {
                x[rd] = immU;
            }
            pc += 4;
            break;
        }
 
        CASE_MAKE_OPCODE_ALL_FUNCT3(0x18, 3)
        { // beq, bne, blt, bge, bltu, bgeu etc
            if(dump) std::cout << "bge\n";
            switch(funct3) {
                case 0: pc += (x[rs1] == x[rs2]) ? immSB : 4; break;
                case 1: pc += (x[rs1] != x[rs2]) ? immSB : 4; break;
                case 4: pc += (static_cast<int32_t>(x[rs1]) < static_cast<int32_t>(x[rs2])) ? immSB : 4; break;
                case 5: pc += (static_cast<int32_t>(x[rs1]) >= static_cast<int32_t>(x[rs2])) ? immSB : 4; break;
                case 6: pc += (x[rs1] < x[rs2]) ? immSB : 4; break;
                case 7: pc += (x[rs1] >= x[rs2]) ? immSB : 4; break;
                default: unimpl(); break;
            }
            break;
        }

        case makeOpcode(0, 0x0C, 3): { // add
            if(dump) std::cout << "add\n";
            if(rd > 0) {
                x[rd] = x[rs1] + x[rs2];
            }
            pc += 4;
            break;
        }

        case makeOpcode(0, 0x19, 3): { // jalr
            if(dump) std::cout << "jalr\n";
            uint32_t ra = (x[rs1] + immI) & ~0x00000001; // spec says set least significant bit to zero
            if(rd > 0) {
                x[rd] = pc + 4;
            }
            pc = ra;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x1b, 3)
        { // jal
            if(dump) std::cout << "jal\n";
            if(rd > 0) {
                x[rd] = pc + 4;
            }
            pc += immUJ;
            break;
        }

        CASE_MAKE_OPCODE_ALL_FUNCT3(0x04, 3)
        { // addi
            if(dump) std::cout << "addi\n";
            if(rd > 0) {
                switch(funct3) {
                    case 0: x[rd] = x[rs1] + immI; break;
                    case 1: x[rd] = x[rs1] << shamt; break;
                    case 4: x[rd] = x[rs1] ^ immI; break;
                    case 6: x[rd] = x[rs1] | immI; break;
                    case 7: x[rd] = x[rs1] & immI; break;
                    default: unimpl(); break;
                }
            } else {
                if((funct3 == 0) && (rs1 == 0) && (immI == 0)) {
                    if(substFunctions.find(pc) != substFunctions.end()) {
                        switch(substFunctions[pc]) {
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
                            case SUBST_FRACT: {
                                float f = popf();
                                pushf(f - floorf(f));
                                break;
                            }
                            case SUBST_REFRACT: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_DISTANCE: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_REFLECT: {
                                unimpl_subst();
                                break;
                            }
                            case SUBST_FACEFORWARD: {
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
                        pc += 4;
                        return RUNNING;
                    }
                }
            }

            pc += 4;
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
    uint32_t magic = 0x30354c41;        // 'AL50', version 0 of Alice 5 header
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
