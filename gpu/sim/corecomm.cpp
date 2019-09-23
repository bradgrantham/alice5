#include <iostream>
#include <chrono>
#include <thread>
#include <inttypes.h>


// Proposed H2F interface for testing a single core
const uint32_t h2f_reset_n_bit          = 0x80000000;
const uint32_t h2f_not_reset            = h2f_reset_n_bit;
const uint32_t h2f_run_bit              = 0x40000000;
const uint32_t h2f_request_bit          = 0x20000000;
const uint32_t h2f_cmd_put_low_16       = 0x00000000;
const uint32_t h2f_cmd_put_high_16      = 0x00010000;
const uint32_t h2f_cmd_write_inst_ram   = 0x00020000;
const uint32_t h2f_cmd_write_data_ram   = 0x00030000;
const uint32_t h2f_cmd_read_inst_ram    = 0x00040000;
const uint32_t h2f_cmd_read_data_ram    = 0x00050000;
const uint32_t h2f_cmd_read_x_reg       = 0x00060000;
const uint32_t h2f_cmd_read_f_reg       = 0x00070000;
const uint32_t h2f_cmd_read_special     = 0x00080000;
const uint32_t h2f_special_PC           = 0x00080000;
const uint32_t h2f_cmd_get_low_16       = 0x00090000;
const uint32_t h2f_cmd_get_high_16      = 0x00100000;

const uint32_t h2f_cmd_idle             = h2f_not_reset;
const uint32_t h2f_cmd_request          = h2f_not_reset | h2f_request_bit;
const uint32_t h2f_cmd_run              = h2f_not_reset | h2f_run_bit;

// Proposed F2H interface for testing a single core
const uint32_t f2h_run_halted_bit       = 0x80000000;
const uint32_t f2h_run_exception_bit    = 0x40000000;
const uint32_t f2h_run_exc_data_mask    = 0x00FFFFFF;
const uint32_t f2h_busy_bit             = 0x20000000;
const uint32_t f2h_phase_busy           = f2h_busy_bit;
const uint32_t f2h_phase_ready          = 0;
const uint32_t f2h_cmd_response_mask    = 0x0000FFFF;

uint32_t *h2f;
uint32_t *f2h;

using namespace std::chrono_literals;
#if SIMULATE
const std::chrono::duration<float, std::micro> h2f_timeout_micros = 1000us;
#else
const std::chrono::duration<float, std::micro> h2f_timeout_micros = 10us;
#endif

bool waitOnPhaseOrTimeout(int phase)
{

    uint32_t phaseExpected = phase ? f2h_busy_bit : 0;
    auto start = std::chrono::high_resolution_clock::now();
    while((*f2h & f2h_busy_bit) != phaseExpected) {
#if SIMULATE
        // cycle simulation clock
#else
        // nanosleep and check clock for timeout
        std::this_thread::sleep_for(1us);
#endif
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout_micros) {
            std::cerr << "waitOnPhaseOrTimeout : timeout waiting for FPGA to change phase\n";
            return false;
        }
    }
    return true;
}

bool processCommand(uint32_t command)
{
    // double-check we are in idle
    if(!waitOnPhaseOrTimeout(f2h_phase_ready)) {
        std::cerr << "processCommand : timeout waiting on FPGA to become idle before sending command\n";
        return false;
    }
    
    // send the command and then wait for core to report it is processing the command
    *h2f = command | h2f_not_reset | h2f_request_bit;
    if(!waitOnPhaseOrTimeout(f2h_phase_busy)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate busy processing command\n";
        return false;
    }

    // return to idle state and wait for the core to tell us it's also idle.
    *h2f = h2f_cmd_idle;
    if(!waitOnPhaseOrTimeout(f2h_phase_ready)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate command is completed\n";
        return false;
    }

    return true;
}

enum RamType { INST_RAM, DATA_RAM };

void writeWord(uint32_t address, RamType ramType, uint32_t value)
{
    // put low 16 bits into write register 
    processCommand(h2f_cmd_put_low_16 | (value & 0xFFFF));

    // put high 16 bits into write register 
    processCommand(h2f_cmd_put_high_16 | ((value >> 16) & 0xFFFF));

    // send the command to store the word from the write register into memory
    if(ramType == INST_RAM) {
        processCommand(h2f_cmd_write_inst_ram | (address & 0xFFFF));
    } else {
        processCommand(h2f_cmd_write_data_ram | (address & 0xFFFF));
    }
}

uint32_t readWord(uint32_t address, RamType ramType)
{
    uint32_t value;

    // send the command to latch the word from memory into the read register
    if(ramType == INST_RAM) {
        processCommand(h2f_cmd_read_inst_ram | (address & 0xFFFF));
    } else {
        processCommand(h2f_cmd_read_data_ram | (address & 0xFFFF));
    }

    // put low 16 bits of read register on low 16 bits of F2H
    processCommand(h2f_cmd_get_low_16);

    // get low 16 bits into value
    value = *f2h & 0xFFFF;

    // put high 16 bits of read register on low 16 bits of F2H
    processCommand(h2f_cmd_get_high_16);

    // get high 16 bits into value
    value = value | ((*f2h & 0xFFFF) << 16);

    return value;
}

enum RegisterKind {X_REG, F_REG, PC_REG};

uint32_t readReg(RegisterKind kind, int reg)
{
    uint32_t value;

    // send the command to latch the register into the read register
    if(kind == X_REG) {
        processCommand(h2f_cmd_read_x_reg | (reg & 0x1F));
    } else if(kind == PC_REG) {
        processCommand(h2f_cmd_read_special | h2f_special_PC);
    } else {
        processCommand(h2f_cmd_read_f_reg | (reg & 0x1F));
    }

    // put low 16 bits of read register on low 16 bits of F2H
    processCommand(h2f_cmd_get_low_16);

    // get low 16 bits into value
    value = *f2h & f2h_cmd_response_mask;

    // put high 16 bits of read register on low 16 bits of F2H
    processCommand(h2f_cmd_get_high_16);

    // get high 16 bits into value
    value = value | ((*f2h & f2h_cmd_response_mask) << 16);

    return value;
}

uint32_t readPC()
{
    return readReg(PC_REG, 0);
}

