#!/usr/bin/env python3
"""Make AstroMenace VFS v1.6 explicitly little-endian.

Upstream serialises uint16_t/uint32_t values by writing their native memory
representation.  Existing AstroMenace assets, including models.pack, were
built on little-endian hosts.  A 68k Amiga therefore reads their table offset,
entry offsets and sizes backwards; conversely, a VFS made on the Amiga is not
portable to the other supported hosts.

VFS v1.6 is already an on-disk interchange format.  Preserve the established
little-endian files and make both readers and writers explicit so the same
assets work on x86, ARM and big-endian m68k.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
vfs = source / "src" / "core" / "vfs" / "vfs.cpp"
text = vfs.read_text(encoding="utf-8")

marker = "AstroMenace VFS v1.6 integers are little-endian on disk"
if marker in text:
    print("AstroMenace VFS little-endian patch already applied")
    raise SystemExit(0)


def replace_once(old: str, new: str, label: str) -> None:
    global text
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{label}: expected one source marker, found {count}")
    text = text.replace(old, new, 1)


anchor = "constexpr unsigned int FixedHeaderPartSize = 4 + 4 + 4; /*VFS_ + ver + build*/\n"
helpers = r'''constexpr unsigned int FixedHeaderPartSize = 4 + 4 + 4; /*VFS_ + ver + build*/

// AstroMenace VFS v1.6 integers are little-endian on disk.  Do not write
// native integer representations: classic 68k Amigas are big-endian while
// the existing VFS assets were produced on little-endian hosts.
static void WriteLE16(std::ostream &Stream, uint16_t Value)
{
    const unsigned char Bytes[2]{
        static_cast<unsigned char>(Value & 0xffu),
        static_cast<unsigned char>((Value >> 8) & 0xffu)
    };
    Stream.write(reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
}

static void WriteLE32(std::ostream &Stream, uint32_t Value)
{
    const unsigned char Bytes[4]{
        static_cast<unsigned char>(Value & 0xffu),
        static_cast<unsigned char>((Value >> 8) & 0xffu),
        static_cast<unsigned char>((Value >> 16) & 0xffu),
        static_cast<unsigned char>((Value >> 24) & 0xffu)
    };
    Stream.write(reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
}

static void ReadLE16(std::istream &Stream, uint16_t &Value)
{
    unsigned char Bytes[2]{};
    Stream.read(reinterpret_cast<char*>(Bytes), sizeof(Bytes));
    if (!Stream.fail()) {
        Value = static_cast<uint16_t>(Bytes[0])
              | static_cast<uint16_t>(static_cast<uint16_t>(Bytes[1]) << 8);
    }
}

static void ReadLE32(std::istream &Stream, uint32_t &Value)
{
    unsigned char Bytes[4]{};
    Stream.read(reinterpret_cast<char*>(Bytes), sizeof(Bytes));
    if (!Stream.fail()) {
        Value = static_cast<uint32_t>(Bytes[0])
              | (static_cast<uint32_t>(Bytes[1]) << 8)
              | (static_cast<uint32_t>(Bytes[2]) << 16)
              | (static_cast<uint32_t>(Bytes[3]) << 24);
    }
}
'''
replace_once(anchor, helpers, "helper insertion")

replace_once(
    "            WritableVFS->File.write(reinterpret_cast<char*>(&tmpNameSize), sizeof(tmpNameSize));",
    "            WriteLE16(WritableVFS->File, tmpNameSize);",
    "table name length write",
)
replace_once(
    "            WritableVFS->File.write(reinterpret_cast<const char*>(&tmpEntry.second.Offset),\n"
    "                                    sizeof(tmpEntry.second.Offset));",
    "            WriteLE32(WritableVFS->File, tmpEntry.second.Offset);",
    "table offset write",
)
replace_once(
    "            WritableVFS->File.write(reinterpret_cast<const char*>(&tmpEntry.second.Size),\n"
    "                                    sizeof(tmpEntry.second.Size));",
    "            WriteLE32(WritableVFS->File, tmpEntry.second.Size);",
    "table size write",
)
replace_once(
    "    WritableVFS->File.write(reinterpret_cast<char*>(&FileTableOffset), sizeof(FileTableOffset));",
    "    WriteLE32(WritableVFS->File, FileTableOffset);",
    "updated table offset write",
)
replace_once(
    "    TempVFS->File.write(reinterpret_cast<char*>(&BuildNumber), 4 /*fixed 4 bytes size*/);",
    "    WriteLE32(TempVFS->File, static_cast<uint32_t>(BuildNumber));",
    "build number write",
)
replace_once(
    "    TempVFS->File.write(reinterpret_cast<char*>(&FileTableOffset), sizeof(FileTableOffset));",
    "    WriteLE32(TempVFS->File, FileTableOffset);",
    "initial table offset write",
)
replace_once(
    "    unsigned int vfsBuildNumber;\n"
    "    VFSList.front()->File.read(reinterpret_cast<char*>(&vfsBuildNumber), 4 /*fixed 4 bytes size*/);",
    "    uint32_t vfsBuildNumber{0};\n"
    "    ReadLE32(VFSList.front()->File, vfsBuildNumber);",
    "build number read",
)
replace_once(
    "    VFSList.front()->File.read(reinterpret_cast<char*>(&FileTableOffset), sizeof(FileTableOffset));",
    "    ReadLE32(VFSList.front()->File, FileTableOffset);",
    "table offset read",
)
replace_once(
    "        VFSList.front()->File.read(reinterpret_cast<char*>(&tmpNameSize), sizeof(tmpNameSize));",
    "        ReadLE16(VFSList.front()->File, tmpNameSize);",
    "table name length read",
)
replace_once(
    "        VFSList.front()->File.read(reinterpret_cast<char*>(&VFSEntriesMap[tmpName].Offset),\n"
    "                                   sizeof(VFSEntriesMap[tmpName].Offset));",
    "        ReadLE32(VFSList.front()->File, VFSEntriesMap[tmpName].Offset);",
    "entry offset read",
)
replace_once(
    "        VFSList.front()->File.read(reinterpret_cast<char*>(&VFSEntriesMap[tmpName].Size),\n"
    "                                   sizeof(VFSEntriesMap[tmpName].Size));",
    "        ReadLE32(VFSList.front()->File, VFSEntriesMap[tmpName].Size);",
    "entry size read",
)

vfs.write_text(text, encoding="utf-8")
print("patched AstroMenace VFS v1.6 to explicit little-endian integers")
