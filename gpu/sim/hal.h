#ifndef __HAL_H__
#define __HAL_H__

#include <inttypes.h>

class Hal {
public:
    virtual int getCoreCount() = 0;
    virtual void setH2F(uint32_t value, int coreNumber) = 0;
    virtual uint32_t getF2H(int coreNumber) = 0;
    virtual uint32_t readRam(uint32_t wordAddress) = 0;
    virtual void allowGpuProgress() = 0;
    virtual ~Hal() {};
};

extern Hal *HalGetInstance();
uint32_t HalReadMemory(uint32_t address);

#endif /* __HAL_H__ */
