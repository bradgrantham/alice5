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
#include <thread>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FPGA_MANAGER_BASE 0xFF208000
#define FPGA_CORE_INCREMENT 0x1000
#define FPGA_MANAGER_SIZE 0x100000
#define FPGA_GPO_OFFSET 0x0
#define FPGA_GPI_OFFSET 0x0

#define SHARED_MEM_BASE 0x3E000000
#define SHARED_MEM_SIZE (16*1024*1024)

constexpr bool dumpH2FAndF2H = false; // true;

#include "util.h"
#include "corecomm.h"
#include "hal.h"

constexpr int MAX_CORE_COUNT = 64;

using namespace std::chrono_literals;

// HAL specifically for hardware, e.g. FPGA
class RealHardware : public Hal {

    volatile unsigned long *mGpo[MAX_CORE_COUNT];
    volatile unsigned long *mGpi[MAX_CORE_COUNT];
    static volatile void *gSdram;

public:
    RealHardware() {
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

        for(int core = 0; core < getCoreCount(); core++) {
            mGpo[core] = (unsigned long*)(mem + FPGA_GPO_OFFSET + FPGA_CORE_INCREMENT * core);
            mGpi[core] = (unsigned long*)(mem + FPGA_GPI_OFFSET + FPGA_CORE_INCREMENT * core);
        }

        // For shared memory.
        gSdram = mmap(0, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, /* MAP_NOCACHE | */ MAP_SHARED , devMem, SHARED_MEM_BASE);
        if(gSdram == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    virtual ~RealHardware() {
    }

    virtual void setH2F(uint32_t value, int coreNumber) {
        if(dumpH2FAndF2H) {
            std::cout << "set h2f_value[" << coreNumber << "] to " << to_hex(value) << "\n";
        }
        *(mGpo[coreNumber]) = value;
    }

    virtual uint32_t getF2H(int coreNumber) {
        uint32_t f2h_value = *(mGpi[coreNumber]);
        if(dumpH2FAndF2H) {
            std::cout << "get f2h_value[" << coreNumber << "] yields " << to_hex(f2h_value) << "\n";
        }
        return f2h_value;
    }

    virtual void allowGpuProgress() {
        // std::this_thread::sleep_for(1us);
    }

    uint32_t getClockCount() const {
        return 0;
    }

    virtual int getCoreCount() {
        return 4; // XXX probe from hardware
    }

    static uint32_t readSdram(uint32_t wordAddress)
    {
        return (reinterpret_cast<volatile uint32_t*>(gSdram))[wordAddress - (SHARED_MEM_BASE >> 2)];
    }
};

volatile void *RealHardware::gSdram;

// True if only one Instance can be created.
bool HalCanCreateMultipleInstances = false;

Hal *HalCreateInstance()
{
    RealHardware *si = new RealHardware;

    return si;
}

uint32_t HalReadMemory(uint32_t address)
{
    return RealHardware::readSdram(address);
}

std::string HalPreferredName()
{
    return "fpga";
}
