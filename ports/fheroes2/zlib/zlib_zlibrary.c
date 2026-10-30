/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 AmigaChrome Team
 *
 * zlib.h's calls for fheroes2, through z.library. The library is opened on
 * first use and closed at exit. Without z.library the (de)compression calls
 * return Z_MEM_ERROR, as zlib does when it cannot get memory, and crc32 is
 * computed here so the game's checksums never depend on it. */
#include <stdlib.h>
#include <exec/types.h>
#include <proto/exec.h>
#include "zlib.h"

static struct Library *ZBase;
static int ZTried;

static void z_close(void)
{
    if (ZBase) {
        CloseLibrary(ZBase);
        ZBase = NULL;
    }
}

static struct Library *z_base(void)
{
    if (!ZTried) {
        ZTried = 1;
        ZBase = OpenLibrary((CONST_STRPTR)"z.library", 0);
        if (ZBase)
            atexit(z_close);
    }
    return ZBase;
}

/* z.library -204 Compress2(a0 dest, a1 destLen, a2 source, d0 sourceLen, d1 level) */
int compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level)
{
    struct Library *base = z_base();
    if (!base)
        return Z_MEM_ERROR;
    register LONG r __asm("d0") = (LONG)sourceLen;
    register LONG l __asm("d1") = level;
    register APTR a0 __asm("a0") = dest;
    register APTR a1 __asm("a1") = destLen;
    register CONST_APTR a2 __asm("a2") = source;
    register struct Library *a6 __asm("a6") = base;
    __asm volatile("jsr %%a6@(-204:W)"
                   : "+d"(r), "+d"(l), "+a"(a0), "+a"(a1), "+a"(a2)
                   : "a"(a6)
                   : "cc", "memory", "fp0", "fp1");
    return (int)r;
}

int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    return compress2(dest, destLen, source, sourceLen, Z_DEFAULT_COMPRESSION);
}

/* z.library -210 CompressBound(d0 sourceLen) */
uLong compressBound(uLong sourceLen)
{
    struct Library *base = z_base();
    if (!base) /* zlib's own bound, used only to size a buffer */
        return sourceLen + (sourceLen >> 12) + (sourceLen >> 14) + (sourceLen >> 25) + 13;
    register ULONG r __asm("d0") = sourceLen;
    register struct Library *a6 __asm("a6") = base;
    __asm volatile("jsr %%a6@(-210:W)"
                   : "+d"(r)
                   : "a"(a6)
                   : "d1", "a0", "a1", "cc", "memory", "fp0", "fp1");
    return r;
}

/* z.library -216 Uncompress(a0 dest, a1 destLen, a2 source, d0 sourceLen) */
int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen)
{
    struct Library *base = z_base();
    if (!base)
        return Z_MEM_ERROR;
    register LONG r __asm("d0") = (LONG)sourceLen;
    register APTR a0 __asm("a0") = dest;
    register APTR a1 __asm("a1") = destLen;
    register CONST_APTR a2 __asm("a2") = source;
    register struct Library *a6 __asm("a6") = base;
    __asm volatile("jsr %%a6@(-216:W)"
                   : "+d"(r), "+a"(a0), "+a"(a1), "+a"(a2)
                   : "a"(a6)
                   : "d1", "cc", "memory", "fp0", "fp1");
    return (int)r;
}

/* z.library -234 Crc32(d0 crc, a0 buf, d1 len) */
uLong crc32(uLong crc, const Bytef *buf, uInt len)
{
    struct Library *base = z_base();
    if (!buf)
        return 0;
    if (!base) {
        crc = ~crc;
        while (len--) {
            crc ^= *buf++;
            for (int k = 0; k < 8; k++)
                crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1)));
        }
        return ~crc;
    }
    register ULONG r __asm("d0") = crc;
    register CONST_APTR a0 __asm("a0") = buf;
    register ULONG d1 __asm("d1") = len;
    register struct Library *a6 __asm("a6") = base;
    __asm volatile("jsr %%a6@(-234:W)"
                   : "+d"(r), "+a"(a0), "+d"(d1)
                   : "a"(a6)
                   : "a1", "cc", "memory", "fp0", "fp1");
    return r;
}
