#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil


def replace_function(text: str, name: str, next_name: str, replacement: str) -> str:
    start_marker = f"int\n{name} "
    next_marker = f"\nint\n{next_name} "
    start = text.find(start_marker)
    if start < 0:
        raise RuntimeError(f"ENet function not found: {name}")
    end = text.find(next_marker, start + len(start_marker))
    if end < 0:
        raise RuntimeError(f"ENet next-function boundary not found after {name}: {next_name}")
    return text[:start] + replacement.rstrip() + "\n" + text[end:]


def patch_unix_c(path: Path, compat_header: Path) -> None:
    text = path.read_text(encoding="utf-8")

    old = "#define _POSIX_C_SOURCE 200112L\n"
    new = (
        "#if !defined(__AROS__) && !defined(__amigaos__) && "
        "!defined(__AMIGA__) && !defined(AMIGA)\n"
        "#define _POSIX_C_SOURCE 200112L\n"
        "#endif\n"
    )
    if old in text:
        text = text.replace(old, new, 1)

    marker = '#include "enet/enet.h"\n'
    include = '#include "enet/enet.h"\n#include "enet_amiga_compat.h"\n'
    if '#include "enet_amiga_compat.h"' not in text:
        if marker not in text:
            raise RuntimeError(f"ENet include marker not found in {path}")
        text = text.replace(marker, include, 1)

    if "enet_amiga_inet_aton (name" not in text:
        text = replace_function(
            text,
            "enet_address_set_host_ip",
            "enet_address_set_host",
            """int
enet_address_set_host_ip (ENetAddress * address, const char * name)
{
#ifdef ENET_AMIGA_TARGET
    if (! enet_amiga_inet_aton (name, & address -> host))
#else
#ifdef HAS_INET_PTON
    if (! inet_pton (AF_INET, name, & address -> host))
#else
    if (! inet_aton (name, (struct in_addr *) & address -> host))
#endif
#endif
        return -1;

    return 0;
}""",
        )

    if "return enet_amiga_inet_ntoa" not in text:
        text = replace_function(
            text,
            "enet_address_get_host_ip",
            "enet_address_get_host",
            """int
enet_address_get_host_ip (const ENetAddress * address, char * name, size_t nameLength)
{
#ifdef ENET_AMIGA_TARGET
    return enet_amiga_inet_ntoa (address -> host, name, nameLength);
#else
#ifdef HAS_INET_NTOP
    if (inet_ntop (AF_INET, & address -> host, name, nameLength) == NULL)
#else
    char * addr = inet_ntoa (* (struct in_addr *) & address -> host);
    if (addr != NULL)
    {
        size_t addrLen = strlen(addr);
        if (addrLen >= nameLength)
          return -1;
        memcpy (name, addr, addrLen + 1);
    }
    else
#endif
        return -1;
    return 0;
#endif
}""",
        )

    replacements = {
        "int\nenet_initialize (void)\n{\n    return 0;\n}":
            "int\nenet_initialize (void)\n{\n    return enet_amiga_socket_init();\n}",
        "void\nenet_deinitialize (void)\n{\n}":
            "void\nenet_deinitialize (void)\n{\n    enet_amiga_socket_deinit();\n}",
        "sin.sin_addr.s_addr = INADDR_ANY;":
            "sin.sin_addr.s_addr = ENET_AMIGA_INADDR_ANY;",
        "result = ioctl (socket, FIONBIO, & value);":
            "result = ENET_AMIGA_IOCTL_SOCKET (socket, FIONBIO, & value);",
        "if (result == -1 && errno == EINPROGRESS)":
            "if (result == -1 && ENET_AMIGA_ERRNO == EINPROGRESS)",
        "      close (socket);":
            "      ENET_AMIGA_CLOSE_SOCKET (socket);",
        "       if (errno == EWOULDBLOCK)":
            "       if (ENET_AMIGA_ERRNO == EWOULDBLOCK)",
        "    return select (maxSocket + 1, readSet, writeSet, NULL, & timeVal);":
            "    return ENET_AMIGA_SELECT (maxSocket + 1, readSet, writeSet, NULL, & timeVal);",
        "    selectCount = select (socket + 1, & readSet, & writeSet, NULL, & timeVal);":
            "    selectCount = ENET_AMIGA_SELECT (socket + 1, & readSet, & writeSet, NULL, & timeVal);",
        "        if (errno == EINTR && * condition & ENET_SOCKET_WAIT_INTERRUPT)":
            "        if (ENET_AMIGA_ERRNO == EINTR && * condition & ENET_SOCKET_WAIT_INTERRUPT)",
    }
    for before, after in replacements.items():
        if before in text:
            text = text.replace(before, after)

    required = (
        '#include "enet_amiga_compat.h"',
        "enet_amiga_socket_init()",
        "enet_amiga_socket_deinit()",
        "enet_amiga_inet_aton (name",
        "return enet_amiga_inet_ntoa",
        "ENET_AMIGA_INADDR_ANY",
        "ENET_AMIGA_IOCTL_SOCKET",
        "ENET_AMIGA_SELECT",
        "ENET_AMIGA_ERRNO",
    )
    missing = [token for token in required if token not in text]
    if missing:
        raise RuntimeError(
            f"ENet Amiga patch incomplete for {path}: missing " + ", ".join(missing)
        )

    path.write_text(text, encoding="utf-8")
    shutil.copy2(compat_header, path.parent / "enet_amiga_compat.h")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("enet_dir", type=Path)
    ap.add_argument(
        "--compat-header",
        type=Path,
        default=Path(__file__).resolve().parents[1]
        / "ports" / "openomf" / "enet_amiga_compat.h",
    )
    ns = ap.parse_args()

    enet_dir = ns.enet_dir.resolve()
    unix_c = enet_dir / "unix.c"
    if not unix_c.is_file():
        raise FileNotFoundError(unix_c)
    if not ns.compat_header.is_file():
        raise FileNotFoundError(ns.compat_header)

    patch_unix_c(unix_c, ns.compat_header)
    print(f"patched and verified {unix_c}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
