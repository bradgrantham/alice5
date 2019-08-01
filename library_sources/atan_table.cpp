#include <cstdio>
#include <cmath>

const int tableSegments = 128;

int main(int argc, char **argv)
{
    for(int i = 0; i < tableSegments + 1; i++) {
        float a = i / (float)tableSegments;
        float v = atanf(a);
        unsigned int u = *reinterpret_cast<unsigned int*>(&v);

        printf("        .word   %u ; 0x%08X = %f\n", u, u, v);
    }
}
