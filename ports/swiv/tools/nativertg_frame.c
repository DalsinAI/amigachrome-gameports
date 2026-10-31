/* Renders one dumped chipset frame through nativertg, native size and
 * upscaled, as PPM. The frame file comes from swivharness.py --dump:
 *   "NRTG" w h depth stride colour[32] (big-endian 16-bit) then the planes.
 *   nativertg_frame FRAME FACTOR native.ppm upscaled.ppm
 * Copyright (c) 2026 AmigaChrome Team
 */
#include "nativertg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int rd16(FILE *fp)
{
    int a = fgetc(fp), b = fgetc(fp);
    return a << 8 | b;
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
    nativertg_frame f;
    char magic[4];
    FILE *fp;
    if (argc < 5) { fprintf(stderr, "usage: %s FRAME FACTOR native.ppm upscaled.ppm\n", argv[0]); return 2; }
    fp = fopen(argv[1], "rb");
    if (!fp || fread(magic, 1, 4, fp) != 4 || memcmp(magic, "NRTG", 4)) { fprintf(stderr, "not a frame file\n"); return 1; }
    memset(&f, 0, sizeof f);
    f.width = rd16(fp); f.height = rd16(fp); f.depth = rd16(fp);
    int stride = rd16(fp);
    f.dual_playfield = rd16(fp); f.pf2_priority = rd16(fp);
    for (int i = 0; i < NATIVERTG_COLOURS; i++) f.colour[i] = (uint16_t)rd16(fp);
    size_t plane = (size_t)stride * (size_t)f.height;
    uint8_t *planes = malloc(plane * (size_t)f.depth);
    if (!planes || fread(planes, 1, plane * (size_t)f.depth, fp) != plane * (size_t)f.depth) { fprintf(stderr, "short frame file\n"); return 1; }
    fclose(fp);
    for (int p = 0; p < f.depth; p++) { f.plane[p] = planes + plane * (size_t)p; f.stride[p] = stride; }
    f.pf1p = f.pf2p = 4;

    int factor = atoi(argv[2]);
    size_t n = (size_t)f.width * (size_t)f.height, big = n * (size_t)factor * (size_t)factor;
    uint8_t *idx = malloc(n), *tmp = malloc(big), *up = malloc(big);
    uint32_t *rgb = malloc(big * 4);
    if (!idx || !tmp || !up || !rgb) return 1;
    nativertg_compose(&f, idx);
    nativertg_to_argb(idx, n, f.colour, rgb);
    if (write_ppm(argv[3], rgb, f.width, f.height)) return 1;
    if (nativertg_upscale(idx, f.width, f.height, factor, tmp, up)) return 2;
    nativertg_to_argb(up, big, f.colour, rgb);
    return write_ppm(argv[4], rgb, f.width * factor, f.height * factor) ? 1 : 0;
}
