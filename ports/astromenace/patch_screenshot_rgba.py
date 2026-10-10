#!/usr/bin/env python3
"""Make AstroMenace screenshots match OpenGPU's RGBA readback contract.

The OpenGPU/WebGL path reads pixels as RGBA. Asking it for GL_RGB while
allocating three bytes per pixel can leave the saved image byte-shifted (and
risks an overwrite if the bridge returns four components). Read four bytes per
pixel explicitly and copy RGB into SDL's 24-bit surface.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
misc_cpp = source / "src" / "core" / "graphics" / "misc.cpp"
text = misc_cpp.read_text(encoding="utf-8")

old = '''    // fixed for BMP format, supported by libSDL
    constexpr uint8_t BitsPerPixels = 24;
    constexpr uint8_t ChannelsNumber = 3;

    SDL_Surface *tmpSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, BitsPerPixels,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                                   0x000000FF, 0x0000FF00, 0x00FF0000,
#else
                                                   0x00FF0000, 0x0000FF00, 0x000000FF,
#endif
                                                   0);
    if (!tmpSurface) {
        return ERR_MEM;
    }

    // std::unique_ptr, we need only memory allocation without container's features
    // don't use std::vector here, since it allocates AND value-initializes
    std::unique_ptr<uint8_t[]> Pixels{new uint8_t[ChannelsNumber * Width * Height]};

    glReadPixels(0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, Pixels.get());

    // flip pixels in proper order
    for (int i = 0; i < Height; i++) {
        memcpy((uint8_t *)tmpSurface->pixels + tmpSurface->pitch * i,
               Pixels.get() + ChannelsNumber * Width * (Height - i - 1),
               Width * ChannelsNumber);
    }
'''

new = '''    // fixed for BMP format, supported by libSDL
    constexpr uint8_t BitsPerPixels = 24;
    constexpr uint8_t OutputChannels = 3;
#ifdef AMIGACHROME
    // OpenGPU/WebGL readback is RGBA. Keep the read buffer at four bytes per
    // pixel, then discard alpha while copying into SDL's 24-bit BMP surface.
    constexpr uint8_t ReadChannels = 4;
#else
    constexpr uint8_t ReadChannels = 3;
#endif

    SDL_Surface *tmpSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, Width, Height, BitsPerPixels,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                                                   0x000000FF, 0x0000FF00, 0x00FF0000,
#else
                                                   0x00FF0000, 0x0000FF00, 0x000000FF,
#endif
                                                   0);
    if (!tmpSurface) {
        return ERR_MEM;
    }

    // std::unique_ptr, we need only memory allocation without container's features
    // don't use std::vector here, since it allocates AND value-initializes
    std::unique_ptr<uint8_t[]> Pixels{new uint8_t[ReadChannels * Width * Height]};

#ifdef AMIGACHROME
    glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels.get());
#else
    glReadPixels(0, 0, Width, Height, GL_RGB, GL_UNSIGNED_BYTE, Pixels.get());
#endif

    // Flip rows and copy to SDL's packed 24-bit surface.
    for (int y = 0; y < Height; ++y) {
        uint8_t *Dst = static_cast<uint8_t *>(tmpSurface->pixels) + tmpSurface->pitch * y;
        const uint8_t *Src = Pixels.get() + ReadChannels * Width * (Height - y - 1);
        for (int x = 0; x < Width; ++x) {
            Dst[x * OutputChannels + 0] = Src[x * ReadChannels + 0];
            Dst[x * OutputChannels + 1] = Src[x * ReadChannels + 1];
            Dst[x * OutputChannels + 2] = Src[x * ReadChannels + 2];
        }
    }
'''

if "OpenGPU/WebGL readback is RGBA" not in text:
    if old not in text:
        raise RuntimeError("AstroMenace screenshot readback marker missing")
    text = text.replace(old, new, 1)

misc_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace screenshot readback to RGBA on OpenGPU")
