#include <vector>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
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
    for(int i = 0; i < 32; i++) {
        std::cout << "ft" << std::setfill('0') << std::setw(2) << i << ":" << std::setw(8) << std::setfill(' ') << core.f[i];
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
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<uint32_t*>(memorybytes.data() + addr);
    }
    float readf(uint32_t addr)
    {
        if(verbose) printf("read float from %08X\n", addr);
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        return *reinterpret_cast<float*>(memorybytes.data() + addr);
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
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        *reinterpret_cast<uint32_t*>(memorybytes.data() + addr) = v;
    }
    void writef(uint32_t addr, float v)
    {
        if(verbose) printf("write float %f to %08X\n", v, addr);
        assert(addr < memorybytes.size());
        assert(addr + 3 < memorybytes.size());
        *reinterpret_cast<float*>(memorybytes.data() + addr) = v;
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

template <class MEMORY, class TYPE>
void set(MEMORY& memory, uint32_t address, const TYPE& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    memory.write32(address, *reinterpret_cast<const uint32_t*>(&value));
}

template <class MEMORY, class TYPE, unsigned long N>
void set(MEMORY& memory, uint32_t address, const std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(int i = 0; i < N; i++)
        memory.write32(address + i * sizeof(uint32_t), *reinterpret_cast<const uint32_t*>(&value[i]));
}

typedef std::array<float,1> v1float;
typedef std::array<uint32_t,1> v1uint;
typedef std::array<int32_t,1> v1int;
typedef std::array<float,2> v2float;
typedef std::array<uint32_t,2> v2uint;
typedef std::array<int32_t,2> v2int;
typedef std::array<float,3> v3float;
typedef std::array<uint32_t,3> v3uint;
typedef std::array<int32_t,3> v3int;
typedef std::array<float,4> v4float;
typedef std::array<uint32_t,4> v4uint;
typedef std::array<int32_t,4> v4int;

// https://stackoverflow.com/a/34571089/211234
static const char *BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64Encode(const std::string &in) {
    std::string out;
    int val = 0;
    int valb = -6;

    for (uint8_t c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_ALPHABET[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(BASE64_ALPHABET[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }

    return out;
}

void usage(const char* progname)
{
    printf("usage: %s [options] shader.blob\n", progname);
    printf("options:\n");
    printf("\t-v        Print memory options\n");
    printf("\t-S        show the disassembly of the SPIR-V code\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
}


int main(int argc, char **argv)
{
    bool verboseMemory = false;
    bool disassemble = false;
    bool imageToTerminal = false;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            verboseMemory = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-S") == 0) {

            disassemble = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-h") == 0) {

            usage(progname);
            exit(EXIT_SUCCESS);

        } else {

            usage(progname);
            exit(EXIT_FAILURE);

        }
    }

    if(argc < 1) {
        usage(progname);
        exit(EXIT_FAILURE);
    }

    std::vector<uint8_t> blob = readFileContents(argv[0]);
    assert(blob.size() > sizeof(RunHeader));
    RunHeader hdr = *reinterpret_cast<RunHeader*>(blob.data());

    std::vector<uint8_t> bytes(blob.begin() + sizeof(RunHeader), blob.end());

    bytes.resize(bytes.size() + 0x10000);
    Memory m(bytes, false);

    GPUCore core;
    GPUCore::Status status;

    int imageWidth = 320;
    int imageHeight = 180;

    float fw = imageWidth;
    float fh = imageHeight;
    float zero = 0.0;
    float when = 1.5;
    if (hdr.iResolutionAddress == 0xFFFFFFFF) {
        std::cerr << "Warning: No memory location for iResolution.\n";
    } else {
        set(m, hdr.iResolutionAddress, v4float{fw, fh, zero, zero});
        set(m, hdr.iResolutionAddress +  0, when);
    }
    unsigned char *img = new unsigned char[imageWidth * imageHeight * 3];

    for(int j = 0; j < imageHeight; j++)
        for(int i = 0; i < imageWidth; i++) {

            float x = i, y = j, z = 0, w = 0;
            set(m, hdr.gl_FragCoordAddress, v4float{x, y, z, w});
            set(m, hdr.colorAddress, v4float{1, 1, 1, 1});

            core.x[1] = 0xffffffff; // Set PC to unlikely value to catch ret with no caller
            core.x[2] = bytes.size(); // Set SP to end of memory 
            core.pc = hdr.initialPC;

            if(disassemble) {
                std::cout << "pixel " << x << ", " << y << '\n';
            }
            try {
                do {
                    if(disassemble) {
                        print_inst(core.pc, m.read32(core.pc));
                    }
                    m.verbose = verboseMemory;
                    status = core.step(m);
                    m.verbose = false;
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

            uint32_t ir = m.read32(hdr.colorAddress +  0);
            uint32_t ig = m.read32(hdr.colorAddress +  4);
            uint32_t ib = m.read32(hdr.colorAddress +  8);
            // uint32_t ia = m.read32(hdr.colorAddress + 12);
            float r = *reinterpret_cast<float*>(&ir);
            float g = *reinterpret_cast<float*>(&ig);
            float b = *reinterpret_cast<float*>(&ib);
            // float a = *reinterpret_cast<float*>(&ia);
            img[3 * ((imageHeight - 1 - j) * imageWidth + i) + 0] = std::clamp(int(r * 255.99), 0, 255);
            img[3 * ((imageHeight - 1 - j) * imageWidth + i) + 1] = std::clamp(int(g * 255.99), 0, 255);
            img[3 * ((imageHeight - 1 - j) * imageWidth + i) + 2] = std::clamp(int(b * 255.99), 0, 255);
        }

    FILE *fp = fopen("emulated.ppm", "wb");
    fprintf(fp, "P6 %d %d 255\n", imageWidth, imageHeight);
    fwrite(img, 1, imageWidth * imageHeight * 3, fp);
    fclose(fp);

    if (imageToTerminal) {
        // https://www.iterm2.com/documentation-images.html
        std::ostringstream ss;
        ss << "P6 " << imageWidth << " " << imageHeight << " 255\n";
        ss.write(reinterpret_cast<char *>(img), 3*imageWidth*imageHeight);
        std::cout << "\033]1337;File=width="
            << imageWidth << "px;height="
            << imageHeight << "px;inline=1:"
            << base64Encode(ss.str()) << "\007\n";
    }
}
