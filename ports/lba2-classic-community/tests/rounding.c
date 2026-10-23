#include <fenv.h>
#include <math.h>
#include <stdio.h>

extern long openamiga_lrintl(long double);

int main(void)
{
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const long double special[] = {
        0.0L, -0.0L, 0.5L, -0.5L, 1.5L, -1.5L, 2.5L, -2.5L,
        2147483646.5L, -2147483647.5L,
        0.5L - 0x1p-60L, 0.5L + 0x1p-60L,
        -0.5L - 0x1p-60L, -0.5L + 0x1p-60L
    };
    unsigned checks = 0;
    for (unsigned m = 0; m < sizeof(modes) / sizeof(modes[0]); ++m) {
        if (fesetround(modes[m])) return 2;
        for (unsigned i = 0; i < sizeof(special) / sizeof(special[0]); ++i) {
            if (openamiga_lrintl(special[i]) != lrintl(special[i])) return 3;
            ++checks;
        }
        for (int i = -10000; i <= 10000; ++i) {
            const long double value = (long double)i / 8.0L;
            if (openamiga_lrintl(value) != lrintl(value)) return 4;
            ++checks;
        }
    }
    printf("%u exact rounding comparisons passed on m68k\n", checks);
    return 0;
}
