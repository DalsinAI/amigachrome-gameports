#!/usr/bin/env python3
"""Make AstroMenace's TGA reader endian-stable.

The TGA header stores width and height as little-endian 16-bit integers.
Upstream reads those bytes directly into uint16_t, which is correct on x86 but
byte-swaps the values on big-endian 68k.  A 64x64 image then becomes
16384x16384 and the decoder tries to allocate hundreds of megabytes before the
OpenGL upload.  Read the two header bytes explicitly instead.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
tga_cpp = source / "src" / "core" / "texture" / "texture_tga.cpp"
text = tga_cpp.read_text(encoding="utf-8")

old = """    // read the width, height and bpp
    uint16_t TmpReadData;
    if (pFile->fread(&TmpReadData, sizeof(TmpReadData), 1) != 1) {
        return ERR_FILE_IO;
    }
    DWidth = TmpReadData;
    if (pFile->fread(&TmpReadData, sizeof(TmpReadData), 1) != 1) {
        return ERR_FILE_IO;
    }
    DHeight = TmpReadData;
    if (pFile->fread(&tmpBits, sizeof(tmpBits), 1) != 1) {
        return ERR_FILE_IO;
    }
"""

new = """    // TGA width and height are always little-endian on disk.  Reading them
    // directly into uint16_t makes a 64x64 image appear as 16384x16384 on
    // big-endian 68k and can trigger a multi-hundred-megabyte allocation.
    uint8_t TmpReadData[2];
    if (pFile->fread(TmpReadData, sizeof(TmpReadData), 1) != 1) {
        return ERR_FILE_IO;
    }
    DWidth = static_cast<int>(TmpReadData[0]) |
             (static_cast<int>(TmpReadData[1]) << 8);
    if (pFile->fread(TmpReadData, sizeof(TmpReadData), 1) != 1) {
        return ERR_FILE_IO;
    }
    DHeight = static_cast<int>(TmpReadData[0]) |
              (static_cast<int>(TmpReadData[1]) << 8);
    if (pFile->fread(&tmpBits, sizeof(tmpBits), 1) != 1) {
        return ERR_FILE_IO;
    }

    if (DWidth <= 0 || DHeight <= 0) {
        std::cerr << __func__ << \"(): invalid TGA dimensions.\\n\";
        return ERR_FILE_IO;
    }
"""

if "TGA width and height are always little-endian" not in text:
    if old not in text:
        raise RuntimeError("AstroMenace TGA width/height marker missing")
    text = text.replace(old, new, 1)

tga_cpp.write_text(text, encoding="utf-8")
print("patched AstroMenace TGA dimensions to explicit little-endian bytes")
