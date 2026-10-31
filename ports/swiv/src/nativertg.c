/* nativertg: an OCS/ECS chipset picture redrawn for an RTG screen.
 * Copyright (c) 2026 AmigaChrome Team
 */
#include "nativertg.h"

#include <string.h>

/* One bit of a plane at playfield x, after its BPLCON1 delay. */
static int plane_bit(const nativertg_frame *f, int p, int x, int y)
{
    int sx = x - f->scroll[p];
    if (sx < 0 || !f->plane[p])
        return 0;
    return f->plane[p][(size_t)y * (size_t)f->stride[p] + (size_t)(sx >> 3)] >> (7 - (sx & 7)) & 1;
}

/* A sprite's two-bit value at (x, y), 0 where it is clear. */
static int sprite_value(const nativertg_sprite *s, int x, int y)
{
    int dx = x - s->x, dy = y - s->y;
    if (s->height <= 0 || !s->data || dx < 0 || dx > 15 || dy < 0 || dy >= s->height)
        return 0;
    return (s->data[dy * 2] >> (15 - dx) & 1) | (s->data[dy * 2 + 1] >> (15 - dx) & 1) << 1;
}

void nativertg_compose(const nativertg_frame *f, uint8_t *out)
{
    int depth = f->depth < 1 ? 1 : f->depth > NATIVERTG_MAX_PLANES ? NATIVERTG_MAX_PLANES : f->depth;

    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            int idx = 0, front = 0;     /* front: 1 PF1 shows, 2 PF2 shows */

            if (f->dual_playfield) {
                /* odd planes (1, 3, 5) are playfield 1, colours 0-7; even
                 * planes (2, 4, 6) playfield 2, colours 8-15 */
                int v1 = 0, v2 = 0;
                for (int p = 0; p < depth; p += 2)
                    v1 |= plane_bit(f, p, x, y) << (p / 2);
                for (int p = 1; p < depth; p += 2)
                    v2 |= plane_bit(f, p, x, y) << (p / 2);
                if (v2 && (!v1 || f->pf2_priority)) {
                    idx = 8 + v2;
                    front = 2;
                } else if (v1) {
                    idx = v1;
                    front = 1;
                }
            } else {
                for (int p = 0; p < depth; p++)
                    idx |= plane_bit(f, p, x, y) << p;
                /* single playfield counts as playfield 1 for sprite priority,
                 * as the runtime's Denise does (gfx.c) */
                front = idx ? 1 : 0;
            }

            /* Sprites: the lowest-numbered pair in front; a pair shows over a
             * playfield when its number is below that playfield's priority. */
            for (int n = 0; n < NATIVERTG_MAX_SPRITES; n += 2) {
                const nativertg_sprite *se = &f->sprite[n], *so = &f->sprite[n + 1];
                int c0 = sprite_value(se, x, y), c1 = sprite_value(so, x, y);
                int pair = n / 2, col;
                if (so->attached && so->height > 0)
                    col = (c0 | c1 << 2) ? 16 + (c0 | c1 << 2) : 0;
                else if (c0)
                    col = 16 + pair * 4 + c0;
                else if (c1)
                    col = 16 + pair * 4 + c1;
                else
                    col = 0;
                if (!col)
                    continue;
                if ((front == 1 && pair >= f->pf1p) || (front == 2 && pair >= f->pf2p))
                    continue;
                idx = col;
                break;
            }
            out[(size_t)y * (size_t)f->width + (size_t)x] = (uint8_t)idx;
        }
    }
}

void nativertg_scale2x(const uint8_t *in, int w, int h, uint8_t *out)
{
    size_t ow = (size_t)w * 2;
    for (int y = 0; y < h; y++) {
        const uint8_t *row = in + (size_t)y * (size_t)w;
        const uint8_t *up = y > 0 ? row - w : row;
        const uint8_t *dn = y < h - 1 ? row + w : row;
        uint8_t *o0 = out + (size_t)y * 2 * ow, *o1 = o0 + ow;
        for (int x = 0; x < w; x++) {
            int l = x > 0 ? x - 1 : x, r = x < w - 1 ? x + 1 : x;
            uint8_t c = row[x], n = up[x], s = dn[x], wv = row[l], e = row[r];
            uint8_t a = c, b = c, d = c, g = c;
            /* a corner turns only where the two neighbours beside it match
             * each other and the opposite pair differ: a true diagonal edge */
            if (n != s && wv != e) {
                if (wv == n) a = wv;
                if (n == e) b = e;
                if (wv == s) d = wv;
                if (s == e) g = e;
            }
            o0[x * 2] = a;
            o0[x * 2 + 1] = b;
            o1[x * 2] = d;
            o1[x * 2 + 1] = g;
        }
    }
}

int nativertg_upscale(const uint8_t *in, int w, int h, int factor,
                      uint8_t *tmp, uint8_t *out)
{
    if (factor == 2) {
        nativertg_scale2x(in, w, h, out);
        return 0;
    }
    if (factor == 4) {
        nativertg_scale2x(in, w, h, tmp);
        nativertg_scale2x(tmp, w * 2, h * 2, out);
        return 0;
    }
    if (factor == 8) {
        nativertg_scale2x(in, w, h, out);
        nativertg_scale2x(out, w * 2, h * 2, tmp);
        nativertg_scale2x(tmp, w * 4, h * 4, out);
        return 0;
    }
    return -1;
}

uint32_t nativertg_rgb12to24(uint16_t c)
{
    uint32_t r = c >> 8 & 15, g = c >> 4 & 15, b = c & 15;
    return (r * 17) << 16 | (g * 17) << 8 | b * 17;
}

void nativertg_to_argb(const uint8_t *in, size_t pixels,
                       const uint16_t colour[NATIVERTG_COLOURS], uint32_t *out)
{
    uint32_t pal[64];
    for (int i = 0; i < NATIVERTG_COLOURS; i++) {
        pal[i] = nativertg_rgb12to24(colour[i]);
        /* extra half-brite: six planes, single playfield, colours 32-63 */
        pal[i + 32] = pal[i] >> 1 & 0x7f7f7f;
    }
    for (size_t i = 0; i < pixels; i++)
        out[i] = pal[in[i] & 63];
}
