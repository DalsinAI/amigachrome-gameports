
#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument(
        "--stub",
        type=Path,
        default=Path(__file__).resolve().parents[1]
        / "port-layer" / "openomf-amiga" / "modmanager_amiga_stub.c",
    )
    ns = ap.parse_args()
    source = ns.source.resolve()

    cmake = source / "CMakeLists.txt"
    text = cmake.read_text(encoding="utf-8")
    text = text.replace(
        "add_definitions(-DOPENOMF_AMIGAAGA=1 -D__AROS__=1)",
        "add_definitions(-DOPENOMF_AMIGAAGA=1 -D__amigaos__=1 -DARG_REPLACE_GETOPT=0)",
        1,
    )

    old = """    add_library(openomf::enet INTERFACE IMPORTED)
    target_link_libraries(openomf::enet INTERFACE enet)
"""
    new = """    add_library(openomf::enet INTERFACE IMPORTED)
    target_include_directories(openomf::enet INTERFACE "${CMAKE_SOURCE_DIR}/src/vendored/enet/include")
    target_link_libraries(openomf::enet INTERFACE enet)
"""
    if "target_include_directories(openomf::enet INTERFACE" not in text:
        if old not in text:
            raise RuntimeError("OpenOMF ENet target marker missing")
        text = text.replace(old, new, 1)

    old_remove = """        "src/game/audio/audio_sources.c"
        "src/audio/music_sources/opus_source.c"
        "src/audio/music_sources/psm_source.c"
    )
endif()
"""
    new_remove = """        "src/game/audio/audio_sources.c"
        "src/audio/music_sources/opus_source.c"
        "src/audio/music_sources/psm_source.c"
        "src/resources/modmanager.c"
    )
    list(APPEND OPENOMF_SRC
        "src/resources/modmanager_amiga_stub.c"
    )
endif()
"""
    if '"src/resources/modmanager_amiga_stub.c"' not in text:
        if old_remove not in text:
            raise RuntimeError("OpenOMF AMIGAAGA source-filter marker missing")
        text = text.replace(old_remove, new_remove, 1)

    psm_source_marker = """    list(APPEND OPENOMF_SRC
        "src/resources/modmanager_amiga_stub.c"
    )
"""
    psm_source_new = """    list(APPEND OPENOMF_SRC
        "src/resources/modmanager_amiga_stub.c"
        "src/audio/music_sources/psm_source_amiga_stub.c"
    )
"""
    if '"src/audio/music_sources/psm_source_amiga_stub.c"' not in text:
        if psm_source_marker not in text:
            raise RuntimeError("OpenOMF PSM stub source marker missing")
        text = text.replace(psm_source_marker, psm_source_new, 1)

    vendored_filter = """if(AMIGAAGA)
    list(FILTER OPENOMF_SRC EXCLUDE REGEX "^src/vendored/enet/")
    list(FILTER OPENOMF_SRC EXCLUDE REGEX "^src/vendored/acgame/")
endif()

"""
    filter_anchor = "# Remove all player plugin source code from OPENOMF_SRC\n"
    if 'EXCLUDE REGEX "^src/vendored/enet/"' not in text:
        if filter_anchor not in text:
            raise RuntimeError("OpenOMF vendored-source filter anchor missing")
        text = text.replace(filter_anchor, vendored_filter + filter_anchor, 1)

    final_old = "openomf::argtable openomf::zip openomf::SDL2main acgame)"
    final_new = "openomf::argtable openomf::zip openomf::SDL2main openomf::enet acgame)"
    if final_new not in text:
        if final_old not in text:
            raise RuntimeError("OpenOMF final ENet link marker missing")
        text = text.replace(final_old, final_new, 1)

    text = text.replace("AROS_SDK", "AMIGA_SDK")
    text = text.replace("AROS/Developer sysroot", "AmigaOS 3 GCC16 sysroot")

    cmake.write_text(text, encoding="utf-8")

    dst = source / "src" / "resources" / "modmanager_amiga_stub.c"
    if not ns.stub.is_file():
        raise FileNotFoundError(ns.stub)
    shutil.copy2(ns.stub, dst)

    psm_stub = Path(__file__).resolve().with_name("psm_source_amiga_stub.c")
    if not psm_stub.is_file():
        raise FileNotFoundError(psm_stub)
    shutil.copy2(psm_stub, source / "src" / "audio" / "music_sources" / "psm_source_amiga_stub.c")

    png = source / "src" / "utils" / "png_reader.c"
    text = png.read_text(encoding="utf-8")
    text = text.replace(
        "bool read_paletted_png_from_memory(const char *buffer, size_t size,",
        "bool read_paletted_png_from_memory(const unsigned char *buffer, size_t size,",
        1,
    )
    text = text.replace(
        '    PERROR("PNG reading is not supported in current build!");',
        '    log_error("PNG reading is not supported in current build!");',
    )
    png.write_text(text, encoding="utf-8")

    png_writer = source / "src" / "utils" / "png_writer.c"
    text = png_writer.read_text(encoding="utf-8")
    text = text.replace(
        'bool png_write_rgb(const char *filename, int w, int h, const unsigned char *data, bool has_alpha, bool flip) {',
        'bool write_rgb_png(const path *filename, int w, int h, const unsigned char *data, bool has_alpha, bool flip) {',
        1,
    )
    text = text.replace(
        'bool png_write_paletted(const char *filename, int w, int h, const vga_palette *pal, const unsigned char *data) {',
        'bool write_paletted_png(const path *filename, int w, int h, const vga_palette *pal, const unsigned char *data) {',
        1,
    )
    text = text.replace(
        '    PERROR("PNG writing is not supported in current build!");',
        '    log_error("PNG writing is not supported in current build!");',
    )
    png_writer.write_text(text, encoding="utf-8")

    path_c = source / "src" / "utils" / "path.c"
    text = path_c.read_text(encoding="utf-8")
    feature = (
        "#if (defined(__AROS__) || defined(__amigaos__)) && !defined(_POSIX_C_SOURCE)\n"
        "#define _POSIX_C_SOURCE 200112L\n"
        "#endif\n\n"
    )
    if not text.startswith(feature):
        text = feature + text

    include_marker = "#include <stdarg.h>\n"
    include_extra = (
        "#include <stdarg.h>\n"
        "#include <stdlib.h>\n"
        "#if defined(__AROS__) || defined(__amigaos__)\n"
        "#include <aros/posixc/stdlib.h>\n"
        "#endif\n"
    )
    if "#include <aros/posixc/stdlib.h>" not in text:
        if include_marker not in text:
            raise RuntimeError("OpenOMF path stdarg marker missing")
        text = text.replace(include_marker, include_extra, 1)

    text = text.replace(
        "#if defined(_WIN32) || defined(WIN32)\nstatic void generate_noise",
        "#if defined(_WIN32) || defined(WIN32) || (defined(__AROS__) || defined(__amigaos__))\nstatic void generate_noise",
        1,
    )

    unix_tmp = """#else
    char template[] = "/tmp/openomf.XXXXXX"; // Will be modified by mkdtemp
    const char *dir_name = mkdtemp(template);
    if(dir_name == NULL) {
        return false;
    }
    path_from_c(path, dir_name);
#endif
"""
    amiga_tmp = """#elif (defined(__AROS__) || defined(__amigaos__))
    str component;
    str_from_c(&component, "T:openomf.");
    generate_noise(&component, 16);
    path_from_c(path, str_c(&component));
    str_free(&component);
    if(!path_mkdir(path)) {
        return false;
    }
#else
    char template[] = "/tmp/openomf.XXXXXX"; // Will be modified by mkdtemp
    const char *dir_name = mkdtemp(template);
    if(dir_name == NULL) {
        return false;
    }
    path_from_c(path, dir_name);
#endif
"""
    if "T:openomf." not in text:
        if unix_tmp not in text:
            raise RuntimeError("OpenOMF tmpdir marker missing")
        text = text.replace(unix_tmp, amiga_tmp, 1)

    path_c.write_text(text, encoding="utf-8")

    print(f"patched OpenOMF AmigaOS 3 portability in {source}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
