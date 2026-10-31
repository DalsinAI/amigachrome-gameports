include("${CMAKE_CURRENT_LIST_DIR}/../../toolchain/amigaos3-opengpu.cmake")
# Link feature probes: a static archive does not establish symbol availability.
set(CMAKE_TRY_COMPILE_TARGET_TYPE EXECUTABLE)
