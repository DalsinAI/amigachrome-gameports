#!/usr/bin/env python3
"""Build an isolated pointer-width shim for host mocks, never target builds."""
import sys
from pathlib import Path

source, output = map(Path, sys.argv[1:])
text = (source / 'exec/types.h').read_text()
if 'typedef unsigned int\tULONG;' in text:
    # OpenNDK intentionally uses 32-bit ints. Mock TagItems carry host pointers.
    text = text.replace('typedef int\t\tLONG;', 'typedef long\t\tLONG;')
    text = text.replace('typedef unsigned int\tULONG;', 'typedef unsigned long\tULONG;')
    target = output / 'exec/types.h'
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
