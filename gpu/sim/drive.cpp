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
#include <chrono>

#undef SIMULATE 
constexpr bool dumpH2FAndF2H = false; // true;

#include "risc-v.h"
#include "timer.h"
#include "disassemble.h"
#include "objectfile.h"

#include "hal.h"
#include "corecomm.h"

constexpr std::chrono::microseconds h2f_timeout(h2f_timeout_micros);

typedef std::unique_ptr<Hal> HalPtr;

// Work pool of integer work items.
class WorkPool {
    std::vector<int> mItems;
    std::mutex mMutex;

public:
    // Add an item to the work pool.
    void add(int item) {
        mItems.push_back(item);
    }

    // Add all in a range.
    void addRange(int begin, int end) {
        for (int item = begin; item < end; item++) {
            add(item);
        }
    }

    // Get any item, or -1 if the work pool is empty.
    int get() {
        std::scoped_lock lock(mMutex);
        int item;

        if (mItems.empty()) {
            item = -1;
        } else {
            item = mItems.back();
            mItems.pop_back();
        }

        return item;
    }
};

bool waitOnExitReset(Hal *hal, int coreNumber)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnExitReset()...\n";
    }

    hal->allowGpuProgress();

    auto start = std::chrono::high_resolution_clock::now();
    while((hal->getF2H(coreNumber) & f2h_exited_reset_bit) != f2h_exited_reset_bit) {

        hal->allowGpuProgress();

        // Check clock for timeout
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout) {
            std::cerr << "waitOnExitReset : timed out waiting for FPGA to change phase (" << elapsed.count() << ")\n";
            return false;
        }
    }
    return true;
}

bool waitOnPhaseOrTimeout(int phase, Hal *hal, int coreNumber)
{
    if(dumpH2FAndF2H) {
        std::cout << "waitOnPhaseOrTimeout(" << to_hex(phase) << ")...\n";
    }
    uint32_t phaseExpected = phase ? f2h_busy_bit : 0;

    hal->allowGpuProgress();

    auto start = std::chrono::high_resolution_clock::now();
    while((hal->getF2H(coreNumber) & f2h_busy_bit) != phaseExpected) {

        hal->allowGpuProgress();

        // Check clock for timeout
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::micro> elapsed = now - start;
        if(elapsed > h2f_timeout) {
            std::cerr << "waitOnPhaseOrTimeout : timed out waiting for FPGA to change phase (" << elapsed.count() << ")\n";
            return false;
        }
    }
    return true;
}

bool processCommand(uint32_t command, Hal *hal, int coreNumber)
{
    // double-check we are in idle
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, hal, coreNumber)) {
        std::cerr << "processCommand : timeout waiting on FPGA to become idle before sending command\n";
        return false;
    }
    
    // send the command and then wait for core to report it is processing the command
    hal->setH2F(command, coreNumber);
    if(!waitOnPhaseOrTimeout(f2h_phase_busy, hal, coreNumber)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate busy processing command\n";
        return false;
    }

    // return to idle state and wait for the core to tell us it's also idle.
    hal->setH2F(h2f_gpu_idle, coreNumber);
    if(!waitOnPhaseOrTimeout(f2h_phase_ready, hal, coreNumber)) {
        std::cerr << "processCommand : timeout waiting on FPGA to indicate command is completed\n";
        return false;
    }

    return true;
}

enum RamType { INST_RAM, DATA_RAM };

void CHECK(bool success, const char *filename, int line)
{
    static bool stored_exit_flag = false;
    bool exit_on_error;

    if(!stored_exit_flag) {
        exit_on_error = getenv("EXIT_ON_ERROR") != NULL;
        stored_exit_flag = true;
    }

    if(!success) {
        printf("command processing error at %s:%d\n", filename, line);
        if(exit_on_error) {
            exit(1);
        }
    }
}

void writeWordToRam(uint32_t value, uint32_t address, RamType ramType, Hal *hal, int coreNumber)
{
    // put low 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_low_16 | (value & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);

    // put high 16 bits into write register 
    CHECK(processCommand(h2f_cmd_put_high_16 | ((value >> 16) & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);

    // send the command to store the word from the write register into memory
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_write_inst_ram | (address & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_write_data_ram | (address & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);
    }
}

uint32_t readWordFromRam(uint32_t address, RamType ramType, Hal *hal, int coreNumber)
{
    uint32_t value;

    // send the command to latch the word from memory into the read register
    if(ramType == INST_RAM) {
        CHECK(processCommand(h2f_cmd_read_inst_ram | (address & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_data_ram | (address & 0xFFFF), hal, coreNumber), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, hal, coreNumber), __FILE__, __LINE__);

    // get low 16 bits into value
    value = hal->getF2H(coreNumber) & 0xFFFF;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, hal, coreNumber), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((hal->getF2H(coreNumber) & 0xFFFF) << 16);

    return value;
}

enum RegisterKind {X_REG, F_REG, PC_REG};

uint32_t readReg(RegisterKind kind, int reg, Hal *hal, int coreNumber)
{
    uint32_t value;

    // send the command to latch the register into the read register
    if(kind == X_REG) {
        CHECK(processCommand(h2f_cmd_read_x_reg | (reg & 0x1F), hal, coreNumber), __FILE__, __LINE__);
    } else if(kind == PC_REG) {
        CHECK(processCommand(h2f_cmd_read_special | h2f_special_PC, hal, coreNumber), __FILE__, __LINE__);
    } else {
        CHECK(processCommand(h2f_cmd_read_f_reg | (reg & 0x1F), hal, coreNumber), __FILE__, __LINE__);
    }

    // put low 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_low_16, hal, coreNumber), __FILE__, __LINE__);

    // get low 16 bits into value
    value = hal->getF2H(coreNumber) & f2h_cmd_response_mask;

    // put high 16 bits of read register on low 16 bits of F2H
    CHECK(processCommand(h2f_cmd_get_high_16, hal, coreNumber), __FILE__, __LINE__);

    // get high 16 bits into value
    value = value | ((hal->getF2H(coreNumber) & f2h_cmd_response_mask) << 16);

    return value;
}

uint32_t readPC(Hal *hal, int coreNumber)
{
    return readReg(PC_REG, 0, hal, coreNumber);
}

void writeBytesToRam(const std::vector<uint8_t>& bytes, RamType ramType, bool dumpState, Hal *hal, int coreNumber)
{
    // Write inst bytes to inst memory
    for(uint32_t byteaddr = 0; byteaddr < bytes.size(); byteaddr += 4) {
        uint32_t word = 
            bytes[byteaddr + 0] <<  0 |
            bytes[byteaddr + 1] <<  8 |
            bytes[byteaddr + 2] << 16 |
            bytes[byteaddr + 3] << 24;

        writeWordToRam(word, byteaddr, ramType, hal, coreNumber);

    }
}

template <class TYPE>
void set(Hal *hal, uint32_t address, const TYPE& value, int coreNumber)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    writeWordToRam(toBits(value), address, DATA_RAM, hal, coreNumber);
}

template <class TYPE, unsigned long N>
void set(Hal *hal, uint32_t address, const std::array<TYPE, N>& value, int coreNumber)
{
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        writeWordToRam(toBits(value[i]), address + i * sizeof(uint32_t), DATA_RAM, hal, coreNumber);
}

template <class TYPE, unsigned long N>
void get(Hal *hal, uint32_t address, std::array<TYPE, N>& value, int coreNumber)
{
    // TODO: use a state machine through Main to read memory using clock
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    for(unsigned long i = 0; i < N; i++)
        value[i] = fromBits<TYPE>(readWordFromRam(address + i * sizeof(uint32_t), DATA_RAM, hal, coreNumber)) ;
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

    CoreShared()
    {
        // Nothing.
    }
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

void startProgram(Hal *hal, int coreNumber)
{
    // Run one clock cycle in not-run reset to process STATE_INIT
    hal->setH2F(h2f_gpu_reset, coreNumber);
    hal->allowGpuProgress();

    // Release reset
    hal->setH2F(h2f_gpu_idle, coreNumber);
    waitOnExitReset(hal, coreNumber);

    // run == 0, nothing should be happening here.
    hal->allowGpuProgress();

    // Run
    hal->setH2F(h2f_gpu_run, coreNumber);
}

// From https://en.cppreference.com/w/cpp/algorithm/clamp
template<class T>
constexpr const T& clamp( const T& v, const T& lo, const T& hi )
{
    assert( !(hi < lo) );
    return (v < lo) ? lo : (hi < v) ? hi : v;
}

/**
 * Whether the particular core has halted.
 */
bool isCoreHalted(Hal *hal, int coreNumber) {
    return (hal->getF2H(coreNumber) & f2h_run_halted_bit) != 0;
}

/**
 * Configure the core to prepare to do this row. Does not start the program.
 */
void configureCoreForRow(const SimDebugOptions *debugOptions, const CoreParameters *params,
        Hal *hal, int row, int coreNumber)
{
    // Set up parameters.
    if(params->gl_FragCoordAddress != 0xFFFFFFFF)
        set(hal, params->gl_FragCoordAddress, v4float{params->startX + 0.5f, row + 0.5f, 0, 0}, coreNumber);
    if(params->colorAddress != 0xFFFFFFFF)
        set(hal, params->colorAddress, v4float{1, 1, 1, 1}, coreNumber);
    if(false) // XXX TODO: when iTime is exported by compilation
        set(hal, params->iTimeAddress, params->frameTime, coreNumber);
    uint32_t width = params->afterLastX - params->startX;
    set(hal, params->rowWidthAddress, width, coreNumber);
    // Offset in pixels into the image.
    uint32_t pixelOffset = (params->imageHeight - 1 - row)*params->imageWidth + params->startX;
    // Offset in bytes into the shared memory space.
    uint32_t sdramAddr = pixelOffset*3*sizeof(float);
    set(hal, params->colBufAddrAddress, sdramAddr | 0x80000000, coreNumber);

    if(debugOptions->printDisassembly) {
        std::cout << "; pixel " << params->startX << ", " << row << '\n';
    }
}

/**
 * Put the core briefly into reset, and take it back out. Leaves core
 * in non-running mode.
 */
void resetCore(Hal *hal, int coreNumber) {
    // Run one clock cycle in reset to process STATE_INIT
    hal->setH2F(h2f_gpu_reset, coreNumber);
    hal->allowGpuProgress();

    // Release reset
    hal->setH2F(h2f_gpu_idle, coreNumber);
    waitOnExitReset(hal, coreNumber);

    // run == 0, nothing should be happening here.
    hal->allowGpuProgress();
}

/**
 * Load instruction and data memory for the core.
 */
void loadMemory(const SimDebugOptions* debugOptions, const CoreParameters* params, Hal *hal,
        int coreNumber) {

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->inst_bytes, INST_RAM, debugOptions->dumpState, hal, coreNumber);

    // TODO - should we count clocks issued here?
    writeBytesToRam(params->data_bytes, DATA_RAM, debugOptions->dumpState, hal, coreNumber);

#if SIMULATE
    // check words written to inst memory
    for(uint32_t byteaddr = 0; byteaddr < params->inst_bytes.size(); byteaddr += 4) {
        uint32_t inst_word = 
            params->inst_bytes[byteaddr + 0] <<  0 |
            params->inst_bytes[byteaddr + 1] <<  8 |
            params->inst_bytes[byteaddr + 2] << 16 |
            params->inst_bytes[byteaddr + 3] << 24;
        if(gpuCore->instRam->memory[byteaddr >> 2] != inst_word) {
            std::cout << "error: inst memory[" << byteaddr << "] = "
                << to_hex(gpuCore->instRam->memory[byteaddr >> 2])
                << ", should be " << to_hex(inst_word) << "\n";
        }
    }

    // check words written to data memory
    for(uint32_t byteaddr = 0; byteaddr < params->data_bytes.size(); byteaddr += 4) {
        uint32_t data_word = 
            params->data_bytes[byteaddr + 0] <<  0 |
            params->data_bytes[byteaddr + 1] <<  8 |
            params->data_bytes[byteaddr + 2] << 16 |
            params->data_bytes[byteaddr + 3] << 24;
        if(gpuCore->dataRam->memory[byteaddr >> 2] != data_word) {
            std::cout << "error: data memory[" << byteaddr << "] = "
                << to_hex(gpuCore->dataRam->memory[byteaddr >> 2])
                << ", should be " << to_hex(data_word) << "\n";
        }
    }
#endif
}

// Represents a single hardware machine (possibly with multiple FPGA cores).
void render(const SimDebugOptions* debugOptions, const CoreParameters* params, CoreShared* shared, WorkPool *workPool)
{
    HalPtr hal {HalGetInstance()};

    uint32_t insts = 0;

    const float fw = params->imageWidth;
    const float fh = params->imageHeight;
    const float one = 1.0;

    for (int coreNumber = 0; coreNumber < hal->getCoreCount(); coreNumber++) {
        resetCore(hal.get(), coreNumber);
        loadMemory(debugOptions, params, hal.get(), coreNumber);

        if (params->data_symbols.find(".anonymous") != params->data_symbols.end()) {
            set(hal.get(), params->data_symbols.find(".anonymous")->second, v3float{fw, fh, one}, coreNumber);
            set(hal.get(), params->data_symbols.find(".anonymous")->second + sizeof(v3float), params->frameTime, coreNumber);
        }
    }

    // Keep track of which core is busy.
    bool coreIsWorking[hal->getCoreCount()];
    steady_clock::time_point coreStartTime[hal->getCoreCount()];
    for (int coreNumber = 0; coreNumber < hal->getCoreCount(); coreNumber++) {
        coreIsWorking[coreNumber] = false;
    }

#ifdef SIMULATE

    // Should eventually remove this and all the code that depends on it.
    const int debugCoreNumber = 0;

    VMain_GPU *debugGpuCore = getGpuByCore(top, debugCoreNumber);

    uint32_t oldXRegs[32];
    uint32_t oldFRegs[32];
    uint32_t oldPC;
    for(int i = 0; i < 32; i++) oldXRegs[i] = debugGpuCore->shaderCore->registers->bank1->memory[i];
    for(int i = 0; i < 32; i++) oldFRegs[i] = debugGpuCore->shaderCore->float_registers->bank1->memory[i];
    oldPC = debugGpuCore->shaderCore->PC;

#endif // SIMULATE

    // XXX while (!Verilated::gotFinish()) {
    while (true) {
        // Dispatch work to idle cores.
        int idleCoreCount = 0;
        for (int coreNumber = 0; coreNumber < hal->getCoreCount(); coreNumber++) {
            if (!coreIsWorking[coreNumber]) {
                int row = workPool->get();

                // We have an idle core. Dispatch a row if we have one.
                if (row == -1) {
                    // No rows left.
                    idleCoreCount++;
                } else {
                    printf("Core %d is idle, doing row %d.\n", coreNumber, row);

                    resetCore(hal.get(), coreNumber);
                    configureCoreForRow(debugOptions, params, hal.get(), row, coreNumber);
                    startProgram(hal.get(), coreNumber);
                    coreIsWorking[coreNumber] = true;
                    coreStartTime[coreNumber] = steady_clock::now();
                }
            }
        }
        if (idleCoreCount == hal->getCoreCount()) {
            // Done with all rendering.
            break;
        }

        hal->allowGpuProgress();

#ifdef SIMULATE

        if(false) {
            // TODO this used to be when clock = 1, so maybe move to allowGpuProgress().
            // left side of nonblocking assignments
            std::string pad = "                     ";
            std::cout << pad << "between clock 1 and clock 0\n";
            std::cout << pad << "CPU in state " << stateToString(debugGpuCore->shaderCore->state) << " (" << int(debugGpuCore->shaderCore->state) << ")\n";
            std::cout << pad << "pc = 0x" << to_hex(debugGpuCore->shaderCore->PC) << "\n";
            std::cout << pad << "inst_ram_address = 0x" << to_hex(debugGpuCore->shaderCore->inst_ram_address) << "\n";
            std::cout << pad << "inst_to_decode = 0x" << to_hex(debugGpuCore->shaderCore->inst_to_decode) << "\n";
            std::cout << pad << "data_ram_write_data = 0x" << to_hex(debugGpuCore->shaderCore->data_ram_write_data) << "\n";
            std::cout << pad << "data_ram_address = 0x" << to_hex(debugGpuCore->shaderCore->data_ram_address) << "\n";
            std::cout << pad << "data_ram_read_result = 0x" << to_hex(debugGpuCore->shaderCore->data_ram_read_result) << "\n";
            std::cout << pad << "data_ram_write = 0x" << to_hex(debugGpuCore->shaderCore->data_ram_write) << "\n";
        }

        if(debugOptions->dumpState) {

            dumpRegisters(debugGpuCore);
        }

        if(debugOptions->dumpState) {
            // right side of nonblocking assignments
            std::cout << "between clock 0 and clock 1\n";
            std::cout << "CPU in state " << stateToString(debugGpuCore->shaderCore->state) << " (" << int(debugGpuCore->shaderCore->state) << ")\n";
            if(false) {
                std::cout << "inst_ram_address = 0x" << to_hex(debugGpuCore->shaderCore->inst_ram_address) << "\n";
                std::cout << "inst_to_decode = 0x" << to_hex(debugGpuCore->shaderCore->inst_to_decode) << "\n";
            }
        }

        if(debugGpuCore->shaderCore->state == VMain_ShaderCore::STATE_RETIRE) {
            insts++;
        }

        if((debugGpuCore->shaderCore->state == VMain_ShaderCore::STATE_EXECUTE) ||
            (debugGpuCore->shaderCore->state == VMain_ShaderCore::STATE_FP_WAIT)
            ) {
            if(debugOptions->dumpState) {
                std::cout << "after DECODE - ";
                printDecodedInst(debugGpuCore->shaderCore->PC, debugGpuCore->shaderCore->inst_to_decode, debugGpuCore->shaderCore);
            }
            if(debugOptions->printDisassembly) {
                print_inst(debugGpuCore->shaderCore->PC, debugGpuCore->instRam->memory[debugGpuCore->shaderCore->PC / 4], params->textAddressesToSymbols);
            }
        }

        // if(debugGpuCore->shaderCore->state == VMain_ShaderCore::STATE_FETCH) {
            if(debugOptions->printCoreDiff) {

                dumpRegsDiff(oldPC, oldXRegs, oldFRegs, debugGpuCore);

                for(int i = 0; i < 32; i++) oldXRegs[i] = debugGpuCore->shaderCore->registers->bank1->memory[i];
                for(int i = 0; i < 32; i++) oldFRegs[i] = debugGpuCore->shaderCore->float_registers->bank1->memory[i];
                oldPC = debugGpuCore->shaderCore->PC;
            }
        // }

        if(debugOptions->dumpState) {
            std::cout << "---\n";
        }

#endif // SIMULATE

        // See if any cores are now idle.
        for (int coreNumber = 0; coreNumber < hal->getCoreCount(); coreNumber++) {
            if (coreIsWorking[coreNumber] && isCoreHalted(hal.get(), coreNumber)) {
                steady_clock::time_point now = steady_clock::now();
                steady_clock::duration timeSpan = now - coreStartTime[coreNumber];
                double seconds = double(timeSpan.count())*steady_clock::period::num/
                    steady_clock::period::den;
                printf("Core %d is no longer working (%.1f seconds)\n", coreNumber, seconds);

                uint32_t width = params->afterLastX - params->startX;
                shared->pixelsLeft -= width;

                /* return to STATE_INIT so we can drive ext memory access */
                hal->setH2F(h2f_gpu_idle, coreNumber);

                coreIsWorking[coreNumber] = false;
            }
        }
    }

#ifdef SIMULATE

    if(debugOptions->dumpState) {
        std::cout << "halted.\n";

        dumpRegisters(debugGpuCore);

        // Dump contents of beginning of memory
        for (int i = 0; i < 16; i++) {
            // Draw in columns.
            bool start = (i % 4 == 0);
            bool end = (i % 4 == 3);
            if(start) {
                std::cout << "0x" << to_hex(i) << " :";
            }

            std::cout << " " << to_hex(debugGpuCore->dataRam->memory[i]) << "   ";
            if (end) {
                std::cout << "\n";
            }
        }
    }

#endif // SIMULATE

    {
        shared->rendererMutex.lock();
        shared->dispatchedCount += insts; // XXX probe from core
        shared->clocksCount += 0; //  XXX hal->getClockCount();
        shared->rendererMutex.unlock();
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
    int threadCount = std::thread::hardware_concurrency();

    SimDebugOptions debugOptions;
    CoreParameters params;
    CoreShared shared;

    debugOptions.dumpState = false;

    // XXX Verilated::commandArgs(argc, argv);
    // XXX Verilated::debug(1);

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
    params.rowWidthAddress = params.data_symbols[".rowWidth"];
    params.colBufAddrAddress = params.data_symbols[".colBufAddr"];

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

    // Set up our work pool.
    WorkPool workPool;
    workPool.addRange(params.startY, params.afterLastY);

    // Start the threads.
    for (int t = 0; t < threadCount; t++) {
        thread.push_back(new std::thread(render, &debugOptions, &params, &shared, &workPool));
    }

    // Progress information.
    Timer frameElapsed;
    if (false) {
        thread.push_back(new std::thread(showProgress, &params, &shared,
                    frameElapsed.startTime(), totalPixels));
    }

    // Wait for worker threads to quit.
    while(!thread.empty()) {
        std::thread* td = thread.back();
        thread.pop_back();
        td->join();
    }

    if(shared.coreHadAnException) {
        exit(EXIT_FAILURE);
    }

    // Convert image to bytes.
    uint32_t sdramAddr = SDRAM_BASE;
    uint8_t *rgbByte = shared.img;
    for (int y = 0; y < params.imageHeight; y++) {
        for (int x = 0; x < params.imageWidth; x++) {
            for (int c = 0; c < 3; c++) {
                uint32_t v = HalReadMemory(sdramAddr++);
                float f = intToFloat(v);
                *rgbByte++ = clamp(int(f * 255.99), 0, 255);
            }
        }
    }

    std::cout << "shading took " << frameElapsed.elapsed() << " seconds.\n";
    std::cout << shared.dispatchedCount << " instructions executed.\n";
    std::cout << shared.clocksCount << " clocks cycled.\n";
    float fps = 50000000.0f / shared.clocksCount;
    std::cout << fps << " fps estimated at 50 MHz.\n";
    float wvgaFps = fps * params.imageWidth / 800 * params.imageHeight / 480;
    std::cout << "at least " << (int)ceilf(5.0 / wvgaFps) << " cores required at 50 MHz for 5 fps at 800x480.\n";

    FILE *fp = fopen("simulated.ppm", "wb");
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

