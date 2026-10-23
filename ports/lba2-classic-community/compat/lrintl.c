/* Open2 libnix lacks lrintl. The 68040 FPU converts using the current FPCR
 * rounding mode, including the polygon renderer's temporary truncation mode.
 * Keep the extended operand: narrowing to double can move half-integer ties.
 * The engine consumes signed 32-bit, representable results. */
#ifndef LBA2_LRINTL_SYMBOL
#define LBA2_LRINTL_SYMBOL lrintl
#endif

long LBA2_LRINTL_SYMBOL(long double value)
{
    long result;
    __asm__ ("fmove.l %1,%0" : "=d" (result) : "f" (value));
    return result;
}
