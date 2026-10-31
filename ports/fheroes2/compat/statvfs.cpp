// SPDX-License-Identifier: MIT
// Copyright (c) 2026 AmigaChrome Team
//
// libnix has no statvfs(), but the open3 libstdc++'s filesystem::space()
// refers to one (declared there with C++ linkage). fheroes2 never asks for
// free space; this answers "not supported" so std::filesystem links.
#include <cerrno>

struct statvfs;

int statvfs( const char *, struct statvfs * )
{
    errno = ENOSYS;
    return -1;
}
