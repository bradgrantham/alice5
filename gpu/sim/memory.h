
#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <verilated.h>

#define DUMP_MEMORY_ACCESS false

/**
 * Class to simulate a chunk of memory.
 */
class Memory {
public:
    Memory(uint32_t base, int wordCount)
        : mBase(base), mWordCount(wordCount)
    {
        mWords = new uint32_t[wordCount];

        for (int i = 0; i < 16; i++) {
            uint32_t mask = 0;

            // Bit 0 corresponds to the least significant byte.
            // See the Avalong Memory-Mapped spec.
            for (int j = 0; j < 4; j++) {
                if (((i >> j) & 1) != 0) {
                    mask |= (uint32_t) 0xFF << (j*8);
                }
            }

            mByteEnableMask[i] = mask;
        }
    }

    virtual ~Memory() {
        delete[] mWords;
    }

    uint32_t &operator[](uint32_t address) {
        return mWords[addressToIndex(address)];
    }

    void evalRead(vluint8_t burstCount, vluint8_t read, vluint32_t address, vluint8_t &waitRequest,
            vluint8_t &readDataValid, vluint32_t &readData) {

        if (read) {
            if (burstCount != 1) {
                printf("Burst count is %d, must be 1.\n", (int) burstCount);
                throw std::exception();
            }

            readData = (*this)[address];
            if (DUMP_MEMORY_ACCESS && (address & 0xFF) == 0) {
                printf("-------------------- Reading 0x%016x from 0x%08x\n", readData, address);
            }
            readDataValid = 1;
        } else {
            readDataValid = 0;
        }

        waitRequest = 0;
    }

    void evalWrite(vluint8_t burstCount, vluint8_t write, vluint32_t address, vluint8_t &waitRequest,
            vluint8_t byteEnable, vluint32_t writeData) {

        if (write) {
            if (burstCount != 1) {
                printf("Burst count is %d, must be 1.\n", (int) burstCount);
                throw std::exception();
            }

            if (DUMP_MEMORY_ACCESS && (address & 0xFF) == 0) {
                printf("-------------------- Writing 0x%016x to 0x%08x\n", writeData, address);
            }

            if (byteEnable == 0xFF) {
                (*this)[address] = writeData;
            } else {
                uint32_t mask = mByteEnableMask[byteEnable];
                (*this)[address] = ((*this)[address] & ~mask) | (writeData & mask);
            }
        }

        waitRequest = 0;
    }

    void evalReadWrite(vluint8_t read, vluint8_t write, vluint32_t address, vluint8_t &waitRequest,
            vluint8_t &readDataValid, vluint32_t &readData, vluint8_t byteEnable,
            vluint32_t writeData) {

        if (read) {
            readData = (*this)[address];
            if (DUMP_MEMORY_ACCESS && (address & 0xFF) == 0) {
                printf("-------------------- Reading 0x%016x from 0x%08x\n", readData, address);
            }
            readDataValid = 1;
        } else {
            readDataValid = 0;
        }

        if (write) {
            if (DUMP_MEMORY_ACCESS && (address & 0xFF) == 0) {
                printf("-------------------- Writing 0x%016x to 0x%08x\n", writeData, address);
            }

            if (byteEnable == 0xFF) {
                (*this)[address] = writeData;
            } else {
                uint32_t mask = mByteEnableMask[byteEnable];
                (*this)[address] = ((*this)[address] & ~mask) | (writeData & mask);
            }
        }

        waitRequest = 0;
    }

public:
    uint32_t mBase;
    int mWordCount;
    uint32_t *mWords;
    uint32_t mByteEnableMask[16];

    uint32_t addressToIndex(uint32_t address) {
        if (address < mBase) {
            printf("Address 0x%0X is too low (less than 0x%0X).\n", address, mBase);
            throw std::exception();
        }

        if (address >= mBase + mWordCount) {
            printf("Address 0x%0X is too high (past 0x%0X).\n", address, mBase + mWordCount);
            throw std::exception();
        }

        return address - mBase;
    }
};

#endif // __MEMORY_H__
