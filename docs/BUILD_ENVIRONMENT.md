# Build Environment Snapshot

The successful builds were produced on 30 September 2026 with the AmigaChrome local AROS m68k SDK.

## Canonical target

- architecture: Motorola m68k
- guest OS: AROS
- CPU target: 68040
- C compiler: `m68k-aros-gcc`
- C++ compiler: `m68k-aros-g++`
- sysroot layout: AROS `Developer/include` + `Developer/lib`
- SDL2 SDK version observed: 2.32.10

The AmigaChrome tree used a generated CMake toolchain equivalent to:

    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_C_COMPILER "$ENV{AROS_CC}")
    set(CMAKE_CXX_COMPILER "$ENV{AROS_CXX}")
    set(CMAKE_C_FLAGS_INIT "--sysroot=$ENV{AROS_SYSROOT} -m68040")
    set(CMAKE_CXX_FLAGS_INIT "--sysroot=$ENV{AROS_SYSROOT} -m68040")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "--sysroot=$ENV{AROS_SYSROOT} -m68040")
    set(CMAKE_FIND_ROOT_PATH "$ENV{AROS_SYSROOT}")
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

## SDK quirks discovered

### SDL2

The AROS SDK had SDL2 headers and `libSDL2.a`, but no normal desktop-style `SDL2Config.cmake` / `sdl2.pc`. The successful CMake probes therefore supplied a small imported-target shim for `SDL2::SDL2`.

### SDL2_mixer

The SDK contains static `libSDL2_mixer.a`. Its package config only creates the shared target `SDL2_mixer::SDL2_mixer` if a shared library exists.

Chocolate Doom expected that shared target name. The successful cross-build supplied an imported CMake target named `SDL2_mixer::SDL2_mixer` pointing at the static archive.

### OpenJazz / libgcc

OpenJazz compiled to 100% but its final link initially left `__divsi3` unresolved. The successful link explicitly added the GCC target runtime returned by:

    m68k-aros-g++ --sysroot="$AROS_SYSROOT" -m68040 -print-libgcc-file-name

### NXEngine-evo

Current upstream's `ResourceManager.cpp` does not recognise AROS in its stat-based resource lookup path. The original compile-success probe used `-D__unix__`.

The game-port lane now carries a small explicit `__AROS__` patch and the NXEngine build applies it before configuration. The Unix impersonation flag has been removed. A clean current-toolchain rebuild and runtime qualification remain before release.

### SDLPoP

The old source assumes `alloca()` is implicitly available. With modern GCC warnings promoted to errors, the successful build used:

    -Dalloca=__builtin_alloca

Again, this should become a small explicit portability patch rather than remain magic build flags forever.

## Test status

These notes reproduce the successful compile environment. They do **not** claim runtime compatibility yet.
