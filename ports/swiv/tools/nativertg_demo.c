/* Draws a made-up 320x256 five-plane scene (circles, diagonals, text-sized
 * steps) through nativertg and writes it 1x and upscaled as PPM, to look at
 * the scaler by eye. No game data. Copyright (c) 2026 AmigaChrome Team
 *   nativertg_demo FACTOR native.ppm upscaled.ppm
 */
#include "nativertg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 320
#define H 256
#define STRIDE (W / 8)

static void put(uint8_t planes[5][STRIDE * H], int x, int y, int c)
{
    if (x < 0 || y < 0 || x >= W || y >= H)
        return;
    for (int p = 0; p < 5; p++) {
        uint8_t m = (uint8_t)(0x80 >> (x & 7));
        if (c >> p & 1) planes[p][y * STRIDE + x / 8] |= m;
        else planes[p][y * STRIDE + x / 8] &= (uint8_t)~m;
    }
}

static int write_ppm(const char *path, const uint32_t *px, int w, int h)
{
    FILE *fp = fopen(path, "wb");
    if (!fp) return -1;
    fprintf(fp, "P6\n%d %d\n255\n", w, h);
    for (long i = 0; i < (long)w * h; i++) {
        unsigned char rgb[3] = { (unsigned char)(px[i] >> 16), (unsigned char)(px[i] >> 8), (unsigned char)px[i] };
        fwrite(rgb, 1, 3, fp);
    }
    return fclose(fp);
}

int main(int argc, char **argv)
{
    static uint8_t planes[5][STRIDE * H];
    static uint16_t ship[16 * 2];
    nativertg_frame f;
    int factor = argc > 1 ? atoi(argv[1]) : 4;
    if (argc < 4) { fprintf(stderr, "usage: %s FACTOR native.ppm upscaled.ppm\n", argv[0]); return 2; }

    memset(&f, 0, sizeof f);
    f.width = W; f.height = H; f.depth = 5;
    for (int p = 0; p < 5; p++) { f.plane[p] = planes[p]; f.stride[p] = STRIDE; }
    for (int i = 0; i < 32; i++)
        f.colour[i] = (uint16_t)(((i * 5) & 15) << 8 | ((i * 3 + 4) & 15) << 4 | ((15 - i) & 15));
    f.colour[0] = 0x0123;

    /* ground: stripes and diagonal roads */
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int c = ((x + y) / 24 & 1) ? 2 : 3;
            if (abs((x - y) % 64) < 3) c = 7;
            put(planes, x, y, c);
        }
    /* round buildings and rings */
    for (int k = 0; k < 6; k++) {
        int cx = 40 + k * 50, cy = 60 + (k % 3) * 60, r = 10 + k * 3;
        for (int y = -r; y <= r; y++)
            for (int x = -r; x <= r; x++) {
                int d = x * x + y * y;
                if (d <= r * r) put(planes, cx + x, cy + y, d > (r - 3) * (r - 3) ? 9 : 12 + k);
            }
    }
    /* a sprite: a small round craft */
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int dx = x - 7, dy = y - 7, d = dx * dx + dy * dy;
            int v = d < 12 ? 3 : d < 40 ? 1 : d < 56 ? 2 : 0;
            ship[y * 2] |= (uint16_t)((v & 1) << (15 - x));
            ship[y * 2 + 1] |= (uint16_t)((v >> 1 & 1) << (15 - x));
        }
    f.sprite[0] = (nativertg_sprite){ 150, 200, 16, ship, 0 };
    f.colour[17] = 0x0ccc; f.colour[18] = 0x0f80; f.colour[19] = 0x0fff;
    f.pf1p = 4;

    size_t n = (size_t)W * H, big = n * (size_t)factor * (size_t)factor;
    uint8_t *idx = malloc(n), *tmp = malloc(big), *up = malloc(big);
    uint32_t *rgb = malloc(big * 4);
    if (!idx || !tmp || !up || !rgb) return 1;
    nativertg_compose(&f, idx);
    nativertg_to_argb(idx, n, f.colour, rgb);
    if (write_ppm(argv[2], rgb, W, H)) return 1;
    if (nativertg_upscale(idx, W, H, factor, tmp, up)) return 2;
    nativertg_to_argb(up, big, f.colour, rgb);
    if (write_ppm(argv[3], rgb, W * factor, H * factor)) return 1;
    return 0;
}
