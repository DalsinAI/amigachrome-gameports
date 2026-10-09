#ifndef ENET_AMIGA_COMPAT_H
#define ENET_AMIGA_COMPAT_H

#if defined(__AROS__) || defined(__amigaos__) || defined(__AMIGA__) || defined(AMIGA)

#define ENET_AMIGA_TARGET 1

#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#if defined(__AROS__)
#include <bsdsocket/socketbasetags.h>
#else
#include <amitcp/socketbasetags.h>
#endif

#ifdef HAS_FCNTL
#undef HAS_FCNTL
#endif

#ifdef HAS_POLL
#undef HAS_POLL
#endif

#ifndef ENET_AMIGA_SOCKET_MIN_VERSION
#define ENET_AMIGA_SOCKET_MIN_VERSION 4
#endif

struct Library *SocketBase = NULL;
static LONG enet_amiga_socket_errno = 0;
static LONG enet_amiga_socket_herrno = 0;

#define ENET_AMIGA_INADDR_ANY ((enet_uint32)0)
#define ENET_AMIGA_ERRNO ((int)enet_amiga_socket_errno)
#define ENET_AMIGA_CLOSE_SOCKET(s) CloseSocket((LONG)(s))
#define ENET_AMIGA_IOCTL_SOCKET(s, request, argp) \
    IoctlSocket((LONG)(s), (ULONG)(request), (char *)(argp))
#define ENET_AMIGA_SELECT(n, r, w, e, t) \
    WaitSelect((LONG)(n), (r), (w), (e), (t), NULL)

static int
enet_amiga_socket_init(void)
{
    if (SocketBase != NULL)
        return 0;

    SocketBase = OpenLibrary("bsdsocket.library", ENET_AMIGA_SOCKET_MIN_VERSION);
    if (SocketBase == NULL)
        return -1;

    SocketBaseTags(
        SBTM_SETVAL(SBTC_ERRNOLONGPTR), (ULONG)&enet_amiga_socket_errno,
        SBTM_SETVAL(SBTC_HERRNOLONGPTR), (ULONG)&enet_amiga_socket_herrno,
        TAG_DONE);

    return 0;
}

static void
enet_amiga_socket_deinit(void)
{
    if (SocketBase != NULL)
    {
        CloseLibrary(SocketBase);
        SocketBase = NULL;
    }
}

static int
enet_amiga_inet_aton(const char *text, enet_uint32 *out)
{
    unsigned int octet[4];
    unsigned int i;
    const char *p = text;
    unsigned char *bytes = (unsigned char *)out;

    if (text == NULL || out == NULL)
        return 0;

    for (i = 0; i < 4; ++i)
    {
        unsigned int value = 0;
        unsigned int digits = 0;

        while (*p >= '0' && *p <= '9')
        {
            value = value * 10U + (unsigned int)(*p - '0');
            if (value > 255U)
                return 0;
            ++digits;
            ++p;
        }

        if (digits == 0)
            return 0;
        octet[i] = value;

        if (i != 3)
        {
            if (*p != '.')
                return 0;
            ++p;
        }
    }

    if (*p != '\0')
        return 0;

    bytes[0] = (unsigned char)octet[0];
    bytes[1] = (unsigned char)octet[1];
    bytes[2] = (unsigned char)octet[2];
    bytes[3] = (unsigned char)octet[3];
    return 1;
}

static size_t
enet_amiga_write_octet(char *dst, unsigned int value)
{
    char tmp[3];
    size_t n = 0;
    size_t i;

    if (value >= 100U)
        tmp[n++] = (char)('0' + (value / 100U));
    if (value >= 10U)
        tmp[n++] = (char)('0' + ((value / 10U) % 10U));
    tmp[n++] = (char)('0' + (value % 10U));

    for (i = 0; i < n; ++i)
        dst[i] = tmp[i];
    return n;
}

static int
enet_amiga_inet_ntoa(enet_uint32 address, char *name, size_t nameLength)
{
    const unsigned char *bytes = (const unsigned char *)&address;
    size_t pos = 0;
    unsigned int i;

    if (name == NULL || nameLength == 0)
        return -1;

    for (i = 0; i < 4; ++i)
    {
        char part[3];
        size_t n = enet_amiga_write_octet(part, (unsigned int)bytes[i]);

        if (pos + n + (i == 3 ? 1U : 2U) > nameLength)
            return -1;

        memcpy(name + pos, part, n);
        pos += n;
        if (i != 3)
            name[pos++] = '.';
    }

    name[pos] = '\0';
    return 0;
}

#else

#define ENET_AMIGA_INADDR_ANY INADDR_ANY
#define ENET_AMIGA_ERRNO errno
#define ENET_AMIGA_CLOSE_SOCKET(s) close((s))
#define ENET_AMIGA_IOCTL_SOCKET(s, request, argp) ioctl((s), (request), (argp))
#define ENET_AMIGA_SELECT(n, r, w, e, t) select((n), (r), (w), (e), (t))

static int enet_amiga_socket_init(void) { return 0; }
static void enet_amiga_socket_deinit(void) {}

#endif
#endif
