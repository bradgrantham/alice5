#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "gpuemu.h"

    
void dumpGPUCore(const GPUCore& core)
{
    std::cout << std::setfill('0');
    for(int i = 0; i < 32; i++) {
        std::cout << "x" << std::setw(2) << i << ":" << std::hex << std::setw(8) << core.x[i] << std::dec;
        std::cout << (((i + 1) % 4 == 0) ? "\n" : " ");
    }
    std::cout << "pc :" << std::hex << std::setw(8) << core.pc << '\n' << std::dec;
    std::cout << std::setfill(' ');
}

struct Memory
{
    std::vector<uint8_t> memorybytes;
    Memory(const std::vector<uint8_t>& memorybytes_) :
        memorybytes(memorybytes_)
    {}
    uint8_t read8(uint32_t addr)
    {
        assert(addr < memorybytes.size());
        return memorybytes[addr];
    }
    uint16_t read16(uint32_t addr)
    {
        assert(addr + 1 < memorybytes.size());
        return *reinterpret_cast<uint16_t*>(memorybytes.data() + addr);
    }
    uint32_t read32(uint32_t addr)
    {
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
};

int main(int argc, char **argv)
{
    {
        GPUCore core;
        Memory m({0x13, 0x05, 0x55, 0x03});
        core.step(m, false);
        assert(core.x[10] == 53);
        if(false) dumpGPUCore(core);
    }
}
