#include <cstdio>
#include <cmath>

const int tableSegments = 512;

int main(int argc, char **argv)
{
    for(int i = 0; i < tableSegments + 1; i++) {
        float a = 2 * M_PI / tableSegments * i;
        float v = cosf(a);

        unsigned int u = *reinterpret_cast<unsigned int*>(&v);

        printf("        .word   %u ; 0x%08X = %f\n", u, u, v);
    }
}
