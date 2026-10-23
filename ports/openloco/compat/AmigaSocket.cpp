// Copyright (c) 2026 Dalsin Limited. MIT licensed.
#include <proto/exec.h>
#include <stdexcept>
struct Library* OpenLocoBsdBase();
// A socket library base belongs to the calling Amiga task. Opening one per
// C++ thread also keeps OpenTLS's caller-owned SocketBase convention available.
struct Library* OpenLocoBsdBase() {
    struct TaskSocket {
        struct Library* base;
        TaskSocket() : base(OpenLibrary("bsdsocket.library",4)) {
            if(!base) throw std::runtime_error("Cannot open bsdsocket.library v4");
        }
        ~TaskSocket() { CloseLibrary(base); }
    };
    static thread_local TaskSocket socket;
    return socket.base;
}
