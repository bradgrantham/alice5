#ifndef _UTIL_H_
#define _UTIL_H_

#include <sstream>
#include <iomanip>
#include <iostream>

template<typename T>
std::string to_hex(T i) {
    std::stringstream stream;
    stream << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << std::uppercase;
    stream << uint64_t(i);
    return stream.str();
}

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

template <typename TYPE>
TYPE fromBits(uint32_t u) {
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    union bits {uint32_t u; TYPE x;} x;

    x.u = u;

    return x.x;
}

template <typename TYPE>
uint32_t toBits(const TYPE& f) {
    static_assert(sizeof(TYPE) == sizeof(uint32_t));
    union bits {uint32_t u; TYPE x;} x;

    x.x = f;

    return x.u;
}


#endif /* _UTIL_H_ */
