#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
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

std::vector<uint8_t> readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!shaderFile.good()) {
        throw std::runtime_error("couldn't open file " + shaderFileName + " for reading");
    }
    std::ifstream::pos_type size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::vector<uint8_t> text(size);
    shaderFile.read(reinterpret_cast<char*>(text.data()), size);

    return text;
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " bin\n";
        exit(EXIT_FAILURE);
    }

    GPUCore core;
    std::vector<uint8_t> bytes = readFileContents(argv[1]);
    bytes.resize(0x10000);
    Memory m(bytes);
    core.x[1] = 0xffffffff;
    do {
        std::cout << "pc :";
        std::cout << std::hex << std::setw(8) << std::setfill('0') << core.pc;
        std::cout << " " << std::hex << std::setw(8) << std::setfill('0') << m.read32(core.pc);
        std::cout << std::setw(0) << std::dec << std::setfill(' ') << '\n';
        core.step(m);
    } while(core.pc != 0xffffffff);
    if(true) dumpGPUCore(core);
}
