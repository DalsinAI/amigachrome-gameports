// Copyright (c) 2026 Dalsin Limited. MIT licensed.
#pragma once
#ifdef __amigaos__
struct Library;
struct Library* OpenLocoBsdBase();
#define BSDSOCKET_BASE_NAME OpenLocoBsdBase()
#include <proto/bsdsocket.h>
#include <sys/filio.h>
// No TCP listen() call is used here; keep the game interface method intact.
#undef listen
// This SDK has sockaddr_in6 but does not expose POSIX sockaddr_storage.
// Only address storage is needed here; the OS's native sockaddr ABI is retained.
union OpenLocoSocketStorage {
    struct sockaddr address;
    struct sockaddr_in ipv4;
    struct sockaddr_in6 ipv6;
};
using sockaddr_storage = OpenLocoSocketStorage;
#undef LAST_SOCKET_ERROR
#define LAST_SOCKET_ERROR() Errno()
#undef closesocket
#define closesocket CloseSocket
#undef ioctlsocket
#define ioctlsocket IoctlSocket
#endif
#ifdef __amigaos__
// The classic SDK predates const-correct socket prototypes. Keep casts at the
// native ABI boundary; callers retain the usual read-only buffer contracts.
inline const char* LocoInetNtop(int af,const void* src,char* dst,int size) {
    return inet_ntop(af,const_cast<void*>(src),dst,size);
}
inline int LocoSetSockOpt(int fd,int level,int option,const void* value,int size) {
    return setsockopt(fd,level,option,const_cast<void*>(value),size);
}
inline int LocoSendTo(int fd,const void* bytes,int size,int flags,const sockaddr* to,int length) {
    return sendto(fd,const_cast<void*>(bytes),size,flags,const_cast<sockaddr*>(to),length);
}
#undef inet_ntop
#define inet_ntop LocoInetNtop
#undef setsockopt
#define setsockopt LocoSetSockOpt
#undef sendto
#define sendto LocoSendTo
#endif
