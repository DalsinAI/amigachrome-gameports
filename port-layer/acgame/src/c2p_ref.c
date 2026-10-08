#include "acgame/acgame.h"

int acgame_c2p8_ref(const uint8_t *chunky, size_t chunky_pitch,
                    uint8_t *planes[8], size_t plane_pitch,
                    unsigned width, unsigned height) {
    unsigned y, x, p, i;
    if (!chunky || !planes || width == 0 || height == 0 || (width & 7u)) {
        return -1;
    }
    for (p = 0; p < 8; ++p) {
        if (!planes[p]) return -1;
    }
    for (y = 0; y < height; ++y) {
        const uint8_t *src = chunky + (size_t)y * chunky_pitch;
        for (x = 0; x < width; x += 8) {
            uint8_t packed[8] = {0,0,0,0,0,0,0,0};
            for (i = 0; i < 8; ++i) {
                const uint8_t px = src[x + i];
                const uint8_t mask = (uint8_t)(0x80u >> i);
                for (p = 0; p < 8; ++p) {
                    if (px & (1u << p)) packed[p] |= mask;
                }
            }
            for (p = 0; p < 8; ++p) {
                planes[p][(size_t)y * plane_pitch + (x >> 3)] = packed[p];
            }
        }
    }
    return 0;
}

int acgame_p2c8_ref(uint8_t *chunky, size_t chunky_pitch,
                    const uint8_t *planes[8], size_t plane_pitch,
                    unsigned width, unsigned height) {
    unsigned y, x, p;
    if (!chunky || !planes || width == 0 || height == 0 || (width & 7u)) {
        return -1;
    }
    for (p = 0; p < 8; ++p) {
        if (!planes[p]) return -1;
    }
    for (y = 0; y < height; ++y) {
        uint8_t *dst = chunky + (size_t)y * chunky_pitch;
        for (x = 0; x < width; ++x) {
            const uint8_t mask = (uint8_t)(0x80u >> (x & 7u));
            uint8_t px = 0;
            for (p = 0; p < 8; ++p) {
                if (planes[p][(size_t)y * plane_pitch + (x >> 3)] & mask) {
                    px |= (uint8_t)(1u << p);
                }
            }
            dst[x] = px;
        }
    }
    return 0;
}

