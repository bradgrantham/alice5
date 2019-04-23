#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>
#include "gpuemu.h"
#include "riscv-disas.h"

void print_inst(uint64_t pc, uint32_t inst)
{
    char buf[80] = { 0 };
    disasm_inst(buf, sizeof(buf), rv64, pc, inst);
    printf("%016" PRIx64 ":  %s\n", pc, buf);
}

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
    bool verbose;
    Memory(const std::vector<uint8_t>& memorybytes_, bool verbose_ = false) :
        memorybytes(memorybytes_),
        verbose(verbose_)
    {}
    uint8_t read8(uint32_t addr)
    {
        if(verbose) printf("read 8 from %08X\n", addr);
        assert(addr < memorybytes.size());
        return memorybytes[addr];
    }
    uint16_t read16(uint32_t addr)
    {
        if(verbose) printf("read 16 from %08X\n", addr);
        assert(addr + 1 < memorybytes.size());
        return *reinterpret_cast<uint16_t*>(memorybytes.data() + addr);
    }
    uint32_t read32(uint32_t addr)
    {
        if(verbose) printf("read 32 from %08X\n", addr);
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
    void write8(uint32_t addr, uint32_t v)
    {
        if(verbose) printf("write 8 bits of %08X to %08X\n", v, addr);
        assert(addr < memorybytes.size());
        memorybytes[addr] = static_cast<uint8_t>(v);
    }
    void write16(uint32_t addr, uint32_t v)
    {
        if(verbose) printf("write 16 bits of %08X to %08X\n", v, addr);
        assert(addr + 1 < memorybytes.size());
        *reinterpret_cast<uint16_t*>(memorybytes.data() + addr) = static_cast<uint16_t>(v);
    }
    void write32(uint32_t addr, uint32_t v)
    {
        if(verbose) printf("write 32 bits of %08X to %08X\n", v, addr);
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
        Memory m({0x13, 0x05, 0x55, 0x03, 0x73, 0x00, 0x10, 0x00});     /* i = i + 53; ebreak; */
        GPUCore::Status status = core.stepUntilException(m);
        assert(status == GPUCore::BREAK);
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
        core.step(m);       core.step(m);
        assert(m.read32(0x1000) == 0xBADACCE5);
        if(false) dumpGPUCore(core);
    }

    {
        GPUCore core;
        // int j = 9;
        // for(int i = 0; i < 1000000; i++)
        // 	j = j * 3;
        // riscv32-none-elf-gcc -mabi=ilp32 -march=rv32im -S q.c
        std::vector<uint8_t> bytes = { 0x13, 0x01, 0x01, 0xfd, 0x23, 0x26, 0x81, 0x02, 0x13, 0x04, 0x01, 0x03, 0x23, 0x2e, 0xa4, 0xfc, 0x93, 0x07, 0x90, 0x00, 0x23, 0x26, 0xf4, 0xfe, 0x23, 0x24, 0x04, 0xfe, 0x6f, 0x00, 0x40, 0x02, 0x03, 0x27, 0xc4, 0xfe, 0x93, 0x07, 0x07, 0x00, 0x93, 0x97, 0x17, 0x00, 0xb3, 0x87, 0xe7, 0x00, 0x23, 0x26, 0xf4, 0xfe, 0x83, 0x27, 0x84, 0xfe, 0x93, 0x87, 0x17, 0x00, 0x23, 0x24, 0xf4, 0xfe, 0x03, 0x27, 0x84, 0xfe, 0xb7, 0x47, 0x0f, 0x00, 0x93, 0x87, 0xf7, 0x23, 0xe3, 0xda, 0xe7, 0xfc, 0x13, 0x00, 0x00, 0x00, 0x13, 0x85, 0x07, 0x00, 0x03, 0x24, 0xc1, 0x02, 0x13, 0x01, 0x01, 0x03, 0x73, 0x00, 0x10, 0x00 };
        bytes.resize(0x2000);
        Memory m(bytes);
        core.x[2] = 0x2000;     // Set SP to highest memory
        GPUCore::Status status;
        do{
            // print_inst(core.pc, m.read32(core.pc));
            status = core.step(m);
        } while(status == GPUCore::RUNNING);
        assert(m.read32(0x00001FEC) == 0x184ECD09); 
    }

    {
        GPUCore core;
//    unsigned char *pi = (unsigned char *)0x1000;
//    int i = *pi;
//    int j = 9;
//    unsigned char *pb = (unsigned char *)0x1004;
//    for(int k = 0; k < 1000; k++) {
//	j = j * 3;
//	if(j > i)
//	    break;
//    }
//    *pb = j & 0xff;
        std::vector<uint8_t> bytes = {
            0x13, 0x01, 0x01, 0xfd, 0x23, 0x26, 0x81, 0x02, 0x13, 0x04, 0x01, 0x03, 0xb7, 0x17, 0x00, 0x00, 0x23, 0x22, 0xf4, 0xfe, 0x83, 0x27, 0x44, 0xfe, 0x83, 0xc7, 0x07, 0x00, 0x23, 0x20, 0xf4, 0xfe, 0x93, 0x07, 0x90, 0x00, 0x23, 0x26, 0xf4, 0xfe, 0xb7, 0x17, 0x00, 0x00, 0x93, 0x87, 0x47, 0x00, 0x23, 0x2e, 0xf4, 0xfc, 0x23, 0x24, 0x04, 0xfe, 0x6f, 0x00, 0x00, 0x03, 0x03, 0x27, 0xc4, 0xfe, 0x93, 0x07, 0x07, 0x00, 0x93, 0x97, 0x17, 0x00, 0xb3, 0x87, 0xe7, 0x00, 0x23, 0x26, 0xf4, 0xfe, 0x03, 0x27, 0xc4, 0xfe, 0x83, 0x27, 0x04, 0xfe, 0x63, 0xc0, 0xe7, 0x02, 0x83, 0x27, 0x84, 0xfe, 0x93, 0x87, 0x17, 0x00, 0x23, 0x24, 0xf4, 0xfe, 0x03, 0x27, 0x84, 0xfe, 0x93, 0x07, 0x70, 0x3e, 0xe3, 0xd6, 0xe7, 0xfc, 0x6f, 0x00, 0x80, 0x00, 0x13, 0x00, 0x00, 0x00, 0x83, 0x27, 0xc4, 0xfe, 0x13, 0xf7, 0xf7, 0x0f, 0x83, 0x27, 0xc4, 0xfd, 0x23, 0x80, 0xe7, 0x00, 0x13, 0x00, 0x00, 0x00, 0x13, 0x85, 0x07, 0x00, 0x03, 0x24, 0xc1, 0x02, 0x13, 0x01, 0x01, 0x03,
            0x73, 0x00, 0x10, 0x00,
        };
        bytes.resize(0x2000);
        Memory m(bytes, false);
        m.write32(0x1000, 43);
        core.x[2] = 0x2000;     // Set SP to highest memory
        GPUCore::Status status;
        do{
            // print_inst(core.pc, m.read32(core.pc));
            status = core.step(m);
        } while(status == GPUCore::RUNNING);
        assert(m.read8(0x00001004) == 0x51); 
        // dumpGPUCore(core);
    }
}
