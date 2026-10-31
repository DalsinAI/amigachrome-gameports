# AmigaOS 3.x / OpenGPU SDK cross toolchain.
# Canonical revival target: 68040 + FPU, libnix, GCC 16.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

# All build-amigaos3.sh entry points accept STOVE. Honour that same variable
# here: otherwise headers can come from Open2 while CMake silently uses the
# older compiler in HOME. Explicit -DAMIGA_STOVE still takes precedence.
if(NOT DEFINED AMIGA_STOVE OR AMIGA_STOVE STREQUAL "")
  if(DEFINED ENV{AMIGA_STOVE} AND NOT "$ENV{AMIGA_STOVE}" STREQUAL ""
      AND DEFINED ENV{STOVE} AND NOT "$ENV{STOVE}" STREQUAL ""
      AND NOT "$ENV{AMIGA_STOVE}" STREQUAL "$ENV{STOVE}")
    message(FATAL_ERROR
      "Conflicting STOVE and AMIGA_STOVE environments; use one stove so headers, compiler and runtime match")
  endif()
  if(DEFINED ENV{AMIGA_STOVE} AND NOT "$ENV{AMIGA_STOVE}" STREQUAL "")
    set(AMIGA_STOVE "$ENV{AMIGA_STOVE}")
  elseif(DEFINED ENV{STOVE} AND NOT "$ENV{STOVE}" STREQUAL "")
    set(AMIGA_STOVE "$ENV{STOVE}")
  else()
    set(AMIGA_STOVE "$ENV{HOME}/AmigaChrome/stoves/os32-gcc16")
  endif()
endif()

# Keep an explicitly selected stove through CMake's nested probe projects.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
  AMIGA_STOVE AMIGA_CPU_FLAGS AMIGA_RUNTIME AMIGA_THREAD_FLAGS AMIGA_SDK_OVERLAYS)
message(STATUS "Amiga toolchain stove: ${AMIGA_STOVE}")

set(AMIGA_PREFIX "${AMIGA_STOVE}/prefix")
set(AMIGA_SYSROOT "${AMIGA_PREFIX}/m68k-amigaos")

set(CMAKE_C_COMPILER   "${AMIGA_PREFIX}/bin/m68k-amigaos-gcc")
set(CMAKE_CXX_COMPILER "${AMIGA_PREFIX}/bin/m68k-amigaos-g++")
set(CMAKE_AR           "${AMIGA_PREFIX}/bin/m68k-amigaos-ar" CACHE FILEPATH "")
set(CMAKE_RANLIB       "${AMIGA_PREFIX}/bin/m68k-amigaos-ranlib" CACHE FILEPATH "")
set(CMAKE_STRIP        "${AMIGA_PREFIX}/bin/m68k-amigaos-strip" CACHE FILEPATH "")

set(AMIGA_CPU_FLAGS "-m68040 -m68881" CACHE STRING "Amiga CPU/FPU flags")
set(AMIGA_RUNTIME "-noixemul" CACHE STRING "Amiga C runtime")
set(AMIGA_THREAD_FLAGS "-pthread" CACHE STRING "Amiga pthread personality")

set(CMAKE_C_FLAGS_INIT
    "${AMIGA_RUNTIME} ${AMIGA_CPU_FLAGS} ${AMIGA_THREAD_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT
    "${AMIGA_RUNTIME} ${AMIGA_CPU_FLAGS} ${AMIGA_THREAD_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${AMIGA_RUNTIME} ${AMIGA_CPU_FLAGS} ${AMIGA_THREAD_FLAGS}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_PREFIX_PATH "${AMIGA_SYSROOT}" CACHE STRING "")
set(CMAKE_FIND_ROOT_PATH "${AMIGA_SYSROOT}")
# Explicit target-only dependency overlays, kept through nested CMake probes.
# The normal compiler/runtime and all ONLY search modes remain selected.
if(AMIGA_SDK_OVERLAYS)
  list(PREPEND CMAKE_FIND_ROOT_PATH ${AMIGA_SDK_OVERLAYS})
  list(PREPEND CMAKE_PREFIX_PATH ${AMIGA_SDK_OVERLAYS})
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_THREAD_LIBS_INIT "-lpthread")
set(THREADS_PREFER_PTHREAD_FLAG ON)
