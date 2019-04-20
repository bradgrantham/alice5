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
    void write8(uint32_t addr, uint32_t v)
    {
        assert(addr < memorybytes.size());
        memorybytes[addr] = static_cast<uint8_t>(v);
    }
    void write16(uint32_t addr, uint32_t v)
    {
        assert(addr + 1 < memorybytes.size());
        *reinterpret_cast<uint16_t*>(memorybytes.data() + addr) = static_cast<uint16_t>(v);
    }
    void write32(uint32_t addr, uint32_t v)
    {
        assert(addr + 3 < memorybytes.size());
        *reinterpret_cast<uint32_t*>(memorybytes.data() + addr) = v;
    }
};

int main(int argc, char **argv)
{
    {
        GPUCore core;
        Memory m({0x13, 0x05, 0x55, 0x03});     /* i = i + 53 */
        core.step(m);
        assert(core.x[10] == 53);
        if(false) dumpGPUCore(core);
    }
    {
        GPUCore core;
        // unsigned int* ptr = (unsigned int *)0x1000;
        // ptr[0] = 0xBADACCE5;
        std::vector<uint8_t> bytes = { 0x37, 0x17, 0x00, 0x00, 0xb7, 0xd7, 0xda, 0xba, 0x93, 0x87, 0x57, 0xce, 0x23, 0x20, 0xf7, 0x00 };
        bytes.resize(0x2000);
        Memory m(bytes);
        core.step(m);
        core.step(m);
        core.step(m);
        core.step(m);
        assert(m.read32(0x1000) == 0xBADACCE5);
        if(true) dumpGPUCore(core);
    }
}
