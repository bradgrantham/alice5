#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <cstdlib>
#include "gpuemu.h"

extern "C" {
#include "riscv-disas.h"
}

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

uint32_t colorAddress = 6144; // vec4 
uint32_t gl_FragCoordAddress = 1024; // vec4 
uint32_t iMouseAddress = 2064; // ivec4 
uint32_t iResolutionAddress = 2048; // vec2 
uint32_t iTimeAddress = 2056; // float 

// XXX bin may be loaded anywhere, so must be -fPIC
int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " bin\n";
        exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> bytes = readFileContents(argv[1]);

    bytes.resize(0x20000);
    GPUCore core;
    Memory m(bytes);

    core.x[1] = 0xffffffff; // Set PC to unlikely value to catch ret with no caller
    core.x[2] = 0x20000; // Set SP to end of memory 
    core.pc = 0x10000;

    GPUCore::Status status;

    int imageWidth = 320;
    int imageHeight = 180;

    float fw = imageWidth;
    float fh = imageHeight;
    float zero = 0.0;
    float when = 1.5;
    m.write32(iResolutionAddress +  0, *reinterpret_cast<uint32_t*>(&fw));
    m.write32(iResolutionAddress +  4, *reinterpret_cast<uint32_t*>(&fh));
    m.write32(iResolutionAddress +  8, *reinterpret_cast<uint32_t*>(&zero));
    m.write32(iResolutionAddress + 12, *reinterpret_cast<uint32_t*>(&zero));
    m.write32(iTimeAddress +  0, *reinterpret_cast<uint32_t*>(&when));
    m.write32(iMouseAddress +  0, 0);
    m.write32(iMouseAddress +  4, 0);
    m.write32(iMouseAddress +  8, 0);
    m.write32(iMouseAddress + 12, 0);
    unsigned char *img = new unsigned char[imageWidth * imageHeight * 3];
    for(int j = 0; j < 180; j++)
        for(int i = 0; i < 180; i++) {
            float x = i, y = j, z = 0, w = 0;
            m.write32(gl_FragCoordAddress +  0, *reinterpret_cast<uint32_t*>(&x));
            m.write32(gl_FragCoordAddress +  0, *reinterpret_cast<uint32_t*>(&y));
            m.write32(gl_FragCoordAddress +  0, *reinterpret_cast<uint32_t*>(&z));
            m.write32(gl_FragCoordAddress +  0, *reinterpret_cast<uint32_t*>(&w));
            try {
                do {
                    print_inst(core.pc, m.read32(core.pc));
                    status = core.step(m);
                } while(core.pc != 0xffffffff && status == GPUCore::RUNNING);
            } catch(const std::exception& e) {
                std::cerr << e.what() << '\n';
                dumpGPUCore(core);
                exit(EXIT_FAILURE);
            }
            if(status != GPUCore::BREAK) {
                std::cerr << "unexpected core step result " << status << '\n';
                dumpGPUCore(core);
                exit(EXIT_FAILURE);
            }
            uint32_t ir = m.read32(colorAddress +  0);
            uint32_t ig = m.read32(colorAddress +  4);
            uint32_t ib = m.read32(colorAddress +  8);
            uint32_t ia = m.read32(colorAddress + 12);
            float r = *reinterpret_cast<float*>(&ir);
            float g = *reinterpret_cast<float*>(&ig);
            float b = *reinterpret_cast<float*>(&ib);
            float a = *reinterpret_cast<float*>(&ia);
            img[3 * (j * imageWidth + i) + 0] = static_cast<unsigned char>(r * 255.99);
            img[3 * (j * imageWidth + i) + 1] = static_cast<unsigned char>(g * 255.99);
            img[3 * (j * imageWidth + i) + 2] = static_cast<unsigned char>(b * 255.99);
        }
    printf("P6 %d %d 255\n", imageWidth, imageHeight);
    fwrite(img, 1, imageWidth * imageHeight * 3, stdout);
}
