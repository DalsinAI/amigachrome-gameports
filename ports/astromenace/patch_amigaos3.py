#!/usr/bin/env python3
from pathlib import Path
import shutil
import sys

source = Path(sys.argv[1]).resolve()
adapter = Path(sys.argv[2]).resolve()

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
    list(APPEND astromenace_SRCS "src/core/audio/audio_amigachrome.cpp")
ENDIF()
'''
if "audio_amigachrome.cpp" not in text:
    if glob_marker not in text:
        raise RuntimeError("AstroMenace source-glob marker missing")
    text = text.replace(glob_marker, insert, 1)

cmake.write_text(text, encoding="utf-8")

dst = source / "src" / "core" / "audio" / "audio_amigachrome.cpp"
shutil.copy2(adapter, dst)
print("patched AstroMenace to use AmigaChrome SDL2_mixer audio")
