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

#define SIMULATE 1
constexpr bool dumpH2FAndF2H = false; // true;

#include "risc-v.h"
#include "timer.h"
#include "disassemble.h"

#include "objectfile.h"

#include "VMain.h"
#include "VMain_Main.h"
#include "VMain_GPU.h"
#include "VMain_Registers.h"
#include "VMain_ShaderCore.h"
#include "VMain_RegisterFile__A5.h"
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
        case VMain_ShaderCore::STATE_EXECUTE: return "STATE_EXECUTE"; break;
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

void setH2F(VMain* top, uint32_t state)
{
    if(dumpH2FAndF2H) {
        std::cout << "set sim_h2f_value to " << to_hex(state) << "\n";
    }
    top->sim_h2f_value = state;
}

uint32_t getF2H(VMain* top)
{
    uint32_t f2h_value = top->sim_f2h_value;
    if(dumpH2FAndF2H) {
        std::cout << "get sim_f2h_value yields " << to_hex(f2h_value) << "\n";
    }
    return f2h_value;
}

void allowGPUProgress(VMain *top)
{

#if SIMULATE

    // cycle simulation clock
    top->clock = 1;
    top->eval();

    if(dumpH2FAndF2H) {
        std::cout << "CLOCK = 0, state " << int(top->Main->state) << "\n";
        std::cout << "h2f : " << to_hex(top->sim_h2f_value) << ", " << cmdToString(top->sim_h2f_value) << "\n";
        std::cout << "cmd_parameter = " << to_hex(top->Main->cmd_parameter) << "\n";
        std::cout << "write register = " << to_hex((top->Main->write_register_high16 << 16) | top->Main->write_register_low16) << "\n";
        std::cout << "ext_enable_write_inst = " << (top->Main->ext_enable_write_inst ? "true" : "false") << "\n";
    }

    top->clock = 0;
    top->eval();

    if(dumpH2FAndF2H) {
        std::cout << "CLOCK = 0, state " << int(top->Main->state) << "\n";
        std::cout << "h2f : " << to_hex(top->sim_h2f_value) << ", " << cmdToString(top->sim_h2f_value) << "\n";
        std::cout << "cmd_parameter = " << to_hex(top->Main->cmd_parameter) << "\n";
        std::cout << "write register = " << to_hex((top->Main->write_register_high16 << 16) | top->Main->write_register_low16) << "\n";
        std::cout << "ext_enable_write_inst = " << (top->Main->ext_enable_write_inst ? "true" : "false") << "\n";
    }

#else

    // nanosleep
    std::this_thread::sleep_for(1us);

#endif

}

bool waitOnExitReset(VMain* top)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnExitReset()...\n";
    }

    allowGPUProgress(top);

    auto start = std::chrono::high_resolution_clock::now();
    while((getF2H(top) & f2h_exited_reset_bit) != f2h_exited_reset_bit) {

        allowGPUProgress(top);

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

bool waitOnPhaseOrTimeout(int phase, VMain* top)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnPhaseOrTimeout(" << to_hex(phase) << ")...\n";
    }
    uint32_t phaseExpected = phase ? f2h_busy_bit : 0;

    allowGPUProgress(top);

    auto start = std::chrono::high_resolution_clock::now();
    while((getF2H(top) & f2h_busy_bit) != phaseExpected) {

        allowGPUProgress(top);

        // Check clock for timeout
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout_micros) {
            std::cerr << "waitOnPhaseOrTimeout : timed out waiting for FPGA to change phase (" << elapsed.count() << ")\n";
            return false;
        }
    }
    return true;
}

bool processCommand(uint32_t command, VMain* top)
{
    // double-check we are in idle
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, top)) {
        std::cerr << "processCommand : timeout waiting on FPGA to become idle before sending command\n";
        return false;
    }
    
    // send the command and then wait for core to report it is processing the command
    setH2F(top, command);
    if(!waitOnPhaseOrTimeout(f2h_phase_busy, top)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate busy processing command\n";
        return false;
    }

    // return to idle state and wait for the core to tell us it's also idle.
    setH2F(top, h2f_gpu_idle);
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, top)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate command is completed\n";
        return false;
    }

    return true;
}

enum RamType { INST_RAM, DATA_RAM };

void CHECK(bool success, const char *filename, int line)
{
    bool stored_exit_flag = false;
    bool exit_on_error;

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

void writeWordToRam(uint32_t value, uint32_t address, RamType ramType, VMain *top)
{
    // put low 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_low_16 | (value & 0xFFFF), top), __FILE__, __LINE__);

    // put high 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_high_16 | ((value >> 16) & 0xFFFF), top), __FILE__, __LINE__);

    // send the command to store the word from the write register into memory
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_write_inst_ram | (address & 0xFFFF), top), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_write_data_ram | (address & 0xFFFF), top), __FILE__, __LINE__);
    }
}

uint32_t readWordFromRam(uint32_t address, RamType ramType, VMain *top)
{
    uint32_t value;

    // send the command to latch the word from memory into the read register
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_read_inst_ram | (address & 0xFFFF), top), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_data_ram | (address & 0xFFFF), top), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, top), __FILE__, __LINE__);

    // get low 16 bits into value
    value = getF2H(top) & 0xFFFF;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, top), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((getF2H(top) & 0xFFFF) << 16);

    return value;
}

enum RegisterKind {X_REG, F_REG, PC_REG};

uint32_t readReg(RegisterKind kind, int reg, VMain* top)
{
    uint32_t value;

    // send the command to latch the register into the read register
    if(kind == X_REG) {
        CHECK(processCommand(h2f_cmd_read_x_reg | (reg & 0x1F), top), __FILE__, __LINE__);
    } else if(kind == PC_REG) {
        CHECK(processCommand(h2f_cmd_read_special | h2f_special_PC, top), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_f_reg | (reg & 0x1F), top), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, top), __FILE__, __LINE__);

    // get low 16 bits into value
    value = getF2H(top) & f2h_cmd_response_mask;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, top), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((getF2H(top) & f2h_cmd_response_mask) << 16);

    return value;
}

uint32_t readPC(VMain* top)
{
    return readReg(PC_REG, 0, top);
}


void dumpRegisters(VMain* top)
{
    // Dump register contents.
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
            << to_hex(top->Main->gpu->shaderCore->registers->bank1->memory[r]) << "   ";
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        if(false) {
            std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
                << to_hex(top->Main->gpu->shaderCore->float_registers->bank1->memory[r]) << "   ";
        } else {
            std::cout << (r < 10 ? " " : "") << "f" << r << " = ";
            std::cout << std::setprecision(5) << std::setw(10) <<
                intToFloat(top->Main->gpu->shaderCore->float_registers->bank1->memory[r]);
            std::cout << std::setw(0) << "   ";
        }
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    std::cout << " pc = 0x" << to_hex(top->Main->gpu->shaderCore->PC) << "\n";
}

void writeBytesToRam(const std::vector<uint8_t>& bytes, RamType ramType, bool dumpState, VMain *top)
{
    // Write inst bytes to inst memory
    for(uint32_t byteaddr = 0; byteaddr < bytes.size(); byteaddr += 4) {
        uint32_t word = 
            bytes[byteaddr + 0] <<  0 |
            bytes[byteaddr + 1] <<  8 |
            bytes[byteaddr + 2] << 16 |
            bytes[byteaddr + 3] << 24;

        if(dumpState)
            std::cout << "Writing to " << ((ramType == INST_RAM) ? "inst" : "data") << " address " << byteaddr << " value 0x" << to_hex((top->Main->write_register_high16 << 16) | top->Main->write_register_low16) << "\n";

        writeWordToRam(word, byteaddr, ramType, top);

    }
}

template <class TYPE>
void set(VMain *top, uint32_t address, const TYPE& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    writeWordToRam(*reinterpret_cast<const uint32_t*>(&value), address, DATA_RAM, top);
}

template <class TYPE, unsigned long N>
void set(VMain *top, uint32_t address, const std::array<TYPE, N>& value)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        writeWordToRam(*reinterpret_cast<const uint32_t*>(&value[i]), address + i * sizeof(uint32_t), DATA_RAM, top);
}

template <class TYPE, unsigned long N>
void get(VMain *top, uint32_t address, std::array<TYPE, N>& value)
{
    // TODO: use a state machine through Main to read memory using clock
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        *reinterpret_cast<uint32_t*>(&value[i]) = readWordFromRam(address + i * sizeof(uint32_t), DATA_RAM, top);
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

    // Number of pixels still left to shade (for progress report).
    std::atomic_int pixelsLeft;

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
    bool dumpState = false;
};

// Thread to show progress to the user.
void showProgress(const CoreParameters* params, CoreShared* shared, std::chrono::time_point<std::chrono::steady_clock> startTime, int totalPixels)
{
    while(true) {
        int pixelsLeft = shared->pixelsLeft;
        if (pixelsLeft == 0) {
            break;
        }

        printf("%.2f%% done", (totalPixels - pixelsLeft)*100.0/totalPixels);

        // Estimate time left.
        if (pixelsLeft != totalPixels) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedTime = now - startTime;
            auto elapsedSeconds = double(elapsedTime.count())*
                std::chrono::steady_clock::period::num/
                std::chrono::steady_clock::period::den;
            auto secondsLeft = int(elapsedSeconds*pixelsLeft/(totalPixels - pixelsLeft));

            int minutesLeft = secondsLeft/60;
            secondsLeft -= minutesLeft*60;
            printf(" (%d:%02d left)   ", minutesLeft, secondsLeft);
        }
        std::cout << "\r";
        std::cout.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Clear the line.
    std::cout << "                                                             \r";
}

void dumpRegsDiff(const uint32_t prevPC, const uint32_t prevX[32], const uint32_t prevF[32], VMain *top)
{
    std::cout << std::setfill('0');
    if(prevPC != top->Main->gpu->shaderCore->PC) {
        std::cout << "pc changed to " << std::hex << std::setw(8) << top->Main->gpu->shaderCore->PC << std::dec << '\n';
    }
    for(int i = 0; i < 32; i++) {
        if(prevX[i] != top->Main->gpu->shaderCore->registers->bank1->memory[i]) {
            std::cout << "x" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << top->Main->gpu->shaderCore->registers->bank1->memory[i] << std::dec << '\n';
        }
    }
    for(int i = 0; i < 32; i++) {
        float cur = intToFloat(top->Main->gpu->shaderCore->float_registers->bank1->memory[i]);
        float prev = intToFloat(prevF[i]);
        bool bothnan = std::isnan(cur) && std::isnan(prev);
        if((prev != cur) && !bothnan) { // if both NaN, equality test still fails)
            uint32_t x = floatToInt(cur);
            std::cout << "f" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << x << std::dec << "(" << cur << ")\n";
        }
    }
    std::cout << std::setfill(' ');
}


void shadeOnePixel(const SimDebugOptions* debugOptions, const CoreParameters *params, VMain *top, uint32_t *clocks, uint32_t *insts)
{
    // Run one clock cycle in not-run reset to process STATE_INIT
    setH2F(top, h2f_gpu_reset);

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    (*clocks)++;

    // Release reset
    setH2F(top, h2f_gpu_idle);
    waitOnExitReset(top);

    // run == 0, nothing should be happening here.
    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    (*clocks)++;
    // Run
    setH2F(top, h2f_gpu_run);

    std::string pad = "                     ";

    uint32_t oldXRegs[32];
    uint32_t oldFRegs[32];
    uint32_t oldPC;
    for(int i = 0; i < 32; i++) oldXRegs[i] = top->Main->gpu->shaderCore->registers->bank1->memory[i];
    for(int i = 0; i < 32; i++) oldFRegs[i] = top->Main->gpu->shaderCore->float_registers->bank1->memory[i];
    oldPC = top->Main->gpu->shaderCore->PC;

    while (!Verilated::gotFinish() && !(getF2H(top) & f2h_run_halted_bit)) {

        top->clock = 1;
        top->eval();

        if(false) {
            // left side of nonblocking assignments
            std::cout << pad << "between clock 1 and clock 0\n";
            std::cout << pad << "CPU in state " << stateToString(top->Main->gpu->shaderCore->state) << " (" << int(top->Main->gpu->shaderCore->state) << ")\n";
            std::cout << pad << "pc = 0x" << to_hex(top->Main->gpu->shaderCore->PC) << "\n";
            std::cout << pad << "inst_ram_address = 0x" << to_hex(top->Main->gpu->shaderCore->inst_ram_address) << "\n";
            std::cout << pad << "inst_to_decode = 0x" << to_hex(top->Main->gpu->shaderCore->inst_to_decode) << "\n";
            std::cout << pad << "data_ram_write_data = 0x" << to_hex(top->Main->gpu->shaderCore->data_ram_write_data) << "\n";
            std::cout << pad << "data_ram_address = 0x" << to_hex(top->Main->gpu->shaderCore->data_ram_address) << "\n";
            std::cout << pad << "data_ram_read_result = 0x" << to_hex(top->Main->gpu->shaderCore->data_ram_read_result) << "\n";
            std::cout << pad << "data_ram_write = 0x" << to_hex(top->Main->gpu->shaderCore->data_ram_write) << "\n";
        }

        top->clock = 0;
        top->eval();

        if(debugOptions->dumpState) {

            dumpRegisters(top);
        }

        if(debugOptions->dumpState) {
            // right side of nonblocking assignments
            std::cout << "between clock 0 and clock 1\n";
            std::cout << "CPU in state " << stateToString(top->Main->gpu->shaderCore->state) << " (" << int(top->Main->gpu->shaderCore->state) << ")\n";
            if(false) {
                std::cout << "inst_ram_address = 0x" << to_hex(top->Main->gpu->shaderCore->inst_ram_address) << "\n";
                std::cout << "inst_to_decode = 0x" << to_hex(top->Main->gpu->shaderCore->inst_to_decode) << "\n";
            }
        }

        if(top->Main->gpu->shaderCore->state == VMain_ShaderCore::STATE_RETIRE) {
            (*insts)++;
        }

        if((top->Main->gpu->shaderCore->state == VMain_ShaderCore::STATE_EXECUTE) ||
            (top->Main->gpu->shaderCore->state == VMain_ShaderCore::STATE_FPU1)
            ) {
            if(debugOptions->dumpState) {
                std::cout << "after DECODE - ";
                printDecodedInst(top->Main->gpu->shaderCore->PC, top->Main->gpu->shaderCore->inst_to_decode, top->Main->gpu->shaderCore);
            }
            if(debugOptions->printDisassembly) {
                print_inst(top->Main->gpu->shaderCore->PC, top->Main->gpu->instRam->memory[top->Main->gpu->shaderCore->PC / 4], params->textAddressesToSymbols);
            }
        }

        // if(top->Main->gpu->shaderCore->state == VMain_ShaderCore::STATE_FETCH) {
            if(debugOptions->printCoreDiff) {

                dumpRegsDiff(oldPC, oldXRegs, oldFRegs, top);

                for(int i = 0; i < 32; i++) oldXRegs[i] = top->Main->gpu->shaderCore->registers->bank1->memory[i];
                for(int i = 0; i < 32; i++) oldFRegs[i] = top->Main->gpu->shaderCore->float_registers->bank1->memory[i];
                oldPC = top->Main->gpu->shaderCore->PC;
            }
        // }

        if(debugOptions->dumpState) {
            std::cout << "---\n";
        }
        (*clocks)++;
    }

    if(debugOptions->dumpState) {
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

            std::cout << " " << to_hex(top->Main->gpu->dataRam->memory[i]) << "   ";
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

    // Set up inst Ram
    // Run one clock cycle in reset to process STATE_INIT
    setH2F(top, h2f_gpu_reset);

    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    clocks++;

    // Release reset
    setH2F(top, h2f_gpu_idle);
    waitOnExitReset(top);

    // run == 0, nothing should be happening here.
    top->clock = 1;
    top->eval();
    top->clock = 0;
    top->eval();
    clocks++;

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->inst_bytes, INST_RAM, debugOptions->dumpState, top);

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->data_bytes, DATA_RAM, debugOptions->dumpState, top);

    // check words written to inst memory
    for(uint32_t byteaddr = 0; byteaddr < params->inst_bytes.size(); byteaddr += 4) {
        uint32_t inst_word = 
            params->inst_bytes[byteaddr + 0] <<  0 |
            params->inst_bytes[byteaddr + 1] <<  8 |
            params->inst_bytes[byteaddr + 2] << 16 |
            params->inst_bytes[byteaddr + 3] << 24;
        if(top->Main->gpu->instRam->memory[byteaddr >> 2] != inst_word) {
            std::cout << "error: inst memory[" << byteaddr << "] = "
                << to_hex(top->Main->gpu->instRam->memory[byteaddr >> 2]) << ", should be "
                << to_hex(inst_word) << "\n";
        }
    }

    // check words written to data memory
    for(uint32_t byteaddr = 0; byteaddr < params->data_bytes.size(); byteaddr += 4) {
        uint32_t data_word = 
            params->data_bytes[byteaddr + 0] <<  0 |
            params->data_bytes[byteaddr + 1] <<  8 |
            params->data_bytes[byteaddr + 2] << 16 |
            params->data_bytes[byteaddr + 3] << 24;
        if(top->Main->gpu->dataRam->memory[byteaddr >> 2] != data_word) {
            std::cout << "error: data memory[" << byteaddr << "] = "
                << to_hex(top->Main->gpu->dataRam->memory[byteaddr >> 2]) << ", should be "
                << to_hex(data_word) << "\n";
        }
    }

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

            /* return to STATE_INIT so we can drive ext memory access */
            setH2F(top, h2f_gpu_idle);

            v3float rgb = {1, 0, 0};
            if(params->colorAddress != 0xFFFFFFFF)
                get(top, params->colorAddress, rgb);

            int pixelOffset = 3 * ((params->imageHeight - 1 - j) * params->imageWidth + i);
            for(int c = 0; c < 3; c++)
                shared->img[pixelOffset + c] = std::clamp(int(rgb[c] * 255.99), 0, 255);
            shared->pixelsLeft --;
        }
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

    debugOptions.dumpState = false;

    Verilated::commandArgs(argc, argv);
    Verilated::debug(1);

    char *progname = argv[0];
    argv++; argc--;

    while(argc > 0 && argv[0][0] == '-') {
        if(strcmp(argv[0], "-v") == 0) {

            debugOptions.printMemoryAccess = true;
            argv++; argc--;

        } else if(strcmp(argv[0], "--dump") == 0) {

            debugOptions.dumpState = true;
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

    int totalPixels = (params.afterLastY - params.startY)*(params.afterLastX - params.startX);
    shared.pixelsLeft = totalPixels;

    for (int t = 0; t < threadCount; t++) {
        thread.push_back(new std::thread(render, &debugOptions, &params, &shared, params.startY + t, threadCount));
    }

    // Progress information.
    Timer frameElapsed;
    thread.push_back(new std::thread(showProgress, &params, &shared,
                frameElapsed.startTime(), totalPixels));

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
    float wvgaFps = fps * params.imageWidth / 800 * params.imageHeight / 480;
    std::cout << "at least " << (int)ceilf(5.0 / wvgaFps) << " cores required at 50 MHz for 5 fps at 800x480.\n";

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

