/* nativertg: an OCS/ECS chipset picture redrawn for an RTG screen.
 *
 * The game keeps running unchanged on the 68040 and keeps drawing into Chip
 * RAM. Once a frame, the game hook fills a nativertg_frame from what the game
 * has told the chipset (bitplane pointers, BPLCON0/2, colours, sprites) and
 * nativertg turns it into a true-colour picture several times the size, for an
 * RTG screen. Nothing here touches the hardware, so it builds and tests on the
 * host as well as on the Amiga.
 *
 * Copyright (c) 2026 AmigaChrome Team
 */
#ifndef NATIVERTG_H
#define NATIVERTG_H

#include <stddef.h>
#include <stdint.h>

#define NATIVERTG_MAX_PLANES  6
#define NATIVERTG_MAX_SPRITES 8
#define NATIVERTG_COLOURS     32

/* One hardware sprite as the chipset would show it: x and y in playfield
 * pixels, and two data words (A, B) per line, as in the sprite's DMA list. */
typedef struct nativertg_sprite {
    int x, y, height;            /* height 0 = sprite off */
    const uint16_t *data;        /* height * 2 words: A0 B0 A1 B1 ... */
    int attached;                /* odd sprite: attach to the even one below */
} nativertg_sprite;

typedef struct nativertg_frame {
    int width, height;           /* playfield pixels shown */
    int depth;                   /* 1..6 bitplanes */
    int dual_playfield;          /* BPLCON0 DBLPF */
    int pf2_priority;            /* BPLCON2 PF2PRI: playfield 2 in front */
    int pf1p, pf2p;              /* BPLCON2 PF1P and PF2P, 0..4 */
    const uint8_t *plane[NATIVERTG_MAX_PLANES];
    int stride[NATIVERTG_MAX_PLANES];   /* bytes from one line to the next */
    int scroll[NATIVERTG_MAX_PLANES];   /* BPLCON1 delay, 0..15 pixels */
    uint16_t colour[NATIVERTG_COLOURS]; /* 0x0RGB, as in COLOR00-31 */
    nativertg_sprite sprite[NATIVERTG_MAX_SPRITES];
} nativertg_frame;

/* Builds the picture the chipset would show, one byte per pixel holding the
 * colour register (0..31), or 32..63 for an extra half-brite pixel (six
 * planes, one playfield). out has width * height bytes. */
void nativertg_compose(const nativertg_frame *f, uint8_t *out);

/* Pixel-art upscaling of an indexed picture: each pixel becomes 2x2, and a
 * corner takes its neighbours' colour where two edges meet, so diagonals and
 * curves come out smooth while flat areas and hard edges stay sharp. Indexed
 * in, indexed out, no new colours. out is (2w) x (2h). */
void nativertg_scale2x(const uint8_t *in, int w, int h, uint8_t *out);

/* Scale 2, 4 or 8 by repeated scale2x. tmp needs (factor*w)*(factor*h)
 * bytes, as does out. Returns 0, or -1 for a factor it does not do. */
int nativertg_upscale(const uint8_t *in, int w, int h, int factor,
                      uint8_t *tmp, uint8_t *out);

/* 0x0RGB to 0x00RRGGBB, each 4-bit level spread over 8 bits (0xF -> 0xFF). */
uint32_t nativertg_rgb12to24(uint16_t c);

/* Indexed picture to 0x00RRGGBB pixels through the frame's 32 colours,
 * 32..63 at half brightness. */
void nativertg_to_argb(const uint8_t *in, size_t pixels,
                       const uint16_t colour[NATIVERTG_COLOURS], uint32_t *out);

#endif
