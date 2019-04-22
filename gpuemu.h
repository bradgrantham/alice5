#include <algorithm>
#include <cstdint>

struct GPUCore
{
    // Loosely modeled on RISC-V RVI, RVA, RVM, RVF

    uint32_t x[32]; // but 0 not accessed because it's readonly 0
    uint32_t pc;
    // XXX CSRs
    // XXX FCSRs

    GPUCore()
    {
        std::fill(x, x + 32, 0);
        pc = 0;
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
    void runUntilBREAK(T& memory, bool dumpTrace)
    {
        while(step(memory, dumpTrace) != BREAK);
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

constexpr uint32_t makeOpcode(uint32_t bits6_2, uint32_t bits1_0)
{
    return (bits6_2 << 2) | bits1_0;
}

template <class T>
GPUCore::Status GPUCore::step(T& memory)
{
    uint32_t insn = memory.read32(pc);
    pc += 4;

    Status status = RUNNING;

    uint32_t rd = getBits(insn, 11, 7);
    uint32_t funct3 = getBits(insn, 14, 12);
    uint32_t funct7 = getBits(insn, 31, 25);
    uint32_t rs1 = getBits(insn, 19, 15);
    uint32_t rs2 = getBits(insn, 24, 20);
    uint32_t immI = extendSign(getBits(insn, 31, 20), 12);
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

    switch(insn & 0x7F) {
        case makeOpcode(0x08, 3): { // sw
            memory.write32(x[rs1] + immS, x[rs2]);
            break;
        }

        case makeOpcode(0x0D, 3): { // lui
            if(rd > 0) {
                x[rd] = immU;
            }
            break;
        }

        case makeOpcode(0x04, 3): { // addi
            if(rd > 0) {
                x[rd] = x[rs1] + immI;
            }
            break;
        }

        default: {
            std::cerr << "unimplemented instruction " << insn;
            std::cerr << " with opcode 0x" << std::hex << (insn & 0x7F) << std::dec << '\n';
        }
    }
    return status;
}

