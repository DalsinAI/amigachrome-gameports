#include <proto/dos.h>
#include <sys/statvfs.h>
#include <filesystem>
#include <cstdio>
#include <cerrno>
int main() {
    struct statvfs native{};
    if(statvfs("SYS:",&native)!=0 || !native.f_frsize || native.f_bfree>native.f_blocks) {
        std::printf("FAIL DOS space errno=%d\n",errno);return 10;
    }
    std::error_code error;
    const auto space=std::filesystem::space("SYS:",error);
    if(error || space.capacity!=native.f_blocks*native.f_frsize || space.available>space.capacity) {
        std::printf("FAIL filesystem space error=%d\n",error.value());return 11;
    }
    if(statvfs("SYS:Team1-No-Such-Directory-StatVfs-Probe",&native)!=-1 || errno!=ENOENT) {
        std::printf("FAIL missing path errno=%d\n",errno);return 12;
    }
    std::printf("PASS: DOS Info and C++ filesystem space agree; missing path returns ENOENT\n");return 0;
}
