#include <math.h>
#include <stdio.h>

#define TABLE_POW2 128
float atanTable[TABLE_POW2 + 1];

float brad_atan_0_1(float z)
{
#if 1

    int i = z * TABLE_POW2;
    float a = z * TABLE_POW2 - i;
    float lower = atanTable[i];
    float higher = atanTable[i + 1];
    return lower * (1 - a) + higher * a;

#else

    float v = z;
    float d = 1;
    float z2 = z * z;

    /* z */
    float a = z;

    /* -z^3 / 3 */
    v = -v * z2;
    d = d + 2;
    a = a + v / d;

    /* z^5 / 5 */
    v = -v * z2;
    d = d + 2;
    a = a + v / d;

    /* -z^7 / 7 */
    v = -v * z2;
    d = d + 2;
    a = a + v / d;

    /* z^9 / 9 */
    v = -v * z2;
    d = d + 2;
    a = a + v / d;

    /* -z^11 / 11 */
    v = -v * z2;
    d = d + 2;
    a = a + v / d;

    return a;

#endif
}

float brad_atan(float y_x)
{
#if 0
    if(fabsf(y_x) > 1.0) {
        return copysign(M_PI, y_x) / 2 + brad_atan_0_1(1.0 / fabs(y_x));
    } else {
        return copysign(brad_atan_0_1(fabs(y_x)), y_x);
    }
#else
    if(fabsf(y_x) > 1.0) {
        if(y_x < 0.0) {
            return -M_PI / 2 + brad_atan_0_1(1.0 / -y_x);
        } else {
            return M_PI / 2 - brad_atan_0_1(1.0 / y_x);
        }
    } else {
        if(y_x < 0.0) {
            return -brad_atan_0_1(-y_x);
        } else {
            return brad_atan_0_1(y_x);
        }
    }
#endif
}

float brad_atan2(float y, float x)
{
    float z = y / x;
    if(x >= 0) {
        return brad_atan(z);
    } else {
        float z2 = copysign(z, -y);
        float a = copysign(brad_atan(z2), -y);
        return copysign(M_PI, y) + a;
        // return copysign(M_PI, y) - copy_sign(brad_atan(-copysign(z, y), y));
        // if(y >= 0) {
            // return M_PI - brad_atan(-z);
        // } else {
            // return -M_PI + brad_atan(z); /* -y / -x minus signs cancel */
        // }
    }
}

int main()
{
    for(int i = 0; i <= TABLE_POW2; i++)
        atanTable[i] = atanf(i / (float)TABLE_POW2);

#if 0
    for(float a = -M_PI / 2; a <= M_PI / 2; a += .001) {
        float f = tan(a);
        printf("%f %f %f %f%%\n", atanf(f), brad_atan(f), fabs(atan(f) - brad_atan(f)), fabs(atanf(f) - brad_atan(f)) / atanf(f) * 100);
    }
#endif
    
    for(float a = -M_PI; a <= M_PI; a += .001) {
        float x = cos(a);
        float y = sin(a);
        printf("%f %f %f %f%%\n", atan2(y, x), brad_atan2(y, x), fabs(atan2(y, x) - brad_atan2(y, x)), fabs(atan2(y, x) - brad_atan2(y, x)) / atan2(y, x) * 100);
    }
}
