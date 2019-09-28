#include <inttypes.h>
#include <iostream>
#include <cmath>

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

int main()
{
    printf("sqrtf(-1) = 0x%08X\n", floatToInt(sqrtf(-1)));
}
