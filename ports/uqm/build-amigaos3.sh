#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
SRC=${1:-"$ROOT/build/game-ports/sources/uqm/uqm-0.8.0"}
BUILD=${2:-"$ROOT/build/os3/uqm"}
STOVE=${STOVE:-"$HOME/AmigaChrome/stoves/os32-gcc16"}
SDK="$STOVE/prefix/m68k-amigaos"
DEPS=${UQM_DEPS:-"$HOME/AmigaChrome-dev/webkit-os32-040/m68k-amigaos"}
P="$STOVE/prefix"

rm -rf "$BUILD"
mkdir -p "$BUILD/work" "$BUILD/tools" "$BUILD/out"
cp -R "$SRC"/. "$BUILD/work"/
python3 "$ROOT/port-layer/uqm/apply_aros_bootstrap.py" "$BUILD/work" --write-config "$BUILD/config.state"
if [ -f "$BUILD/work/build.sh" ]; then SC2="$BUILD/work"; else SC2="$BUILD/work/sc2"; fi

# The upstream Unix build has an AROS cross branch we reuse only as a generic
# big-endian m68k cross profile. Replace its old sysroot assumptions with the
# current AmigaOS 3/OpenGPU SDK and the codec compatibility root.
F="$SC2/build/unix/config_proginfo_host"
python3 - "$F" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
s=s.replace('SYSTEM_HOST_CFLAGS="--sysroot=$AROS_SDK -m68040 -I$AROS_SDK/include -I$AROS_SDK/include/SDL2"',
'''SYSTEM_HOST_CFLAGS="-noixemul -m68040 -m68881 -fno-tree-loop-distribute-patterns -pthread -I$AROS_SDK/include -I$AROS_SDK/include/SDL2 -I$UQM_DEPS/include"''')
s=s.replace('SYSTEM_HOST_LDFLAGS="--sysroot=$AROS_SDK -m68040"',
'''SYSTEM_HOST_LDFLAGS="-noixemul -m68040 -m68881 -pthread -L$AROS_SDK/lib -L$UQM_DEPS/lib"''')
s=s.replace('LIB_SDL2_CFLAGS="-I$AROS_SDK/include/SDL2"',
'''LIB_SDL2_CFLAGS="-I$AROS_SDK/include/SDL2"''')
s=s.replace('LIB_SDL2_LDFLAGS="-lSDL2"',
'''LIB_SDL2_LDFLAGS="-L$AROS_SDK/lib -lSDL2 -lGL -lm -pthread"''')
s=s.replace('LIB_libpng_CFLAGS="-I$AROS_SDK/include"',
'''LIB_libpng_CFLAGS="-I$UQM_DEPS/include"''')
s=s.replace('LIB_libpng_LDFLAGS="-lpng -lz"',
'''LIB_libpng_LDFLAGS="-L$UQM_DEPS/lib -lpng -lz"''')
s=s.replace('LIB_zlib_LDFLAGS="-lz"',
'''LIB_zlib_LDFLAGS="-L$UQM_DEPS/lib -lz"''')
s=s.replace('SYMBOL_strcasecmp_EXTRA="#include <strings.h>"',
'''SYMBOL_strcasecmp_EXTRA="extern int strcasecmp(const char *, const char *);"''')
s=s.replace('SYMBOL_stricmp_EXTRA="#include <string.h>"',
'''SYMBOL_stricmp_EXTRA="extern int stricmp(const char *, const char *);"''')
s=s.replace('HEADER_regex_EXTRA="#include <sys/types.h>"',
'''HEADER_regex_EXTRA="#include <sys/types.h>"
HEADER_regex_DETECT="false"''')
s=s.replace('SYMBOL_strcasecmp_DEFNAME="HAVE_STRCASECMP_UQM"',
'''SYMBOL_strcasecmp_DEFNAME="HAVE_STRCASECMP_UQM"
SYMBOL_strcasecmp_DETECT="true"''')
s=s.replace('SYMBOL_stricmp_EXTRA="extern int stricmp(const char *, const char *);"',
'''SYMBOL_stricmp_EXTRA="extern int stricmp(const char *, const char *);"
SYMBOL_stricmp_DETECT="true"''')
p.write_text(s)
PY

# UQM 0.8.0 checks HAVE_GETOPT_LONG before its normal platform header is
# visible, and hides alarm.h behind NETPLAY although Alarm_init is unconditional.
python3 - "$SC2/src/uqm.c" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]); s=p.read_text()
if '#include "config_unix.h"' not in s:
    anchor=' */\n\n#ifdef HAVE_UNISTD_H'
    s=s.replace(anchor, ' */\n\n#include "config_unix.h"\n\n#ifdef HAVE_UNISTD_H', 1)
s=s.replace('#ifdef NETPLAY\n#\tinclude "libs/callback.h"\n#\tinclude "libs/alarm.h"', '#include "libs/alarm.h"\n#ifdef NETPLAY\n#\tinclude "libs/callback.h"', 1)
p.write_text(s)
PY

# AmigaOS has no Unix passwd database; use HOME only.
python3 - "$SC2/src/libs/file/dirs.c" <<'PY_UQM_DIRS'
from pathlib import Path
import sys
p = Path(sys.argv[1])
t = p.read_text()
t = t.replace(
    '#ifdef WIN32\n#\tinclude <direct.h>\n'
    '\t\t\t// For _getdcwd()\n#else\n'
    '#\tinclude <pwd.h>\n\t\t\t// For getpwuid()\n#endif',
    '#ifdef WIN32\n#\tinclude <direct.h>\n'
    '\t\t\t// For _getdcwd()\n'
    '#elif !defined(__amigaos__)\n'
    '#\tinclude <pwd.h>\n\t\t\t// For getpwuid()\n#endif',
    1,
)
t = t.replace(
    '#ifdef WIN32\n\treturn getenv ("HOME");\n#else',
    '#if defined(WIN32) || defined(__amigaos__)\n'
    '\treturn getenv ("HOME");\n#else',
    1,
)
p.write_text(t)
PY_UQM_DIRS

# The Amiga sysroot exposes regex.h without the POSIX reg* implementation.
# Build UQM's bundled GNU regex instead and make its header authoritative.
python3 - "$SC2/src/Makeinfo" <<'PY_UQM_REGEX'
from pathlib import Path
import sys
p=Path(sys.argv[1])
t=p.read_text()
t=t.replace(
    'if [ "$uqm_HAVE_REGEX" = 0 ]; then',
    'if [ "$HOST_SYSTEM" = "AROS" ] || [ "$uqm_HAVE_REGEX" = 0 ]; then',
    1,
)
p.write_text(t)
PY_UQM_REGEX

# libnix has readdir() but not readdir_r(), and does not expose _POSIX_NAME_MAX.
python3 - "$SC2/src/port.h" "$SC2/src/libs/uio/uioport.h" "$SC2/src/libs/uio/stdio/stdio.c" <<'PY_UQM_PORT'
from pathlib import Path
import sys

# General UQM portability header.
p = Path(sys.argv[1])
t = p.read_text()
t = t.replace(
    '#\tifndef NAME_MAX\n#\t\tdefine NAME_MAX _POSIX_NAME_MAX\n#\tendif',
    '#\tifndef NAME_MAX\n'
    '#\t\tif defined(__amigaos__)\n'
    '#\t\t\tdefine NAME_MAX 255\n'
    '#\t\telse\n'
    '#\t\t\tdefine NAME_MAX _POSIX_NAME_MAX\n'
    '#\t\tendif\n'
    '#\tendif',
    1,
)
p.write_text(t)

# UIO has its own portability header with the same NAME_MAX assumption.
p = Path(sys.argv[2])
t = p.read_text()
t = t.replace(
    '#\tifndef NAME_MAX\n#\t\tdefine NAME_MAX _POSIX_NAME_MAX\n#\tendif',
    '#\tifndef NAME_MAX\n'
    '#\t\tif defined(__amigaos__)\n'
    '#\t\t\tdefine NAME_MAX 255\n'
    '#\t\telse\n'
    '#\t\t\tdefine NAME_MAX _POSIX_NAME_MAX\n'
    '#\t\tendif\n'
    '#\tendif',
    1,
)
p.write_text(t)

# libnix provides readdir(), not readdir_r().
p = Path(sys.argv[3])
t = p.read_text()
t = t.replace(
    '\tresult = stdio_EntriesIterator_new(dirHandle);\n'
    '\tresult->status = readdir_r(dirHandle, result->direntBuffer,\n'
    '\t\t\t&result->entry);\n',
    '\tresult = stdio_EntriesIterator_new(dirHandle);\n'
    '#ifdef __amigaos__\n'
    '\tresult->entry = readdir(dirHandle);\n'
    '\tresult->status = 0;\n'
    '#else\n'
    '\tresult->status = readdir_r(dirHandle, result->direntBuffer,\n'
    '\t\t\t&result->entry);\n'
    '#endif\n',
    1,
)
t = t.replace(
    '#else\n'
    '\tfor (; iterator->status == 0 && iterator->entry != NULL;\n'
    '\t\t\titerator->status = readdir_r(iterator->dirHandle,\n'
    '\t\t\titerator->direntBuffer, &iterator->entry))\n'
    '#endif\n',
    '#else\n'
    '#ifdef __amigaos__\n'
    '\tfor (; iterator->entry != NULL;\n'
    '\t\t\titerator->entry = readdir(iterator->dirHandle))\n'
    '#else\n'
    '\tfor (; iterator->status == 0 && iterator->entry != NULL;\n'
    '\t\t\titerator->status = readdir_r(iterator->dirHandle,\n'
    '\t\t\titerator->direntBuffer, &iterator->entry))\n'
    '#endif\n'
    '#endif\n',
    1,
)
p.write_text(t)
PY_UQM_PORT

ln -sf "$P/bin/m68k-amigaos-gcc" "$BUILD/tools/gcc"
ln -sf "$P/bin/m68k-amigaos-gcc" "$BUILD/tools/cc"
ln -sf "$P/bin/m68k-amigaos-g++" "$BUILD/tools/g++"
ln -sf "$P/bin/m68k-amigaos-g++" "$BUILD/tools/c++"
ln -sf "$P/bin/m68k-amigaos-ar" "$BUILD/tools/ar"
ln -sf "$P/bin/m68k-amigaos-ranlib" "$BUILD/tools/ranlib"
cp "$BUILD/config.state" "$BUILD/out/config.state"
(
 cd "$SC2"
 PATH="$BUILD/tools:$PATH" \
 BUILD_HOST=AROS BUILD_HOST_ENDIAN=big AROS_SDK="$SDK" UQM_DEPS="$DEPS" \
 BUILD_WORK="$BUILD/out" \
 CFLAGS="-noixemul -m68040 -m68881 -fno-tree-loop-distribute-patterns -pthread -I$SC2/src/regex -I$SDK/include -I$DEPS/include" \
 CXXFLAGS="-noixemul -m68040 -m68881 -fno-tree-loop-distribute-patterns -pthread -I$SC2/src/regex -I$SDK/include -I$DEPS/include" \
 LDFLAGS="-noixemul -m68040 -m68881 -pthread -L$SDK/lib -L$DEPS/lib" \
 /bin/sh build.sh uqm </dev/null
)
find "$SC2" "$BUILD/out" -type f -name 'uqm*' -perm -111 -print
