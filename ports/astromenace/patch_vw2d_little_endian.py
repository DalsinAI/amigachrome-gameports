#!/usr/bin/env python3
"""Make AstroMenace's VW2D texture format explicitly little-endian.

The shipped .vw2d textures were produced on little-endian hosts. Upstream
checks the VW2D signature on either byte order, but then reads width, height
and channel count directly into native integers. On a big-endian 68k a normal
texture can therefore become an enormous allocation and abort while assets
are loading.

VW2D is an on-disk interchange format. Preserve the established little-endian
representation, validate dimensions before allocation, and write the same
portable representation on every host.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
texture = source / "src" / "core" / "texture" / "texture.cpp"
text = texture.read_text(encoding="utf-8")

marker = "AstroMenace VW2D dimensions are little-endian on disk"
if marker in text:
    print("AstroMenace VW2D little-endian patch already applied")
    raise SystemExit(0)

if "#include <limits>" not in text:
    include_anchor = "#include <fstream>\n"
    if include_anchor not in text:
        raise RuntimeError("texture include anchor missing")
    text = text.replace(include_anchor, include_anchor + "#include <limits>\n", 1)

helper_anchor = """// Map with all loaded textures.
std::unordered_map<GLtexture, sTexture> TexturesIDtoDataMap;

} // unnamed namespace
"""
helpers = r'''// Map with all loaded textures.
std::unordered_map<GLtexture, sTexture> TexturesIDtoDataMap;

// AstroMenace VW2D dimensions are little-endian on disk. Existing assets use
// that representation; do not read or write native big-endian 68k integers.
static bool ReadVW2DLE32(cFILE &File, uint32_t &Value)
{
    unsigned char Bytes[4]{};
    if (File.fread(Bytes, 1, sizeof(Bytes)) != sizeof(Bytes)) {
        return false;
    }
    Value = static_cast<uint32_t>(Bytes[0])
          | (static_cast<uint32_t>(Bytes[1]) << 8)
          | (static_cast<uint32_t>(Bytes[2]) << 16)
          | (static_cast<uint32_t>(Bytes[3]) << 24);
    return true;
}

static void WriteVW2DLE32(std::ostream &Stream, uint32_t Value)
{
    const unsigned char Bytes[4]{
        static_cast<unsigned char>(Value & 0xffu),
        static_cast<unsigned char>((Value >> 8) & 0xffu),
        static_cast<unsigned char>((Value >> 16) & 0xffu),
        static_cast<unsigned char>((Value >> 24) & 0xffu)
    };
    Stream.write(reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
}

} // unnamed namespace
'''
if helper_anchor not in text:
    raise RuntimeError("texture helper insertion anchor missing")
text = text.replace(helper_anchor, helpers, 1)

write_old = """    FileVW2D.write(reinterpret_cast<char*>(&tmpWidth), sizeof(tmpWidth));
    FileVW2D.write(reinterpret_cast<char*>(&tmpHeight), sizeof(tmpHeight));
    FileVW2D.write(reinterpret_cast<char*>(&tmpChanels), sizeof(tmpChanels));
    FileVW2D.write(reinterpret_cast<char*>(tmpPixelsArray.get()), tmpWidth * tmpHeight * tmpChanels);
"""
write_new = """    WriteVW2DLE32(FileVW2D, static_cast<uint32_t>(tmpWidth));
    WriteVW2DLE32(FileVW2D, static_cast<uint32_t>(tmpHeight));
    WriteVW2DLE32(FileVW2D, static_cast<uint32_t>(tmpChanels));
    FileVW2D.write(reinterpret_cast<char*>(tmpPixelsArray.get()),
                   static_cast<std::streamsize>(tmpWidth) * tmpHeight * tmpChanels);
"""
if text.count(write_old) != 1:
    raise RuntimeError("VW2D writer marker missing")
text = text.replace(write_old, write_new, 1)

read_old = r'''    case eLoadTextureAs::VW2D:
        // check "VW2D" sign
        {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            constexpr uint32_t SignVW2D = (uint32_t('D') << 8*3) + (uint32_t('2') << 8*2) + (uint32_t('W') << 8) + uint32_t('V'); // `V` `W` `2` `D`
#else
            constexpr uint32_t SignVW2D = (uint32_t('V') << 8*3) + (uint32_t('W') << 8*2) + (uint32_t('2') << 8) + uint32_t('D'); // `V` `W` `2` `D`
#endif
            uint32_t Sign;
            if (pFile->fread(&Sign, 4, 1) != 1 ||
                Sign != SignVW2D) {
                return 0;
            }
        }
        if (pFile->fread(&DWidth, sizeof(DWidth), 1) != 1 ||
            pFile->fread(&DHeight, sizeof(DHeight), 1) != 1 ||
            pFile->fread(&DChanels, sizeof(DChanels), 1) != 1) {
            return 0;
        }
        tmpPixelsArray.reset(new uint8_t[DWidth * DHeight * DChanels]);
        if (pFile->fread(tmpPixelsArray.get(), DWidth * DHeight * DChanels, 1) != 1) {
            return 0;
        }
        break;
'''
read_new = r'''    case eLoadTextureAs::VW2D:
        {
            unsigned char Sign[4]{};
            if (pFile->fread(Sign, 1, sizeof(Sign)) != sizeof(Sign) ||
                std::memcmp(Sign, "VW2D", sizeof(Sign)) != 0) {
                return 0;
            }

            uint32_t Width{0};
            uint32_t Height{0};
            uint32_t Channels{0};
            if (!ReadVW2DLE32(*pFile, Width) ||
                !ReadVW2DLE32(*pFile, Height) ||
                !ReadVW2DLE32(*pFile, Channels) ||
                Width == 0 || Height == 0 ||
                Width > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
                Height > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
                (Channels != 3 && Channels != 4)) {
                return 0;
            }

            const size_t PixelCount = static_cast<size_t>(Width)
                                    * static_cast<size_t>(Height);
            if (PixelCount > std::numeric_limits<size_t>::max() / Channels) {
                return 0;
            }
            const size_t ByteCount = PixelCount * Channels;

            DWidth = static_cast<int>(Width);
            DHeight = static_cast<int>(Height);
            DChanels = static_cast<int>(Channels);
            tmpPixelsArray.reset(new uint8_t[ByteCount]);
            if (pFile->fread(tmpPixelsArray.get(), 1, ByteCount) != ByteCount) {
                return 0;
            }
        }
        break;
'''
if text.count(read_old) != 1:
    raise RuntimeError("VW2D reader marker missing")
text = text.replace(read_old, read_new, 1)

texture.write_text(text, encoding="utf-8")
print("patched AstroMenace VW2D texture format to explicit little-endian dimensions")
