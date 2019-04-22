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
        printf("read 8 from %08X\n", addr);
        if(!(addr < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        return memorybytes[addr];
    }
    uint16_t read16(uint32_t addr)
    {
        printf("read 16 from %08X\n", addr);
        if(!(addr + 1 < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        return *reinterpret_cast<uint16_t*>(memorybytes.data() + addr);
    }
    uint32_t read32(uint32_t addr)
    {
        printf("read 32 from %08X\n", addr);
        if(!(addr + 3 < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
    uint32_t read32quiet(uint32_t addr)
    {
        if(!(addr + 3 < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
    void write8(uint32_t addr, uint32_t v)
    {
        printf("write 8 bits of %08X to %08X\n", v, addr);
        if(!(addr < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        memorybytes[addr] = static_cast<uint8_t>(v);
    }
    void write16(uint32_t addr, uint32_t v)
    {
        printf("write 16 bits of %08X to %08X\n", v, addr);
        if(!(addr + 1 < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
        *reinterpret_cast<uint16_t*>(memorybytes.data() + addr) = static_cast<uint16_t>(v);
    }
    void write32(uint32_t addr, uint32_t v)
    {
        printf("write %08X to %08X\n", v, addr);
        if(!(addr + 3 < memorybytes.size())) {
            throw std::runtime_error("read outside memory at " + std::to_string(addr));
        }
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

template <class T>
void disassemble(uint32_t pc, T& memory)
{
    std::cout << "pc :";
    std::cout << std::hex << std::setw(8) << std::setfill('0') << pc;
    std::cout << " " << std::hex << std::setw(8) << std::setfill('0') << memory.read32quiet(pc);
    std::cout << std::setw(0) << std::dec << std::setfill(' ') << '\n';
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " bin\n";
        exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> bytes = readFileContents(argv[1]);

    for(auto b: { 0x73, 0x00, 0x10, 0x00 }) // EBREAK, in case code has no exit
        bytes.push_back(b);

    bytes.resize(0x10000);
    GPUCore core;
    Memory m(bytes);

    core.x[1] = 0xffffffff; // Set PC to unlikely value to catch ret with no caller

    GPUCore::Status status;

    try {
        do {
            disassemble(core.pc, m);
            status = core.step(m);
        } while(core.pc != 0xffffffff && status == GPUCore::RUNNING);
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        dumpGPUCore(core);
    }
}
