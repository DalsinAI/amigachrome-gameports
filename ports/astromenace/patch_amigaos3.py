#!/usr/bin/env python3
from pathlib import Path
import shutil
import sys

source = Path(sys.argv[1]).resolve()
adapter = Path(sys.argv[2]).resolve()
port_dir = Path(__file__).resolve().parent
glu_source = port_dir / "glu_amigachrome.cpp"
glu_header = port_dir / "GL" / "glu.h"

cmake = source / "CMakeLists.txt"
text = cmake.read_text(encoding="utf-8")

dep_start = "    FIND_PACKAGE(OpenAL REQUIRED)\n"
dep_end = "    # freetype lib + header\n"
start = text.find(dep_start)
end = text.find(dep_end, start)
if start < 0 or end < 0:
    if "AmigaChrome SDL2_mixer backend" not in text:
        raise RuntimeError("AstroMenace audio dependency block marker missing")
else:
    original = text[start:end]
    replacement = """    IF(AMIGACHROME)
        # AmigaChrome SDL2_mixer backend: OpenGPU owns decode/mixing/output.
        IF(NOT SDL2_MIXER_LIBRARY OR NOT SDL2_MIXER_INCLUDE_DIR)
            MESSAGE(FATAL_ERROR "AmigaChrome SDL2_mixer SDK not provided")
        ENDIF()
        INCLUDE_DIRECTORIES(SYSTEM ${SDL2_MIXER_INCLUDE_DIR})
        SET(ALL_LIBRARIES ${ALL_LIBRARIES} ${SDL2_MIXER_LIBRARY})
    ELSE()
""" + original + """    ENDIF()
"""
    text = text[:start] + replacement + text[end:]

glob_marker = 'FILE(GLOB_RECURSE astromenace_SRCS src/*.cpp src/*.h)\n'
insert = '''FILE(GLOB_RECURSE astromenace_SRCS src/*.cpp src/*.h)
IF(AMIGACHROME)
    list(FILTER astromenace_SRCS EXCLUDE REGEX "src/core/audio/(audio|buffer|music|openal|sound)\\.cpp$")
    list(APPEND astromenace_SRCS
        "src/core/audio/audio_amigachrome.cpp"
        "src/core/graphics/glu_amigachrome.cpp")
    INCLUDE_DIRECTORIES(BEFORE "${CMAKE_SOURCE_DIR}/amigachrome-include")
ENDIF()
'''
if "audio_amigachrome.cpp" not in text:
    if glob_marker not in text:
        raise RuntimeError("AstroMenace source-glob marker missing")
    text = text.replace(glob_marker, insert, 1)

if "TARGET_LINK_LIBRARIES(astromenace ${ALL_LIBRARIES})" in text:
    text = text.replace("TARGET_LINK_LIBRARIES(astromenace ${ALL_LIBRARIES})", "TARGET_LINK_LIBRARIES(astromenace ${ALL_LIBRARIES} ${AMIGACHROME_PNG_LIBRARY} ${AMIGACHROME_ZLIB_LIBRARY})", 1)
elif "TARGET_LINK_LIBRARIES(astromenace ${ALL_LIBRARIES} ${AMIGACHROME_PNG_LIBRARY} ${AMIGACHROME_ZLIB_LIBRARY})" not in text:
    raise RuntimeError("AstroMenace final link marker missing")

cmake.write_text(text, encoding="utf-8")

# Amiga system headers define short coordinate-style macros that collide with
# AstroMenace's private _X constructor parameter. Rename the local parameters
# without changing behaviour.
font_cpp = source / "src" / "core" / "font" / "font.cpp"
font_text = font_cpp.read_text(encoding="utf-8")
old_metrics = """    explicit sFontMetrics(const int _X, const int _Y,
                          const unsigned _Width, const unsigned _Height,
                          const long _AdvanceX /* in 1/64th of points */) :
        X{_X},
        Y{_Y},
        Width{_Width},
        Height{_Height},
        /* we are safe with static_cast here, since 'advance.x' will not exceed 'float' */
        AdvanceX{static_cast<float>(_AdvanceX) / 64.0f}
"""
new_metrics = """    explicit sFontMetrics(const int amiga_x, const int amiga_y,
                          const unsigned amiga_width, const unsigned amiga_height,
                          const long amiga_advance_x /* in 1/64th of points */) :
        X{amiga_x},
        Y{amiga_y},
        Width{amiga_width},
        Height{amiga_height},
        /* we are safe with static_cast here, since 'advance.x' will not exceed 'float' */
        AdvanceX{static_cast<float>(amiga_advance_x) / 64.0f}
"""
if old_metrics in font_text:
    font_text = font_text.replace(old_metrics, new_metrics, 1)
elif "amiga_advance_x" not in font_text:
    raise RuntimeError("AstroMenace font metrics portability marker missing")
font_cpp.write_text(font_text, encoding="utf-8")

# AmigaChrome one-click packaging: if gamedata.vfs is absent but the
# redistributable upstream gamedata/ tree is present beside the executable,
# build the VFS automatically on first launch and then continue normally.
main_cpp = source / "src" / "main.cpp"
main_text = main_cpp.read_text(encoding="utf-8")
old_vfs = """    if (vw_OpenVFS(GetDataPath() + "gamedata.vfs", GAME_VFS_BUILD) != 0) {
        std::cerr << __func__ << "(): " << "gamedata.vfs file not found or corrupted.\\n";
        SDL_Quit();
        return 1;
    }
"""
new_vfs = """    if (vw_OpenVFS(GetDataPath() + "gamedata.vfs", GAME_VFS_BUILD) != 0) {
#ifdef AMIGACHROME
        std::cerr << __func__ << "(): creating gamedata.vfs from bundled gamedata/ on first launch.\\n";
        if (ConvertFS2VFS(GetDataPath() + "gamedata/", GetDataPath() + "gamedata.vfs") == 0
            && vw_OpenVFS(GetDataPath() + "gamedata.vfs", GAME_VFS_BUILD) == 0) {
            std::cerr << __func__ << "(): gamedata.vfs created successfully.\\n";
        } else {
            std::cerr << __func__ << "(): bundled gamedata could not be packed/opened.\\n";
            SDL_Quit();
            return 1;
        }
#else
        std::cerr << __func__ << "(): " << "gamedata.vfs file not found or corrupted.\\n";
        SDL_Quit();
        return 1;
#endif
    }
"""
if "creating gamedata.vfs from bundled gamedata/" not in main_text:
    if old_vfs not in main_text:
        raise RuntimeError("AstroMenace VFS-open marker missing")
    main_text = main_text.replace(old_vfs, new_vfs, 1)
main_cpp.write_text(main_text, encoding="utf-8")

dst = source / "src" / "core" / "audio" / "audio_amigachrome.cpp"
shutil.copy2(adapter, dst)

glu_dst = source / "src" / "core" / "graphics" / "glu_amigachrome.cpp"
shutil.copy2(glu_source, glu_dst)
glu_inc = source / "amigachrome-include" / "GL"
glu_inc.mkdir(parents=True, exist_ok=True)
shutil.copy2(glu_header, glu_inc / "glu.h")

print("patched AstroMenace to use AmigaChrome SDL2_mixer audio and OpenGPU GLU shim")
