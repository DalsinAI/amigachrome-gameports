/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 AmigaChrome Team
 *
 * The part of zlib's API fheroes2 calls (uncompress, compress2,
 * compressBound, crc32), answered by the system's z.library (OpenZlib in
 * amigachrome-guest libraries/openzlib, LVOs from its z.lvo). The game links
 * no zlib of its own. */
#ifndef FHEROES2_AMIGA_ZLIB_H
#define FHEROES2_AMIGA_ZLIB_H

#define ZLIB_VERSION "1.3.1"
#define ZLIB_VERNUM 0x1310

#define Z_OK 0
#define Z_STREAM_END 1
#define Z_ERRNO (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR (-3)
#define Z_MEM_ERROR (-4)
#define Z_BUF_ERROR (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NO_COMPRESSION 0
#define Z_BEST_SPEED 1
#define Z_BEST_COMPRESSION 9
#define Z_DEFAULT_COMPRESSION (-1)

#include "zconf.h"

#ifdef __cplusplus
extern "C" {
#endif

int uncompress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
int compress2(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen, int level);
int compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen);
uLong compressBound(uLong sourceLen);
uLong crc32(uLong crc, const Bytef *buf, uInt len);

#ifdef __cplusplus
}
#endif

#endif
