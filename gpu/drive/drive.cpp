#include <cstdio>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FPGA_MANAGER_BASE 0xFF706000
#define FPGA_MANAGER_SIZE 64
#define FPGA_GPO_OFFSET 0x10
#define FPGA_GPI_OFFSET 0x14
#define SHARED_MEM_BASE 0x3E000000
#define SHARED_MEM_SIZE (16*1024*1024)

#define SIMULATE 1
constexpr bool dumpH2FAndF2H = false;

#include "risc-v.h"
#include "timer.h"
#include "disassemble.h"

#include "objectfile.h"

template<class T>
constexpr const T& clamp( const T& v, const T& lo, const T& hi )
{
        assert( !(hi < lo) );
        return (v < lo) ? lo : (hi < v) ? hi : v;
}

// Proposed H2F interface for testing a single core
constexpr uint32_t h2f_reset_n_bit          = 0x80000000;
constexpr uint32_t h2f_not_reset            = h2f_reset_n_bit;
constexpr uint32_t h2f_run_bit              = 0x40000000;
constexpr uint32_t h2f_request_bit          = 0x20000000;

constexpr uint32_t h2f_gpu_reset            = 0;
constexpr uint32_t h2f_gpu_idle             = h2f_not_reset;
constexpr uint32_t h2f_gpu_request_cmd      = h2f_not_reset | h2f_request_bit;
constexpr uint32_t h2f_gpu_run              = h2f_not_reset | h2f_run_bit;

constexpr uint32_t h2f_cmd_mask             = 0x00FF0000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_put_low_16       = 0x00000000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_put_high_16      = 0x00010000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_write_inst_ram   = 0x00020000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_write_data_ram   = 0x00030000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_read_inst_ram    = 0x00040000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_read_data_ram    = 0x00050000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_read_x_reg       = 0x00060000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_read_f_reg       = 0x00070000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_read_special     = 0x00080000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_special_PC           = 0x00000000;
constexpr uint32_t h2f_cmd_get_low_16       = 0x00090000 | h2f_gpu_request_cmd;
constexpr uint32_t h2f_cmd_get_high_16      = 0x000A0000 | h2f_gpu_request_cmd;

// Proposed F2H interface for testing a single core
constexpr uint32_t f2h_exited_reset_bit     = 0x80000000;
constexpr uint32_t f2h_busy_bit             = 0x40000000;
constexpr uint32_t f2h_cmd_error_bit        = 0x20000000;
constexpr uint32_t f2h_run_halted_bit       = 0x10000000;
constexpr uint32_t f2h_run_exception_bit    = 0x08000000;

constexpr uint32_t f2h_run_exc_data_mask    = 0x00FFFFFF;
constexpr uint32_t f2h_phase_busy           = f2h_busy_bit;
constexpr uint32_t f2h_phase_ready          = 0;
constexpr uint32_t f2h_cmd_response_mask    = 0x0000FFFF;

const char *cmdToString(uint32_t h2f)
{
    if(!(h2f & h2f_request_bit)) {
        return "no request so no command";
    }
    switch(h2f & h2f_cmd_mask) {
        case h2f_cmd_put_low_16: return "h2f_cmd_put_low_16";
        case h2f_cmd_put_high_16: return "h2f_cmd_put_high_16";
        case h2f_cmd_write_inst_ram: return "h2f_cmd_write_inst_ram";
        case h2f_cmd_write_data_ram: return "h2f_cmd_write_data_ram";
        case h2f_cmd_read_inst_ram: return "h2f_cmd_read_inst_ram";
        case h2f_cmd_read_data_ram: return "h2f_cmd_read_data_ram";
        case h2f_cmd_read_x_reg: return "h2f_cmd_read_x_reg";
        case h2f_cmd_read_f_reg: return "h2f_cmd_read_f_reg";
        case h2f_cmd_read_special: return "h2f_cmd_read_special";
        case h2f_cmd_get_low_16: return "h2f_cmd_get_low_16";
        case h2f_cmd_get_high_16: return "h2f_cmd_get_high_16";
        default : return "unknown cmd"; break;
    }
}

using namespace std::chrono_literals;
#if SIMULATE
constexpr std::chrono::duration<float, std::micro> h2f_timeout_micros = 1000000us;
#else
constexpr std::chrono::duration<float, std::micro> h2f_timeout_micros = 10us;
#endif

struct CoreShared
{
    bool coreHadAnException = false; // Only set to true after initialization
    unsigned char *img;

    volatile unsigned long *gpo;
    volatile unsigned long *gpi;
    volatile void *sdram;

    void setH2F(uint32_t state) {
        if(dumpH2FAndF2H) {
            std::cout << "set sim_h2f_value to " << to_hex(state) << "\n";
        }
        *gpo = state;
    }

    uint32_t getF2H()
    {
        uint32_t f2h_value = *gpi;
        if(dumpH2FAndF2H) {
            std::cout << "get sim_f2h_value yields " << to_hex(f2h_value) << "\n";
        }
        return f2h_value;
    }
};

void allowGPUProgress()
{
        // usleep(1);
}

// Returns whether it reset before timing out.
bool waitOnExitReset(CoreShared *shared)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnExitReset()...\n";
    }

    allowGPUProgress();

    auto start = std::chrono::high_resolution_clock::now();
    while((shared->getF2H() & f2h_exited_reset_bit) != f2h_exited_reset_bit) {

        allowGPUProgress();

        // Check clock for timeout
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout_micros) {
            std::cerr << "waitOnExitReset : timed out waiting for FPGA to change phase (" << elapsed.count() << ")\n";
            return false;
        }
    }
    return true;
}

bool waitOnPhaseOrTimeout(int phase, CoreShared *shared)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnPhaseOrTimeout(" << to_hex(phase) << ")...\n";
    }
    uint32_t phaseExpected = phase ? f2h_busy_bit : 0;

    allowGPUProgress();

    // auto start = std::chrono::high_resolution_clock::now();
    while((shared->getF2H() & f2h_busy_bit) != phaseExpected) {

        allowGPUProgress();

        // Check clock for timeout
        /*
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout_micros) {
            std::cerr << "waitOnPhaseOrTimeout : timed out waiting for FPGA to change phase (" << elapsed.count() << ")\n";
            return false;
        }
        */
    }
    return true;
}

bool processCommand(uint32_t command, CoreShared *shared)
{
    // double-check we are in idle
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, shared)) {
        std::cerr << "processCommand : timeout waiting on FPGA to become idle before sending command\n";
        return false;
    }
    
    // send the command and then wait for core to report it is processing the command
    shared->setH2F(command);
    if(!waitOnPhaseOrTimeout(f2h_phase_busy, shared)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate busy processing command\n";
        return false;
    }

    // return to idle state and wait for the core to tell us it's also idle.
    shared->setH2F(h2f_gpu_idle);
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, shared)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate command is completed\n";
        return false;
    }

    return true;
}

enum RamType { INST_RAM, DATA_RAM };

void CHECK(bool success, const char *filename, int line)
{
    static bool stored_exit_flag = false;
    static bool exit_on_error;

    if(!stored_exit_flag) {
        exit_on_error = getenv("EXIT_ON_ERROR") != NULL;
        stored_exit_flag = true;
    }

    if(!success) {
        printf("command processing error at %s:%d\n", filename, line);
        if(exit_on_error)
            exit(1);
    }
}

void writeWordToRam(uint32_t value, uint32_t address, RamType ramType, CoreShared *shared)
{
    // put low 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_low_16 | (value & 0xFFFF), shared), __FILE__, __LINE__);

    // put high 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_high_16 | ((value >> 16) & 0xFFFF), shared), __FILE__, __LINE__);

    // send the command to store the word from the write register into memory
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_write_inst_ram | (address & 0xFFFF), shared), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_write_data_ram | (address & 0xFFFF), shared), __FILE__, __LINE__);
    }
}

uint32_t readWordFromRam(uint32_t address, RamType ramType, CoreShared *shared)
{
    uint32_t value;

    // send the command to latch the word from memory into the read register
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_read_inst_ram | (address & 0xFFFF), shared), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_data_ram | (address & 0xFFFF), shared), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, shared), __FILE__, __LINE__);

    // get low 16 bits into value
    value = shared->getF2H() & 0xFFFF;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, shared), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((shared->getF2H() & 0xFFFF) << 16);

    return value;
}

enum RegisterKind {X_REG, F_REG, PC_REG};

uint32_t readReg(RegisterKind kind, int reg, CoreShared* shared)
{
    uint32_t value;

    // send the command to latch the register into the read register
    if(kind == X_REG) {
        CHECK(processCommand(h2f_cmd_read_x_reg | (reg & 0x1F), shared), __FILE__, __LINE__);
    } else if(kind == PC_REG) {
        CHECK(processCommand(h2f_cmd_read_special | h2f_special_PC, shared), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_f_reg | (reg & 0x1F), shared), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, shared), __FILE__, __LINE__);

    // get low 16 bits into value
    value = shared->getF2H() & f2h_cmd_response_mask;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, shared), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((shared->getF2H() & f2h_cmd_response_mask) << 16);

    return value;
}

uint32_t readPC(CoreShared* shared)
{
    return readReg(PC_REG, 0, shared);
}


void writeBytesToRam(const std::vector<uint8_t>& bytes, RamType ramType, CoreShared *shared)
{
    // Write inst bytes to inst memory
    for(uint32_t byteaddr = 0; byteaddr < bytes.size(); byteaddr += 4) {
        uint32_t word = 
            bytes[byteaddr + 0] <<  0 |
            bytes[byteaddr + 1] <<  8 |
            bytes[byteaddr + 2] << 16 |
            bytes[byteaddr + 3] << 24;

        writeWordToRam(word, byteaddr, ramType, shared);
    }
}

template <class TYPE>
void set(CoreShared *shared, uint32_t address, const TYPE& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    writeWordToRam(*reinterpret_cast<const uint32_t*>(&value), address, DATA_RAM, shared);
}

template <class TYPE, std::size_t N>
void set(CoreShared *shared, uint32_t address, const std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        writeWordToRam(*reinterpret_cast<const uint32_t*>(&value[i]), address + i * sizeof(uint32_t), DATA_RAM, shared);
}

template <class TYPE, std::size_t N>
void get(CoreShared *shared, uint32_t address, std::array<TYPE, N>& value)
{
    // TODO: use a state machine through Main to read memory using clock
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        *reinterpret_cast<uint32_t*>(&value[i]) = readWordFromRam(address + i * sizeof(uint32_t), DATA_RAM, shared);
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
    printf("usage: %s [options] shader.o\n", progname);
    printf("options:\n");
    printf("\t-f N      Render frame N\n");
    printf("\t-v        Print memory access\n");
    printf("\t-S        show the disassembly of the SPIR-V code\n");
    printf("\t--term    draw output image on terminal (in addition to file)\n");
    printf("\t-d        print symbols\n");
    printf("\t--header  print information about the object file and\n");
    printf("\t          exit before emulation\n");
}

struct CoreParameters
{
    std::vector<uint8_t> inst_bytes;
    std::vector<uint8_t> data_bytes;
    SymbolTable text_symbols;
    SymbolTable data_symbols;

    AddressToSymbolMap textAddressesToSymbols;

    uint32_t gl_FragCoordAddress;
    uint32_t colorAddress;
    uint32_t iTimeAddress;
    uint32_t rowWidthAddress;
    uint32_t colBufAddrAddress;

    uint32_t initialPC;

    int imageWidth;
    int imageHeight;
    float frameTime;
    int startX;
    int startY;
    int afterLastX;
    int afterLastY;
};

struct SimDebugOptions
{
};

void shadeOnePixel(const SimDebugOptions* debugOptions, const CoreParameters *params, CoreShared *shared)
{
    // Run one clock cycle in not-run reset to process STATE_INIT
    shared->setH2F(h2f_gpu_reset);
    // usleep(1);

    // Release reset
    shared->setH2F(h2f_gpu_idle);
    waitOnExitReset(shared);

    shared->setH2F(h2f_gpu_run);

    while (!(shared->getF2H() & f2h_run_halted_bit)) {
            // usleep(1);
    }
}

void render(const SimDebugOptions* debugOptions, const CoreParameters* params, CoreShared* shared, int start_row)
{
    // Set up inst Ram
    // Run one clock cycle in reset to process STATE_INIT
    shared->setH2F(h2f_gpu_reset);
    usleep(1);

    // Release reset
    shared->setH2F(h2f_gpu_idle);
    waitOnExitReset(shared);

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->inst_bytes, INST_RAM, shared);

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->data_bytes, DATA_RAM, shared);

    const float fw = params->imageWidth;
    const float fh = params->imageHeight;
    const float one = 1.0;

    if (params->data_symbols.find(".anonymous") != params->data_symbols.end()) {
        set(shared, params->data_symbols.find(".anonymous")->second, v3float{fw, fh, one});
        set(shared, params->data_symbols.find(".anonymous")->second + sizeof(v3float), params->frameTime);
    }

    for(int j = start_row; j < params->afterLastY; j += 1) {
        if (j % 10 == 0) {
            std::cout << "j = " << j << "\n";
        }

        if (true) {
            // Do a whole row at a time.
            if(params->gl_FragCoordAddress != 0xFFFFFFFF) {
                set(shared, params->gl_FragCoordAddress,
                        v4float{params->startX + 0.5f, j + 0.5f, 0, 0});
            }
            if(false) {
                // XXX TODO: when iTime is exported by compilation
                set(shared, params->iTimeAddress, params->frameTime);
            }
            // Number of pixels in a row.
            uint32_t width = params->afterLastX - params->startX;
            set(shared, params->rowWidthAddress, width);
            // Offset in pixels into the image.
            uint32_t pixelOffset = (params->imageHeight - 1 - j)*params->imageWidth + params->startX;
            // Offset in bytes into the shared memory space.
            uint32_t offsetAddress = sizeof(float)*3*pixelOffset;
            set(shared, params->colBufAddrAddress, offsetAddress | 0x80000000);

            // Run one clock cycle in not-run reset to process STATE_INIT
            shared->setH2F(h2f_gpu_reset);

            // Release reset
            shared->setH2F(h2f_gpu_idle);
            waitOnExitReset(shared);

            // Run the row.
            shared->setH2F(h2f_gpu_run);
            while (!(shared->getF2H() & f2h_run_halted_bit)) {
                // Nothing.
            }

            /* return to STATE_INIT so we can drive ext memory access */
            shared->setH2F(h2f_gpu_idle);

            // Convert to bytes.
            float *rgbFloat = (float *) shared->sdram + pixelOffset*3;
            uint8_t *rgbByte = shared->img + pixelOffset*3;
            for (int i = 0; i < width; i++) {
                for (int c = 0; c < 3; c++) {
                    rgbByte[c] = clamp(int(rgbFloat[c] * 255.99), 0, 255);
                }
                rgbFloat += 3;
                rgbByte += 3;
            }
        } else {
            // Do a pixel at a time.
            for(int i = params->startX; i < params->afterLastX; i++) {
                if(params->gl_FragCoordAddress != 0xFFFFFFFF)
                    set(shared, params->gl_FragCoordAddress, v4float{i + 0.5f, j + 0.5f, 0, 0});
                if(params->colorAddress != 0xFFFFFFFF)
                    set(shared, params->colorAddress, v4float{1, 1, 1, 1});
                if(false) // XXX TODO: when iTime is exported by compilation
                    set(shared, params->iTimeAddress, params->frameTime);

                // core.regs.pc = params->initialPC; // XXX Ignored

                shadeOnePixel(debugOptions, params, shared);

                /* return to STATE_INIT so we can drive ext memory access */
                shared->setH2F(h2f_gpu_idle);

                v3float rgb = {1, 0, 0};
                if(params->colorAddress != 0xFFFFFFFF)
                    get(shared, params->colorAddress, rgb);

                int pixelOffset = 3 * ((params->imageHeight - 1 - j) * params->imageWidth + i);
                for(int c = 0; c < 3; c++)
                    shared->img[pixelOffset + c] = clamp(int(rgb[c] * 255.99), 0, 255);
            }
        }
    }
}



int main(int argc, char **argv)
{
    bool imageToTerminal = false;
    bool printSymbols = false;
    bool printSubstitutions = false;
    bool printHeaderInfo = false;
    int specificPixelX = -1;
    int specificPixelY = -1;

    SimDebugOptions debugOptions;
    CoreParameters params;
    CoreShared shared;

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "--pixel") == 0) {

            if(argc < 2) {
                std::cerr << "Expected pixel X and Y for \"--pixel\"\n";
                usage(progname);
                exit(EXIT_FAILURE);
            }
            specificPixelX = atoi(argv[1]);
            specificPixelY = atoi(argv[2]);
            argv+=3; argc-=3;

        } else if(strcmp(argv[0], "--header") == 0) {

            printHeaderInfo = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-f") == 0) {

            if(argc < 2) {
                std::cerr << "Expected frame parameter for \"-f\"\n";
                usage(progname);
                exit(EXIT_FAILURE);
            }
            params.frameTime = atoi(argv[1]) / 60.0f;
            argv+=2; argc-=2;

        } else if(strcmp(argv[0], "-d") == 0) {

            printSymbols = true;
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

    RunHeader2 header;

    std::ifstream binaryFile(argv[0], std::ios::in | std::ios::binary);
    if(!binaryFile.good()) {
        throw std::runtime_error(std::string("couldn't open file ") + argv[0] + " for reading");
    }

    if(!ReadBinary(binaryFile, header, params.text_symbols, params.data_symbols, params.inst_bytes, params.data_bytes)) {
        exit(EXIT_FAILURE);
    }
    if(printSymbols) {
        for(auto& it: params.text_symbols) {
            auto symbol = it.first;
            auto address = it.second;
            std::cout << "text segment symbol \"" << symbol << "\" is at " << address << "\n";
        }
        for(auto& it: params.data_symbols) {
            auto symbol = it.first;
            auto address = it.second;
            std::cout << "data segment symbol \"" << symbol << "\" is at " << address << "\n";
        }
    }
    if(printHeaderInfo) {
        std::cout << params.inst_bytes.size() << " bytes of inst memory\n";
        std::cout << params.data_bytes.size() << " bytes of data memory\n";
        std::cout << std::setfill('0');
        std::cout << "initial PC is " << header.initialPC << " (0x" << std::hex << std::setw(8) << header.initialPC << std::dec << ")\n";
        exit(EXIT_SUCCESS);
    }
    assert(params.inst_bytes.size() % 4 == 0);

    params.initialPC = header.initialPC;

    binaryFile.close();

    for(auto& it: params.text_symbols) {
        auto symbol = it.first;
        auto address = it.second;
        params.textAddressesToSymbols[address] = symbol;
    }

    assert(params.data_bytes.size() < RiscVInitialStackPointer);
    if(params.data_bytes.size() > RiscVInitialStackPointer - 1024) {
        std::cerr << "Warning: stack will be less than 1KiB with this binary's data segment.\n";
    }

    params.imageWidth = 320;
    params.imageHeight = 180;

    for(auto& s: { "gl_FragCoord", "color"}) {
        if (params.data_symbols.find(s) == params.data_symbols.end()) {
            std::cerr << "No memory location for required variable " << s << ".\n";
            // exit(EXIT_FAILURE);
        }
    }
    for(auto& s: { "iResolution", "iTime", "iMouse"}) {
        if (params.data_symbols.find(s) == params.data_symbols.end()) {
            // Don't warn, these are in the anonymous params.
            // std::cerr << "Warning: No memory location for variable " << s << ".\n";
        }
    }

    shared.img = new unsigned char[params.imageWidth * params.imageHeight * 3];

    if(params.data_symbols.find("gl_FragCoord") != params.data_symbols.end()) {
        params.gl_FragCoordAddress = params.data_symbols["gl_FragCoord"];
    } else {
        params.gl_FragCoordAddress = 0xFFFFFFFF;
    }
    if(params.data_symbols.find("color") != params.data_symbols.end()) {
        params.colorAddress = params.data_symbols["color"];
    } else {
        params.colorAddress = 0xFFFFFFFF;
    }
    if(params.data_symbols.find("iTime") != params.data_symbols.end()) {
        params.iTimeAddress = params.data_symbols["iTime"];
    } else {
        params.iTimeAddress = 0xFFFFFFFF;
    }
    params.rowWidthAddress = params.data_symbols.at(".rowWidth");
    params.colBufAddrAddress = params.data_symbols.at(".colBufAddr");

    params.startX = 0;
    params.afterLastX = params.imageWidth;
    params.startY = 0;
    params.afterLastY = params.imageHeight;

    if(specificPixelX != -1) {
        params.startX = specificPixelX;
        params.afterLastX = specificPixelX + 1;
        params.startY = specificPixelY;
        params.afterLastY = specificPixelY + 1;
    }

    int devMem = open("/dev/mem", O_RDWR);
    if(devMem == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // For GP register.
    unsigned char *mem = (unsigned char *)mmap(0, FPGA_MANAGER_SIZE, PROT_READ | PROT_WRITE, /* MAP_NOCACHE | */ MAP_SHARED , devMem, FPGA_MANAGER_BASE);
    if(mem == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    shared.gpo = (unsigned long*)(mem + FPGA_GPO_OFFSET);
    shared.gpi = (unsigned long*)(mem + FPGA_GPI_OFFSET);

    // For shared memory.
    shared.sdram = mmap(0, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, /* MAP_NOCACHE | */ MAP_SHARED , devMem, SHARED_MEM_BASE);
    if(shared.sdram == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    render(&debugOptions, &params, &shared, params.startY);

    if(shared.coreHadAnException) {
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen("fpga.ppm", "wb");
    fprintf(fp, "P6 %d %d 255\n", params.imageWidth, params.imageHeight);
    fwrite(shared.img, 1, params.imageWidth * params.imageHeight * 3, fp);
    fclose(fp);

    if (imageToTerminal) {
        // https://www.iterm2.com/documentation-images.html
        std::ostringstream ss;
        ss << "P6 " << params.imageWidth << " " << params.imageHeight << " 255\n";
        ss.write(reinterpret_cast<char *>(shared.img), 3*params.imageWidth*params.imageHeight);
        std::cout << "\033]1337;File=width="
            << params.imageWidth << "px;height="
            << params.imageHeight << "px;inline=1:"
            << base64Encode(ss.str()) << "\007\n";
    }
}
