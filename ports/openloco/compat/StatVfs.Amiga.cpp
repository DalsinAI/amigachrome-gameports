// Copyright (c) 2026 Dalsin Limited. MIT licensed.
// The open3 sysroot declares statvfs with C++ linkage but has no implementation.
// Supply the space fields libstdc++ filesystem uses, through native DOS Info().
#include <proto/dos.h>
#include <sys/statvfs.h>
#include <cerrno>
#include <cstring>
int statvfs(const char* path, struct statvfs* result) {
    if(!path || !result) { errno=EINVAL;return -1; }
    const BPTR lock=Lock(path,ACCESS_READ);
    if(!lock) { errno=IoErr()==ERROR_OBJECT_NOT_FOUND?ENOENT:EIO;return -1; }
    alignas(4) struct InfoData data{};
    const bool ok=Info(lock,&data);
    UnLock(lock);
    if(!ok || data.id_NumBlocks<0 || data.id_NumBlocksUsed<0 || data.id_BytesPerBlock<=0) {
        errno=EIO;return -1;
    }
    std::memset(result,0,sizeof *result);
    result->f_bsize=result->f_frsize=data.id_BytesPerBlock;
    result->f_blocks=static_cast<unsigned long long>(data.id_NumBlocks);
    result->f_bfree=result->f_bavail=data.id_NumBlocksUsed<=data.id_NumBlocks
        ? static_cast<unsigned long long>(data.id_NumBlocks-data.id_NumBlocksUsed):0;
    return 0;
}
