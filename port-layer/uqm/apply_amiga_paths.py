#!/usr/bin/env python3
"""Keep Amiga volume paths native at UQM's physical filesystem boundary."""
from pathlib import Path
import sys


def apply(source):
    source = Path(source)
    p = source / "src/libs/uio/stdio/stdio.c"
    text = p.read_text()
    anchor = "#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)"
    # All five branches describe a filesystem without a Unix / root.
    if text.count(anchor + "\n") != 5:
        raise RuntimeError("Unexpected UQM stdio path layout")
    text = text.replace(anchor + "\n", anchor + " || defined(__amigaos__)\n")
    p.write_text(text)
    p = source / "src/libs/file/dirs.c"
    text = p.read_text()
    anchor = "\torgSrc = src;\n#ifdef HAVE_DRIVE_LETTERS"
    if text.count(anchor) != 1:
        raise RuntimeError("Unexpected UQM absolute path layout")
    text = text.replace(anchor, """#ifdef __amigaos__
    /* A volume/assign prefix is absolute; '/' is not a Unix root here. */
    if (strchr(src, ':') != NULL &&
            (strchr(src, '/') == NULL || strchr(src, ':') < strchr(src, '/'))) {
        *skipSrc = 0;
        return dest;
    }
#endif
""" + anchor)
    p.write_text(text)


if __name__ == "__main__":
    apply(sys.argv[1])
