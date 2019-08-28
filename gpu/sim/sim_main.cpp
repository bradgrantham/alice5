#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "risc-v.h"
#include "timer.h"
#include "disassemble.h"

#include "objectfile.h"

#include "VMain.h"
#include "VMain_Main.h"
#include "VMain_Registers.h"
#include "VMain_ShaderCore.h"
#include "VMain_BlockRam__A5.h"
#include "VMain_BlockRam__A10.h"

template<typename T>
std::string to_hex(T i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << std::uppercase;
    stream << uint64_t(i);
    return stream.str();
}

const char *stateToString(int state)
{
    switch(state) {
        case VMain_ShaderCore::STATE_INIT: return "STATE_INIT"; break;
        case VMain_ShaderCore::STATE_FETCH: return "STATE_FETCH"; break;
        case VMain_ShaderCore::STATE_FETCH2: return "STATE_FETCH2"; break;
        case VMain_ShaderCore::STATE_DECODE: return "STATE_DECODE"; break;
        case VMain_ShaderCore::STATE_REGISTERS: return "STATE_REGISTERS"; break;
        case VMain_ShaderCore::STATE_ALU: return "STATE_ALU"; break;
        case VMain_ShaderCore::STATE_RETIRE: return "STATE_RETIRE"; break;
        case VMain_ShaderCore::STATE_LOAD: return "STATE_LOAD"; break;
        case VMain_ShaderCore::STATE_LOAD2: return "STATE_LOAD2"; break;
        case VMain_ShaderCore::STATE_STORE: return "STATE_STORE"; break;
        case VMain_ShaderCore::STATE_HALTED: return "STATE_HALTED"; break;
        case VMain_ShaderCore::STATE_FPU1: return "STATE_FPU1"; break;
        case VMain_ShaderCore::STATE_FPU2: return "STATE_FPU2"; break;
        case VMain_ShaderCore::STATE_FPU3: return "STATE_FPU3"; break;
        case VMain_ShaderCore::STATE_FPU4: return "STATE_FPU4"; break;
        default : return "unknown state"; break;
    }
}

void printDecodedInst(uint32_t address, uint32_t inst, const VMain_ShaderCore *core)
{
    if(core->decode_opcode_is_ALU_reg_imm)  {
        printf("0x%08X : 0x%08X - opcode_is_ALU_reg_imm %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_imm_alu_load);
    } else if(core->decode_opcode_is_branch) {
        printf("0x%08X : 0x%08X - opcode_is_branch cmp %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rs1, core->decode_rs2, core->decode_imm_branch);
    } else if(core->decode_opcode_is_ALU_reg_reg) {
        printf("0x%08X : 0x%08X - opcode_is_ALU_reg_reg op %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_jal) {
        printf("0x%08X : 0x%08X - opcode_is_jal %d, %d\n", address, inst, core->decode_rd, core->decode_imm_jump);
    } else if(core->decode_opcode_is_jalr) {
        printf("0x%08X : 0x%08X - opcode_is_jalr %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_imm_jump);
    } else if(core->decode_opcode_is_lui) {
        printf("0x%08X : 0x%08X - opcode_is_lui %d, %d\n", address, inst, core->decode_rd, core->decode_imm_upper);
    } else if(core->decode_opcode_is_auipc) {
        printf("0x%08X : 0x%08X - opcode_is_auipc %d, %d\n", address, inst, core->decode_rd, core->decode_imm_upper);
    } else if(core->decode_opcode_is_load) {
        printf("0x%08X : 0x%08X - opcode_is_load size %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_imm_alu_load);
    } else if(core->decode_opcode_is_store) {
        printf("0x%08X : 0x%08X - opcode_is_store size %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rs1, core->decode_rs2, core->decode_imm_store);
    } else if(core->decode_opcode_is_system) {
        printf("0x%08X : 0x%08X - opcode_is_system %d\n", address, inst, core->decode_imm_alu_load);
    } else if(core->decode_opcode_is_fadd) {
        printf("0x%08X : 0x%08X - opcode_is_fadd rm %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fsub) {
        printf("0x%08X : 0x%08X - opcode_is_fsub rm %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fmul) {
        printf("0x%08X : 0x%08X - opcode_is_fmul rm %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fdiv) {
        printf("0x%08X : 0x%08X - opcode_is_fdiv rm %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fsgnj) {
        printf("0x%08X : 0x%08X - opcode_is_fsgnj mode %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fminmax) {
        printf("0x%08X : 0x%08X - opcode_is_fminmax mode %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fsqrt) {
        printf("0x%08X : 0x%08X - opcode_is_fsqrt rm %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1);
    } else if(core->decode_opcode_is_fcmp) {
        printf("0x%08X : 0x%08X - opcode_is_fcmp cmp %d, %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1, core->decode_rs2);
    } else if(core->decode_opcode_is_fcvt_f2i) {
        printf("0x%08X : 0x%08X - opcode_is_f2i type %d, rm %d, %d, %d\n", address, inst, core->decode_shamt_ftype, core->decode_funct3_rm, core->decode_rd, core->decode_rs1);
    } else if(core->decode_opcode_is_fmv_f2i) {
        printf("0x%08X : 0x%08X - opcode_is_fmv_f2i funct3 %d, %d, %d\n", address, inst, core->decode_funct3_rm, core->decode_rd, core->decode_rs1);
    } else if(core->decode_opcode_is_fcvt_i2f) {
        printf("0x%08X : 0x%08X - opcode_is_fcvt_i2f type %d, rm %d, %d, %d\n", address, inst, core->decode_shamt_ftype, core->decode_funct3_rm, core->decode_rd, core->decode_rs1);
    } else if(core->decode_opcode_is_fmv_i2f) {
        printf("0x%08X : 0x%08X - opcode_is_fmv_i2f %d, %d\n", address, inst, core->decode_rd, core->decode_rs1);
    } else if(core->decode_opcode_is_flw) {
        printf("0x%08X : 0x%08X - opcode_is_flw %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_imm_alu_load);
    } else if(core->decode_opcode_is_fsw) {
        printf("0x%08X : 0x%08X - opcode_is_fsw %d, %d, %d\n", address, inst, core->decode_rs1, core->decode_rs2, core->decode_imm_store);
    } else if(core->decode_opcode_is_fmadd) {
        printf("0x%08X : 0x%08X - opcode_is_fmadd %d, %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_rs2, core->decode_rs3);
    } else if(core->decode_opcode_is_fmsub) {
        printf("0x%08X : 0x%08X - opcode_is_fmsub %d, %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_rs2, core->decode_rs3);
    } else if(core->decode_opcode_is_fnmsub) {
        printf("0x%08X : 0x%08X - opcode_is_fnmsub %d, %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_rs2, core->decode_rs3);
    } else if(core->decode_opcode_is_fnmadd) {
        printf("0x%08X : 0x%08X - opcode_is_fnmadd %d, %d, %d, %d\n", address, inst, core->decode_rd, core->decode_rs1, core->decode_rs2, core->decode_rs3);
    } else {
        printf("0x%08X : 0x%08X - undecoded instruction\n", address, inst);
    }
}

// Union for converting from float to int and back.
union FloatUint32 {
    float f;
    uint32_t i;
};

// Bit-wise conversion from int to float.
float intToFloat(uint32_t i) {
    FloatUint32 u;
    u.i = i;
    return u.f;
}

// Bit-wise conversion from float to int.
uint32_t floatToInt(float f) {
    FloatUint32 u;
    u.f = f;
    return u.i;
}

void dumpRegisters(VMain* top)
{
    // Dump register contents.
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
            << to_hex(top->Main->shaderCore->registers->bank1->memory[r]) << "   ";
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        if(false) {
            std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
                << to_hex(top->Main->shaderCore->float_registers->bank1->memory[r]) << "   ";
        } else {
            std::cout << (r < 10 ? " " : "") << "f" << r << " = ";
            std::cout << std::setprecision(5) << std::setw(10) <<
                intToFloat(top->Main->shaderCore->float_registers->bank1->memory[r]);
            std::cout << std::setw(0) << "   ";
        }
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    std::cout << " pc = 0x" << to_hex(top->Main->shaderCore->PC) << "\n";
}

void writeWordToRam(uint32_t word, uint32_t byteaddr, bool inst_not_data, VMain *top)
{
    top->ext_write_address = byteaddr;
    top->ext_write_data = word;

    if(inst_not_data) {
        top->ext_enable_write_inst = 1;
    } else {
        top->ext_enable_write_data = 1;
    }

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();

    if(inst_not_data) {
        top->ext_enable_write_inst = 0;
    } else {
        top->ext_enable_write_data = 0;
    }

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
}

uint32_t readWordFromRam(uint32_t byteaddr, bool inst_not_data, VMain *top)
{
    // TODO: use a state machine through Main to read memory using clock
    if(inst_not_data)
        return top->Main->instRam->memory[byteaddr / 4];
    else
        return top->Main->dataRam->memory[byteaddr / 4];
}

void writeBytesToRam(const std::vector<uint8_t>& bytes, bool inst_not_data, bool beVerbose, VMain *top)
{
    // Write inst bytes to inst memory
    for(uint32_t byteaddr = 0; byteaddr < bytes.size(); byteaddr += 4) {
        uint32_t word = 
            bytes[byteaddr + 0] <<  0 |
            bytes[byteaddr + 1] <<  8 |
            bytes[byteaddr + 2] << 16 |
            bytes[byteaddr + 3] << 24;

        writeWordToRam(word, byteaddr, inst_not_data, top);
        if(beVerbose)
            std::cout << "Writing to " << (inst_not_data ? "inst" : "data") << " address " << byteaddr << " value 0x" << to_hex(top->ext_write_data) << "\n";

    }
}

template <class TYPE>
void set(VMain *top, uint32_t address, const TYPE& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    writeWordToRam(*reinterpret_cast<const uint32_t*>(&value), address, false, top);
}

template <class TYPE, unsigned long N>
void set(VMain *top, uint32_t address, const std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        writeWordToRam(*reinterpret_cast<const uint32_t*>(&value), address + i * sizeof(uint32_t), false, top);
}

template <class TYPE, unsigned long N>
void get(VMain *top, uint32_t address, std::array<TYPE, N>& value)
{
    // TODO: use a state machine through Main to read memory using clock
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        *reinterpret_cast<uint32_t*>(&value[i]) = readWordFromRam(address + i * sizeof(uint32_t), false, top);
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
    printf("\t-j N      Use N threads [%d]\n",
            int(std::thread::hardware_concurrency()));
}

struct CoreShared
{
    bool coreHadAnException = false; // Only set to true after initialization
    unsigned char *img;
    std::mutex rendererMutex;

    // Number of rows still left to shade (for progress report).
    std::atomic_int rowsLeft;

    // These require exclusion using rendererMutex
    uint64_t clocksCount = 0;
    uint64_t dispatchedCount = 0;
};

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
    bool printDisassembly = false;
    bool printMemoryAccess = false;
    bool printCoreDiff = false;
    bool beVerbose = false;
};

// Thread to show progress to the user.
void showProgress(const CoreParameters* params, CoreShared* shared, std::chrono::time_point<std::chrono::steady_clock> startTime)
{
    int totalRows = params->afterLastY - params->startY;

    while(true) {
        int left = shared->rowsLeft;
        if (left == 0) {
            break;
        }

        std::cout << left << " rows left of " << totalRows;

        // Estimate time left.
        if (left != totalRows) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedTime = now - startTime;
            auto elapsedSeconds = double(elapsedTime.count())*
                std::chrono::steady_clock::period::num/
                std::chrono::steady_clock::period::den;
            auto secondsLeft = elapsedSeconds*left/(totalRows - left);

            std::cout << " (" << int(secondsLeft) << " seconds left)   ";
        }
        std::cout << "\r";
        std::cout.flush();

        // Wait one second while polling.
        for (int i = 0; i < 100 && shared->rowsLeft > 0; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

void shadeOnePixel(const SimDebugOptions* debugOptions, const CoreParameters *params, VMain *top, uint32_t *clocks, uint32_t *insts)
{
    // Run one clock cycle in reset to process STATE_INIT
    top->reset_n = 0;
    top->run = 0;

    top->ext_enable_write_inst = 0;
    top->ext_enable_write_data = 0;

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    (*clocks)++;

    // Release reset
    top->reset_n = 1;

    // run == 0, nothing should be happening here.
    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    (*clocks)++;

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->inst_bytes, true, debugOptions->beVerbose, top);

    writeBytesToRam(params->data_bytes, false, debugOptions->beVerbose, top);

    // check first 4 words written to inst memory
    for(uint32_t byteaddr = 0; byteaddr < 16; byteaddr += 4) {
        uint32_t inst_word = 
            params->inst_bytes[byteaddr + 0] <<  0 |
            params->inst_bytes[byteaddr + 1] <<  8 |
            params->inst_bytes[byteaddr + 2] << 16 |
            params->inst_bytes[byteaddr + 3] << 24;
        if(top->Main->instRam->memory[byteaddr >> 2] != inst_word) {
            std::cout << "error: inst memory[" << byteaddr << "] = "
                << to_hex(top->Main->instRam->memory[byteaddr >> 2]) << ", should be "
                << to_hex(inst_word) << "\n";
        }
    }

    // check first 4 words written to data memory
    for(uint32_t byteaddr = 0; byteaddr < 16 && byteaddr < params->data_bytes.size(); byteaddr += 4) {
        uint32_t data_word = 
            params->data_bytes[byteaddr + 0] <<  0 |
            params->data_bytes[byteaddr + 1] <<  8 |
            params->data_bytes[byteaddr + 2] << 16 |
            params->data_bytes[byteaddr + 3] << 24;
        if(top->Main->dataRam->memory[byteaddr >> 2] != data_word) {
            std::cout << "error: data memory[" << byteaddr << "] = "
                << to_hex(top->Main->dataRam->memory[byteaddr >> 2]) << ", should be "
                << to_hex(data_word) << "\n";
        }
    }

    // Run
    top->run = 1;

    std::string pad = "                     ";

    while (!Verilated::gotFinish() && !top->halted) {

        top->clock = 1;
        top->eval();

        if(false) {
            // left side of nonblocking assignments
            std::cout << pad << "between clock 1 and clock 0\n";
            std::cout << pad << "CPU in state " << stateToString(top->Main->shaderCore->state) << " (" << int(top->Main->shaderCore->state) << ")\n";
            std::cout << pad << "pc = 0x" << to_hex(top->Main->shaderCore->PC) << "\n";
            std::cout << pad << "inst_ram_address = 0x" << to_hex(top->Main->shaderCore->inst_ram_address) << "\n";
            std::cout << pad << "inst_to_decode = 0x" << to_hex(top->Main->shaderCore->inst_to_decode) << "\n";
            std::cout << pad << "data_ram_write_data = 0x" << to_hex(top->Main->shaderCore->data_ram_write_data) << "\n";
            std::cout << pad << "data_ram_address = 0x" << to_hex(top->Main->shaderCore->data_ram_address) << "\n";
            std::cout << pad << "data_ram_read_result = 0x" << to_hex(top->Main->shaderCore->data_ram_read_result) << "\n";
            std::cout << pad << "data_ram_write = 0x" << to_hex(top->Main->shaderCore->data_ram_write) << "\n";
        }

        top->clock = 0;
        top->eval();

        if(debugOptions->beVerbose) {

            dumpRegisters(top);
        }

        if(debugOptions->beVerbose) {
            // right side of nonblocking assignments
            std::cout << "between clock 0 and clock 1\n";
            std::cout << "CPU in state " << stateToString(top->Main->shaderCore->state) << " (" << int(top->Main->shaderCore->state) << ")\n";
            if(false) {
                std::cout << "inst_ram_address = 0x" << to_hex(top->Main->shaderCore->inst_ram_address) << "\n";
                std::cout << "inst_to_decode = 0x" << to_hex(top->Main->shaderCore->inst_to_decode) << "\n";
            }
        }

        if(top->Main->shaderCore->state == VMain_ShaderCore::STATE_RETIRE) {
            (*insts)++;
        }

        if(debugOptions->beVerbose && (top->Main->shaderCore->state == VMain_ShaderCore::STATE_ALU)) {
            std::cout << "after DECODE - ";
            printDecodedInst(top->Main->shaderCore->PC, top->Main->shaderCore->inst_to_decode, top->Main->shaderCore);
        }

        if(debugOptions->beVerbose) {
            std::cout << "---\n";
        }
        (*clocks)++;
    }

    if(debugOptions->beVerbose) {
        std::cout << "halted.\n";

        dumpRegisters(top);

        // Dump contents of beginning of memory
        for (int i = 0; i < 16; i++) {
            // Draw in columns.
            bool start = (i % 4 == 0);
            bool end = (i % 4 == 3);
            if(start) {
                std::cout << "0x" << to_hex(i) << " :";
            }

            std::cout << " " << to_hex(top->Main->dataRam->memory[i]) << "   ";
            if (end) {
                std::cout << "\n";
            }
        }
    }
}

void render(const SimDebugOptions* debugOptions, const CoreParameters* params, CoreShared* shared, int start_row, int skip_rows)
{
    VMain *top = new VMain;

    uint32_t clocks = 0;
    uint32_t insts = 0;

    const float fw = params->imageWidth;
    const float fh = params->imageHeight;
    const float one = 1.0;

    if (params->data_symbols.find(".anonymous") != params->data_symbols.end()) {
        set(top, params->data_symbols.find(".anonymous")->second, v3float{fw, fh, one});
        set(top, params->data_symbols.find(".anonymous")->second + sizeof(v3float), params->frameTime);
    }

    for(int j = start_row; j < params->afterLastY; j += skip_rows) {
        for(int i = params->startX; i < params->afterLastX; i++) {

            if(params->gl_FragCoordAddress != 0xFFFFFFFF)
                set(top, params->gl_FragCoordAddress, v4float{i + 0.5f, j + 0.5f, 0, 0});
            if(params->colorAddress != 0xFFFFFFFF)
                set(top, params->colorAddress, v4float{1, 1, 1, 1});
            if(false) // XXX TODO: when iTime is exported by compilation
                set(top, params->iTimeAddress, params->frameTime);

            // core.regs.pc = params->initialPC; // XXX Ignored

            if(debugOptions->printDisassembly) {
                std::cout << "; pixel " << i << ", " << j << '\n';
            }

            shadeOnePixel(debugOptions, params, top, &clocks, &insts);

            v3float rgb = {1, 0, 0};
            if(params->colorAddress != 0xFFFFFFFF)
                get(top, params->colorAddress, rgb);

            int pixelOffset = 3 * ((params->imageHeight - 1 - j) * params->imageWidth + i);
            for(int c = 0; c < 3; c++)
                shared->img[pixelOffset + c] = std::clamp(int(rgb[c] * 255.99), 0, 255);
        }
        shared->rowsLeft --;
    }
    {
        std::scoped_lock l(shared->rendererMutex);
        shared->dispatchedCount += insts;
        shared->clocksCount += clocks;
    }

    top->final();
    delete top;
}



int main(int argc, char **argv)
{
    bool imageToTerminal = false;
    bool printSymbols = false;
    bool printSubstitutions = false;
    bool printHeaderInfo = false;
    int specificPixelX = -1;
    int specificPixelY = -1;
    int threadCount = std::thread::hardware_concurrency();

    SimDebugOptions debugOptions;
    CoreParameters params;
    CoreShared shared;

    debugOptions.beVerbose = false;

    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            debugOptions.printMemoryAccess = true;
            debugOptions.beVerbose = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--pixel") == 0) {

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

        } else if(strcmp(argv[0], "--diff") == 0) {

            debugOptions.printCoreDiff = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-j") == 0) {

            if(argc < 2) {
                usage(progname);
                exit(EXIT_FAILURE);
            }
            threadCount = atoi(argv[1]);
            argv += 2; argc -= 2;

        } else if(strcmp(argv[0], "--term") == 0) {

            imageToTerminal = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "-S") == 0) {

            debugOptions.printDisassembly = true;
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
        for(auto& [symbol, address]: params.text_symbols) {
            std::cout << "text segment symbol \"" << symbol << "\" is at " << address << "\n";
        }
        for(auto& [symbol, address]: params.data_symbols) {
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

    for(auto& [symbol, address]: params.text_symbols)
        params.textAddressesToSymbols[address] = symbol;

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

    std::cout << "Using " << threadCount << " threads.\n";

    std::vector<std::thread *> thread;

    for (int t = 0; t < threadCount; t++) {
        thread.push_back(new std::thread(render, &debugOptions, &params, &shared, params.startY + t, threadCount));
    }

    // Progress information.
    Timer frameElapsed;
    shared.rowsLeft = params.afterLastY - params.startY;
    thread.push_back(new std::thread(showProgress, &params, &shared, frameElapsed.startTime()));

    // Wait for worker threads to quit.
    while(!thread.empty()) {
        std::thread* td = thread.back();
        thread.pop_back();
        td->join();
    }

    if(shared.coreHadAnException) {
        exit(EXIT_FAILURE);
    }

    std::cout << "shading took " << frameElapsed.elapsed() << " seconds.\n";
    std::cout << shared.dispatchedCount << " instructions executed.\n";
    std::cout << shared.clocksCount << " clocks cycled.\n";
    float fps = 50000000.0f / shared.clocksCount;
    std::cout << fps << " fps estimated at 50 MHz.\n";
    std::cout << "at least " << (int)ceilf(5.0 / fps) << " cores required at 50 MHz for 5 fps.\n";

    FILE *fp = fopen("emulated.ppm", "wb");
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

