#!/usr/bin/env python3
"""Create the portable CMake toolchain and SDL2 package shims used by the ports."""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--sysroot", type=Path, required=True)
    ap.add_argument("--cc", default="m68k-aros-gcc")
    ap.add_argument("--cxx", default="m68k-aros-g++")
    ap.add_argument("--cpu", default="68040", choices=("68020", "68030", "68040"))
    args = ap.parse_args()

    out = args.out.resolve()
    sysroot = args.sysroot.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sdl = out / "sdl2"
    mixer = out / "sdl2-mixer"
    sdl.mkdir(exist_ok=True)
    mixer.mkdir(exist_ok=True)

    (out / "aros-m68k.cmake").write_text(f"""set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_C_COMPILER "{args.cc}")
set(CMAKE_CXX_COMPILER "{args.cxx}")
set(CMAKE_C_FLAGS_INIT "--sysroot={sysroot} -m{args.cpu} -fno-delete-null-pointer-checks")
set(CMAKE_CXX_FLAGS_INIT "--sysroot={sysroot} -m{args.cpu} -fno-delete-null-pointer-checks")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--sysroot={sysroot} -m{args.cpu}")
set(CMAKE_FIND_ROOT_PATH "{sysroot}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
""")

    (sdl / "SDL2Config.cmake").write_text(f"""if(NOT TARGET SDL2::SDL2)
  add_library(SDL2::SDL2 STATIC IMPORTED)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "{sysroot}/lib/libSDL2.a"
    INTERFACE_INCLUDE_DIRECTORIES "{sysroot}/include/SDL2"
    INTERFACE_LINK_LIBRARIES "pthread")
endif()
if(NOT TARGET SDL2::SDL2main)
  add_library(SDL2::SDL2main INTERFACE IMPORTED)
  target_link_libraries(SDL2::SDL2main INTERFACE SDL2::SDL2)
endif()
set(SDL2_FOUND TRUE)
set(SDL2_VERSION "2.32.10")
set(SDL2_INCLUDE_DIR "{sysroot}/include/SDL2")
set(SDL2_INCLUDE_DIRS "{sysroot}/include/SDL2")
set(SDL2_LIBRARY "{sysroot}/lib/libSDL2.a")
set(SDL2_LIBRARIES "{sysroot}/lib/libSDL2.a;pthread")
""")
    (sdl / "SDL2ConfigVersion.cmake").write_text("""set(PACKAGE_VERSION "2.32.10")
if(PACKAGE_FIND_VERSION VERSION_LESS_EQUAL PACKAGE_VERSION)
  set(PACKAGE_VERSION_COMPATIBLE TRUE)
  if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
    set(PACKAGE_VERSION_EXACT TRUE)
  endif()
endif()
""")

    (mixer / "SDL2_mixerConfig.cmake").write_text(f"""if(NOT TARGET SDL2_mixer::SDL2_mixer)
  add_library(SDL2_mixer::SDL2_mixer STATIC IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
    IMPORTED_LOCATION "{sysroot}/lib/libSDL2_mixer.a"
    INTERFACE_INCLUDE_DIRECTORIES "{sysroot}/include/SDL2")
endif()
if(NOT TARGET SDL2_mixer::SDL2_mixer-static)
  add_library(SDL2_mixer::SDL2_mixer-static STATIC IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer-static PROPERTIES
    IMPORTED_LOCATION "{sysroot}/lib/libSDL2_mixer.a"
    INTERFACE_INCLUDE_DIRECTORIES "{sysroot}/include/SDL2")
endif()
set(SDL2_mixer_FOUND TRUE)
set(SDL2_MIXER_FOUND TRUE)
""")
    (mixer / "SDL2_mixerConfigVersion.cmake").write_text(
        'set(PACKAGE_VERSION "2.8.1")\nset(PACKAGE_VERSION_COMPATIBLE TRUE)\n'
    )
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
