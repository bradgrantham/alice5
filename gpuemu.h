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
    Status step(T& memory, bool dumpTrace);

    template <class T>
    void runUntilBREAK(T& memory, bool dumpTrace)
    {
        while(step(memory, dumpTrace) != BREAK);
    }
};

uint32_t getBits(uint32_t v, int hi, int lo)
{
    return (v >> lo) & ((1u << (hi - lo + 1)) - 1);
}

constexpr uint32_t makeOpcode(uint32_t bits6_2, uint32_t bits1_0)
{
    return (bits6_2 << 2) | bits1_0;
}

template <class T>
GPUCore::Status GPUCore::step(T& memory, bool dumpTrace)
{
    uint32_t insn = memory.read32(pc);

    Status status = RUNNING;

    uint32_t opcode = getBits(insn, 6, 0);
    uint32_t rd = getBits(insn, 11, 7);
    uint32_t funct3 = getBits(insn, 14, 12);
    uint32_t funct7 = getBits(insn, 31, 25);
    uint32_t rs1 = getBits(insn, 19, 15);
    uint32_t rs2 = getBits(insn, 24, 20);
    uint32_t immI = getBits(insn, 31, 20);
    uint32_t immS = (getBits(insn, 31, 25) << 5) | getBits(insn, 11, 7);
    uint32_t immSB = (getBits(insn, 31, 31) << 12) |
        (getBits(insn, 7, 7) << 11) | 
        (getBits(insn, 30, 25) << 5) | 
        (getBits(insn, 11, 8) << 1);
    uint32_t immU = getBits(insn, 31, 12) << 12;
    uint32_t immUJ = (getBits(insn, 31, 31) << 20) |
        (getBits(insn, 19, 12) << 12) | 
        (getBits(insn, 20, 20) << 11) | 
        (getBits(insn, 30, 21) << 1);

    switch(opcode) {
        case makeOpcode(0x04, 3): { // addi
            if(dumpTrace) {
                printf("%08X: addi", pc);
                printf(" x%-2d", rd);
                printf(" x%-2d:%u(%d)", rs1, x[rs1], x[rs1]);
                printf(" 0x%08X(%d)", immI, immI);
                printf(" => 0x%08X(%d)", x[rd], x[rd]);
                puts("");
            }
            x[rd] = x[rs1] + immI;
            break;
        }
        default: {
            std::cerr << "unimplemented instruction " << insn << " with opcode " << opcode << '\n';
        }
    }
    return status;
}

