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
    /* IEEE 754 exponents range from 0 to 254 */
    for(uint32_t i = 0; i < 255; i++) {
        uint32_t bits = (i << 23);
        // std::cout << "exponent " << i << " = " << intToFloat(bits) << ", sqrt is " << sqrtf(intToFloat(bits)) << "\n";
        float v = sqrtf(intToFloat(bits));
        uint32_t u = floatToInt(v);
        printf("        .word   %u ; 0x%08X = %g (sqrt(%g))\n", u, u, v, intToFloat(bits));
    }
}

