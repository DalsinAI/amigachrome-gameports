#!/usr/bin/env python3
"""Make AstroMenace's VW3D model format explicitly little-endian.

The shipped models.pack was produced on a little-endian host.  Upstream checks
the four-byte VW3D signature correctly on either byte order, but then reads
all chunk counts, formats, floats, vertex data and indices directly into native
objects.  On a big-endian 68k this turns valid model metadata into nonsense,
so every model fails during LoadAllGameAssets and the game aborts before its
main menu.

VW3D is an on-disk interchange format.  Preserve the established little-endian
representation and decode/encode it explicitly on every host.
"""

from pathlib import Path
import sys

source = Path(sys.argv[1]).resolve()
model = source / "src" / "core" / "model3d" / "model3d.cpp"
text = model.read_text(encoding="utf-8")

marker = "AstroMenace VW3D integers and floats are little-endian on disk"
if marker in text:
    print("AstroMenace VW3D little-endian patch already applied")
    raise SystemExit(0)

if "#include <limits>" not in text:
    include_anchor = "#include <fstream>\n"
    if include_anchor not in text:
        raise RuntimeError("model3d include anchor missing")
    text = text.replace(include_anchor, include_anchor + "#include <limits>\n", 1)

namespace_anchor = """// All loaded models.
std::unordered_map<std::string, std::shared_ptr<cModel3DWrapper>> ModelsMap;

} // unnamed namespace
"""
helpers = r'''// All loaded models.
std::unordered_map<std::string, std::shared_ptr<cModel3DWrapper>> ModelsMap;

// AstroMenace VW3D integers and floats are little-endian on disk.  Existing
// model assets use that representation; never read or write native 68k words.
static bool ReadVW3DLE32(cFILE &File, uint32_t &Value)
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

static bool ReadVW3DLEFloat(cFILE &File, float &Value)
{
    static_assert(sizeof(float) == sizeof(uint32_t), "VW3D requires 32-bit float");
    uint32_t Bits{0};
    if (!ReadVW3DLE32(File, Bits)) {
        return false;
    }
    std::memcpy(&Value, &Bits, sizeof(Value));
    return true;
}

static bool ReadVW3DLEFloatArray(cFILE &File, float *Values, size_t Count)
{
    if (!Values && Count) {
        return false;
    }
    for (size_t i = 0; i < Count; ++i) {
        if (!ReadVW3DLEFloat(File, Values[i])) {
            return false;
        }
    }
    return true;
}

static bool ReadVW3DLE32Array(cFILE &File, unsigned *Values, size_t Count)
{
    static_assert(sizeof(unsigned) == sizeof(uint32_t), "VW3D requires 32-bit unsigned");
    if (!Values && Count) {
        return false;
    }
    for (size_t i = 0; i < Count; ++i) {
        uint32_t Value{0};
        if (!ReadVW3DLE32(File, Value)) {
            return false;
        }
        Values[i] = static_cast<unsigned>(Value);
    }
    return true;
}

static void WriteVW3DLE32(std::ostream &Stream, uint32_t Value)
{
    const unsigned char Bytes[4]{
        static_cast<unsigned char>(Value & 0xffu),
        static_cast<unsigned char>((Value >> 8) & 0xffu),
        static_cast<unsigned char>((Value >> 16) & 0xffu),
        static_cast<unsigned char>((Value >> 24) & 0xffu)
    };
    Stream.write(reinterpret_cast<const char*>(Bytes), sizeof(Bytes));
}

static void WriteVW3DLEFloat(std::ostream &Stream, float Value)
{
    static_assert(sizeof(float) == sizeof(uint32_t), "VW3D requires 32-bit float");
    uint32_t Bits{0};
    std::memcpy(&Bits, &Value, sizeof(Bits));
    WriteVW3DLE32(Stream, Bits);
}

} // unnamed namespace
'''
if namespace_anchor not in text:
    raise RuntimeError("model3d helper insertion anchor missing")
text = text.replace(namespace_anchor, helpers, 1)

load_start = text.index("bool cModel3DWrapper::LoadVW3D(const std::string &FileName)\n{")
save_comment = text.index("/*\n * Save VW3D 3D models format.\n */", load_start)
new_load = r'''bool cModel3DWrapper::LoadVW3D(const std::string &FileName)
{
    if (FileName.empty()) {
        return false;
    }

    std::unique_ptr<cFILE> File = vw_fopen(FileName);
    if (!File) {
        return false;
    }

    unsigned char Sign[4]{};
    if (File->fread(Sign, 1, sizeof(Sign)) != sizeof(Sign) ||
        std::memcmp(Sign, "VW3D", sizeof(Sign)) != 0) {
        return false;
    }

    uint32_t ChunkArraySize{0};
    if (!ReadVW3DLE32(*File, ChunkArraySize) || ChunkArraySize == 0 ||
        ChunkArraySize > static_cast<uint32_t>(std::numeric_limits<int>::max())) {
        return false;
    }

    Chunks.resize(static_cast<size_t>(ChunkArraySize));
    GlobalIndexArrayCount = 0;

    for (auto &tmpChunk : Chunks) {
        tmpChunk.RangeStart = GlobalIndexArrayCount;

        uint32_t VertexFormat{0};
        uint32_t VertexStride{0};
        uint32_t VertexQuantity{0};
        if (!ReadVW3DLE32(*File, VertexFormat) ||
            !ReadVW3DLE32(*File, VertexStride) ||
            !ReadVW3DLE32(*File, VertexQuantity) ||
            VertexStride == 0 || VertexQuantity == 0 ||
            VertexStride > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
            VertexQuantity > std::numeric_limits<unsigned>::max() - GlobalIndexArrayCount) {
            return false;
        }

        tmpChunk.VertexFormat = static_cast<int>(VertexFormat);
        tmpChunk.VertexStride = static_cast<int>(VertexStride);
        tmpChunk.VertexQuantity = static_cast<unsigned>(VertexQuantity);
        GlobalIndexArrayCount += tmpChunk.VertexQuantity;

        if (!ReadVW3DLEFloat(*File, tmpChunk.Location.x) ||
            !ReadVW3DLEFloat(*File, tmpChunk.Location.y) ||
            !ReadVW3DLEFloat(*File, tmpChunk.Location.z) ||
            !ReadVW3DLEFloat(*File, tmpChunk.Rotation.x) ||
            !ReadVW3DLEFloat(*File, tmpChunk.Rotation.y) ||
            !ReadVW3DLEFloat(*File, tmpChunk.Rotation.z)) {
            return false;
        }

        tmpChunk.DrawType = eModel3DDrawType::Normal;
        tmpChunk.NeedReleaseOpenGLBuffers = false;
        tmpChunk.VBO = 0;
        tmpChunk.IBO = 0;
        tmpChunk.VAO = 0;
    }

    uint32_t VertexArrayCount{0};
    if (!ReadVW3DLE32(*File, VertexArrayCount)) {
        return false;
    }
    GlobalVertexArrayCount = static_cast<unsigned>(VertexArrayCount);

    const size_t VertexStride = static_cast<size_t>(Chunks[0].VertexStride);
    const size_t VertexCount = static_cast<size_t>(GlobalVertexArrayCount);
    if (VertexCount > std::numeric_limits<size_t>::max() / VertexStride) {
        return false;
    }
    const size_t FloatCount = VertexCount * VertexStride;

    GlobalVertexArray.reset(new float[FloatCount], std::default_delete<float[]>());
    if (!ReadVW3DLEFloatArray(*File, GlobalVertexArray.get(), FloatCount)) {
        return false;
    }

    GlobalIndexArray.reset(new unsigned[GlobalIndexArrayCount], std::default_delete<unsigned[]>());
    if (!ReadVW3DLE32Array(*File, GlobalIndexArray.get(), GlobalIndexArrayCount)) {
        return false;
    }

    for (auto &tmpChunk : Chunks) {
        tmpChunk.VertexArray = GlobalVertexArray;
        tmpChunk.IndexArray = GlobalIndexArray;
    }

    vw_fclose(File);
    MetadataInitialization();
    return true;
}

'''
text = text[:load_start] + new_load + text[save_comment:]

save_start = text.index("bool cModel3DWrapper::SaveVW3D(const std::string &FileName)\n{")
metadata_comment = text.index("/*\n * 3D model's metadata initialization", save_start)
new_save = r'''bool cModel3DWrapper::SaveVW3D(const std::string &FileName)
{
    if (!GlobalVertexArray || !GlobalIndexArray || Chunks.empty()) {
        std::cerr << __func__ << "(): " << "Can't create " << FileName << " file for empty Model3D.\n";
        return false;
    }

    std::ofstream FileVW3D(FileName, std::ios::binary);
    if (FileVW3D.fail()) {
        std::cerr << __func__ << "(): " << "Can't create " << FileName << " file on disk.\n";
        return false;
    }

    constexpr char Sign[4]{'V','W','3','D'};
    FileVW3D.write(Sign, sizeof(Sign));
    WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(Chunks.size()));

    for (const auto &tmpChunk : Chunks) {
        WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(tmpChunk.VertexFormat));
        WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(tmpChunk.VertexStride));
        WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(tmpChunk.VertexQuantity));
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Location.x);
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Location.y);
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Location.z);
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Rotation.x);
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Rotation.y);
        WriteVW3DLEFloat(FileVW3D, tmpChunk.Rotation.z);
    }

    WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(GlobalVertexArrayCount));
    const size_t FloatCount = static_cast<size_t>(Chunks[0].VertexStride)
                            * static_cast<size_t>(GlobalVertexArrayCount);
    for (size_t i = 0; i < FloatCount; ++i) {
        WriteVW3DLEFloat(FileVW3D, GlobalVertexArray.get()[i]);
    }
    for (unsigned i = 0; i < GlobalIndexArrayCount; ++i) {
        WriteVW3DLE32(FileVW3D, static_cast<uint32_t>(GlobalIndexArray.get()[i]));
    }

    if (!FileVW3D.good()) {
        return false;
    }
    std::cout << "VW3D Write: " << FileName << "\n";
    return true;
}

'''
text = text[:save_start] + new_save + text[metadata_comment:]

model.write_text(text, encoding="utf-8")
print("patched AstroMenace VW3D model format to explicit little-endian data")
