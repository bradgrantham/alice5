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

float guesses[255];
float float_nan;

float brads_sqrt(float x)
{
    if(x < 0)
        return float_nan;

    // use exponent to make an initial guess as if it was 1.0^2exp
    uint32_t xi = floatToInt(x);
    uint32_t xi23 = xi >> 23;
    uint32_t ex2 = xi23 & 0xff;
    float x0 = guesses[ex2];

    // Use three steps of Newton's method from
    // https://en.wikipedia.org/wiki/Newton%27s_method#Square_root_of_a_number
    // This gives < .1% error for the test range below.
    float x1 = x0 - (x0 * x0 - x) / (2 * x0);
    float x2 = x1 - (x1 * x1 - x) / (2 * x1);
    float x3 = x2 - (x2 * x2 - x) / (2 * x2);

    return x3;
}

int main()
{
    float_nan = sqrtf(-1);

    /* IEEE 754 exponents range from 0 to 254 */
    for(uint32_t i = 0; i < 255; i++) {
        uint32_t bits = (i << 23);
        // std::cout << "exponent " << i << " = " << intToFloat(bits) << ", sqrt is " << sqrtf(intToFloat(bits)) << "\n";
        guesses[i] = sqrtf(intToFloat(bits));
    }
    for(float x = .0000975; x < 10000; x = x * 1.01) {
        float f1 = sqrtf(x);
        float f2 = brads_sqrt(x);
        float err = fabs(f1 - f2) / f1;
        if(err > .001) {
            std::cout << "sqrtf(" << x << ")," << "brads_sqrt(" << x << ") = " << brads_sqrt(x) << ", error = " << err << "\n";
        }
    }
}
