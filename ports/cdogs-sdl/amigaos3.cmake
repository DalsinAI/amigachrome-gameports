# CMake toolchain for AmigaOS 3.x with bebbo's m68k-amigaos-gcc (libnix, no
# ixemul). The compiler is found on PATH (the Kitchen's OS 3.2 stove).
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)
set(CMAKE_C_COMPILER m68k-amigaos-gcc)
set(CMAKE_AR m68k-amigaos-ar CACHE FILEPATH "")
set(CMAKE_RANLIB m68k-amigaos-ranlib CACHE FILEPATH "")
set(AMIGA_CPU "-m68040 -m68881" CACHE STRING "CPU flags")
set(CMAKE_C_FLAGS_INIT "-noixemul ${AMIGA_CPU}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-noixemul ${AMIGA_CPU}")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
