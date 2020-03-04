#ifndef __CORECOMM_H__
#define __CORECOMM_H__

#include <inttypes.h>

// In words.
static const uint32_t SDRAM_BASE = 0x3E000000 >> 2;
static const uint32_t SDRAM_SIZE = (16*1024*1024) >> 2;

constexpr unsigned long long h2f_timeout_micros = 1000000;

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

inline const char *cmdToString(uint32_t h2f)
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

#endif /* __CORECOMM_H__ */
