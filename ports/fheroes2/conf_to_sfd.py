#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 AmigaChrome Team
"""Write an SFD from an AROS library .conf function list.

The AROS tree keeps cybergraphics.library's calls in workbench/libs/cgfx/
cybergraphics.conf, in vector order. A blank line or ".skip N" is a reserved
vector, exactly as the AROS genmodule tool reads it. The SFD is used only to
generate this build's proto/inline headers with sfdc; it is never shipped.

    conf_to_sfd.py cybergraphics.conf cybergraphics_lib.sfd \
        --base CyberGfxBase --libname cybergraphics.library
"""
import argparse
import re
from pathlib import Path

CALL = re.compile(r"^(?P<ret>.+?)\b(?P<name>\w+)\s*\((?P<args>.*)\)\s*\((?P<regs>[^)]*)\)\s*$")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("conf", type=Path)
    ap.add_argument("sfd", type=Path)
    ap.add_argument("--base", required=True)
    ap.add_argument("--basetype", default="struct Library *")
    ap.add_argument("--libname", required=True)
    ap.add_argument("--include", action="append", default=[])
    args = ap.parse_args()

    # IPTR is AROS's pointer-sized integer; on 68k AmigaOS it is ULONG.
    text = re.sub(r"\bIPTR\b", "ULONG", args.conf.read_text(encoding="latin-1"))
    body = text.split("##begin functionlist", 1)[1].split("##end functionlist", 1)[0]
    out = [f"==id $Id: {args.sfd.name} generated from {args.conf.name} $",
           f"==base _{args.base}", f"==basetype {args.basetype}",
           f"==libname {args.libname}", "==bias 30", "==public"]
    out += [f"==include {inc}" for inc in args.include]
    first = True
    for raw in body.splitlines():
        line = raw.strip()
        if first and not line:
            continue  # the line after "##begin functionlist"
        first = False
        if line.startswith(".version"):
            out.append("==version " + line.split()[1])
        elif line.startswith(".skip"):
            out.append("==reserve " + line.split()[1])
        elif not line:
            out.append("==reserve 1")
        elif line.startswith("."):
            continue
        else:
            m = CALL.match(line)
            if not m:
                raise SystemExit("cannot read: " + line)
            regs = ",".join(r.strip().lower() for r in m["regs"].split(",") if r.strip())
            out.append(f"{m['ret'].strip()} {m['name']}({m['args'].strip()}) ({regs})")
            if m["name"].endswith("TagList"):
                tags = re.sub(r"struct TagItem\s*\*\s*(\w+)\s*$", r"Tag \1, ...", m["args"].strip())
                out += ["==varargs", f"{m['ret'].strip()} {m['name'][:-7]}Tags({tags}) ({regs})"]
    while out[-1] == "==reserve 1":
        out.pop()
    out.append("==end")
    args.sfd.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
