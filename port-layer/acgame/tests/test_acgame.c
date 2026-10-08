#include "acgame/acgame.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_c2p_roundtrip(void) {
    enum { W = 16, H = 3, PP = W / 8 };
    uint8_t chunky[W * H];
    uint8_t roundtrip[W * H];
    uint8_t plane_mem[8][PP * H];
    uint8_t *planes[8];
    const uint8_t *cplanes[8];
    unsigned i, p;
    for (i = 0; i < W * H; ++i) chunky[i] = (uint8_t)((i * 37u + 11u) & 255u);
    memset(roundtrip, 0, sizeof roundtrip);
    memset(plane_mem, 0, sizeof plane_mem);
    for (p = 0; p < 8; ++p) { planes[p] = plane_mem[p]; cplanes[p] = plane_mem[p]; }
    assert(acgame_c2p8_ref(chunky, W, planes, PP, W, H) == 0);
    assert(acgame_p2c8_ref(roundtrip, W, cplanes, PP, W, H) == 0);
    assert(memcmp(chunky, roundtrip, sizeof chunky) == 0);
}

static void test_c2p_bit_order(void) {
    uint8_t src[8] = {1,0,0,0,0,0,0,1};
    uint8_t mem[8] = {0};
    uint8_t *planes[8] = {&mem[0],&mem[1],&mem[2],&mem[3],&mem[4],&mem[5],&mem[6],&mem[7]};
    assert(acgame_c2p8_ref(src, 8, planes, 1, 8, 1) == 0);
    assert(mem[0] == 0x81);
    for (unsigned p = 1; p < 8; ++p) assert(mem[p] == 0);
}

static void test_ham8_controls(void) {
    acgame_rgb8 pal[64] = {{0}};
    acgame_rgb8 src[4] = {
        {0,0,0},
        {252,0,0},
        {252,0,252},
        {252,252,252}
    };
    acgame_rgb8 out[4];
    uint8_t codes[4];
    assert(acgame_ham8_encode_row(src, 4, pal, codes, out) == 0);
    assert(codes[0] == 0x00);
    assert(codes[1] == 0xbf);
    assert(codes[2] == 0x7f);
    assert(codes[3] == 0xff);
    for (unsigned i = 0; i < 4; ++i) {
        assert(out[i].r == src[i].r && out[i].g == src[i].g && out[i].b == src[i].b);
    }
}

static void test_ham8_palette_reset(void) {
    acgame_rgb8 pal[64] = {{0}};
    acgame_rgb8 src[2];
    acgame_rgb8 out[2];
    uint8_t codes[2];
    pal[5] = (acgame_rgb8){17, 34, 51};
    src[0] = pal[5];
    src[1] = pal[5];
    assert(acgame_ham8_encode_row(src, 2, pal, codes, out) == 0);
    assert(codes[0] == 5);
    assert(out[0].r == 17 && out[0].g == 34 && out[0].b == 51);
}

int main(void) {
    test_c2p_roundtrip();
    test_c2p_bit_order();
    test_ham8_controls();
    test_ham8_palette_reset();
    puts("acgame reference tests: OK");
    return 0;
}

