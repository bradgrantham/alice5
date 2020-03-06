#include <cmath>
#include <cstdlib>
#include <fstream>
#include <inttypes.h>
#include <iomanip>
#include <iostream>
#include <chrono>

constexpr bool dumpH2FAndF2H = false; // true;

#include "util.h"
#include "corecomm.h"
#include "hal.h"
#include "memory.h"

#include "VMain.h"
#include "VMain_Main.h"
#include "VMain_GPU.h"
#include "VMain_GPU32BitInterface.h"
#include "VMain_Registers.h"
#include "VMain_ShaderCore.h"
#include "VMain_RegisterFile__A5.h"
#include "VMain_BlockRam__A10.h"

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
        case VMain_ShaderCore::STATE_FP_WAIT: return "STATE_FP_WAIT"; break;
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

using namespace std::chrono_literals;

// HAL specifically for sim.
class SimHal : public Hal {
    VMain *mTop;
    uint32_t mClockCount;

    static Memory *gSdram;

public:
    SimHal(VMain *top) : mTop(top), mClockCount(0) {
        if(!gSdram) {
            gSdram = new Memory(SDRAM_BASE, SDRAM_SIZE);
        }

        mTop->reset_n = 0;
        allowGpuProgress();
        allowGpuProgress();
        mTop->reset_n = 1;
    }

    virtual ~SimHal() {
        mTop->final();
        delete mTop;
    }

    virtual void setH2F(uint32_t value, int coreNumber) {
        if(dumpH2FAndF2H) {
            std::cout << "set h2f_value[" << coreNumber << "] to " << to_hex(value) << "\n";
        }
        mTop->h2f_value[coreNumber] = value;
    }

    virtual uint32_t getF2H(int coreNumber) {
        uint32_t f2h_value = mTop->f2h_value[coreNumber];
        if(dumpH2FAndF2H) {
            std::cout << "get f2h_value[" << coreNumber << "] yields " << to_hex(f2h_value) << "\n";
        }
        return f2h_value;
    }

    virtual void allowGpuProgress() {

        // cycle simulation clock
        mTop->clock = 1;
        mTop->eval();

        // RAM.
        gSdram->evalReadWrite(
                mTop->sdram_read,
                mTop->sdram_write,
                mTop->sdram_address,
                mTop->sdram_waitrequest,
                mTop->sdram_readdatavalid,
                mTop->sdram_readdata,
                0xF,
                mTop->sdram_writedata);

        // Eval again immediately to update dependant wires.
        mTop->eval();
        mTop->clock = 0;
        mTop->eval();
        mClockCount++;
    }

    uint32_t getClockCount() const {
        return mClockCount;
    }

    virtual int getCoreCount() {
        return 4; // XXX probe from hardware
    }

    static uint32_t readSdram(uint32_t address)
    {
        return (*gSdram)[address];
    }
};

Memory *SimHal::gSdram;

// True if only one Instance can be created.
bool HalCanCreateMultipleInstances = true;

Hal *HalCreateInstance()
{
    VMain *top = new VMain;

    SimHal *hw = new SimHal{top};

    return hw;
}

uint32_t HalReadMemory(uint32_t address)
{
    return SimHal::readSdram(address);
}

VMain_GPU *getGpuByCore(VMain *top, int coreNumber) {
    switch (coreNumber) {
        case 0: return top->Main->genblk1__BRA__0__KET____DOT__gpu;
        case 1: return top->Main->genblk1__BRA__1__KET____DOT__gpu;
        case 2: return top->Main->genblk1__BRA__2__KET____DOT__gpu;
        case 3: return top->Main->genblk1__BRA__3__KET____DOT__gpu;
        default: throw std::runtime_error("getGpuByCore");
    }
}

void dumpRegisters(VMain_GPU *gpuCore)
{
    // Dump register contents.
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
            << to_hex(gpuCore->shaderCore->registers->bank1->memory[r]) << "   ";
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    for (int i = 0; i < 32; i++) {
        // Draw in columns.
        int r = i%4*8 + i/4;
        if(false) {
            std::cout << (r < 10 ? " " : "") << "x" << r << " = 0x"
                << to_hex(gpuCore->shaderCore->float_registers->bank1->memory[r]) << "   ";
        } else {
            std::cout << (r < 10 ? " " : "") << "f" << r << " = ";
            std::cout << std::setprecision(5) << std::setw(10) <<
                intToFloat(gpuCore->shaderCore->float_registers->bank1->memory[r]);
            std::cout << std::setw(0) << "   ";
        }
        if (i % 4 == 3) {
            std::cout << "\n";
        }
    }
    std::cout << " pc = 0x" << to_hex(gpuCore->shaderCore->PC) << "\n";
}

void dumpRegsDiff(const uint32_t prevPC, const uint32_t prevX[32], const uint32_t prevF[32], VMain_GPU *gpuCore)
{
    std::cout << std::setfill('0');
    if(prevPC != gpuCore->shaderCore->PC) {
        std::cout << "pc changed to " << std::hex << std::setw(8) << gpuCore->shaderCore->PC << std::dec << '\n';
    }
    for(int i = 0; i < 32; i++) {
        if(prevX[i] != gpuCore->shaderCore->registers->bank1->memory[i]) {
            std::cout << "x" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << gpuCore->shaderCore->registers->bank1->memory[i] << std::dec << '\n';
        }
    }
    for(int i = 0; i < 32; i++) {
        float cur = intToFloat(gpuCore->shaderCore->float_registers->bank1->memory[i]);
        float prev = intToFloat(prevF[i]);
        bool bothnan = std::isnan(cur) && std::isnan(prev);
        if((prev != cur) && !bothnan) { // if both NaN, equality test still fails)
            uint32_t x = floatToInt(cur);
            std::cout << "f" << std::setw(2) << i << " changed to " << std::hex << std::setw(8) << x << std::dec << "(" << cur << ")\n";
        }
    }
    std::cout << std::setfill(' ');
}

std::string HalPreferredName()
{
    return "simulated";
}
