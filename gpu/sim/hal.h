#ifndef __HAL_H__
#define __HAL_H__

#include <inttypes.h>

class Hal {
public:
    virtual int getCoreCount() = 0;
    virtual void setH2F(uint32_t value, int coreNumber) = 0;
    virtual uint32_t getF2H(int coreNumber) = 0;
    virtual void allowGpuProgress() = 0;
    virtual ~Hal() {};
};

extern Hal *HalCreateInstance();

// True if only one Instance can be created.
extern bool HalCanCreateMultipleInstances;

uint32_t HalReadMemory(uint32_t address);

std::string HalPreferredName();

#endif /* __HAL_H__ */
