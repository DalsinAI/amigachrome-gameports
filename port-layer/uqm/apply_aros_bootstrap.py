#!/usr/bin/env python3
"""Apply a deterministic AROS/m68k cross-build profile to UQM 0.8.0."""
from __future__ import annotations

import argparse
from pathlib import Path


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise RuntimeError(f"UQM AROS patch anchor missing: {label}")
    return text.replace(old, new, 1)


def find_sc2(source: Path) -> Path:
    source = source.resolve()
    candidates = (source, source / "sc2")
    for candidate in candidates:
        if (candidate / "build.sh").is_file() and (candidate / "src" / "uqmversion.h").is_file():
            return candidate
    raise RuntimeError("UQM 0.8.0 source root not found")


def verify_version(sc2: Path) -> None:
    version = (sc2 / "src" / "uqmversion.h").read_text(encoding="utf-8")
    required = (
        "#define UQM_MAJOR_VERSION     0",
        "#define UQM_MINOR_VERSION     8",
        "#define UQM_PATCH_VERSION     0",
    )
    if not all(item in version for item in required):
        raise RuntimeError("UQM source does not identify itself as 0.8.0")


def apply(source: Path) -> Path:
    sc2 = find_sc2(source)
    verify_version(sc2)

    functions = sc2 / "build" / "unix" / "config_functions"
    text = functions.read_text(encoding="utf-8")
    text = replace_once(
        text,
        """			[Qq][Nn][Xx])
				HOST_SYSTEM="QNX" ;;
""",
        """			[Qq][Nn][Xx])
				HOST_SYSTEM="QNX" ;;
			[Aa][Rr][Oo][Ss])
				HOST_SYSTEM="AROS" ;;
""",
        "AROS host system",
    )
    functions.write_text(text, encoding="utf-8")

    build_config = sc2 / "build" / "unix" / "build.config"
    text = build_config.read_text(encoding="utf-8")
    text = replace_once(
        text,
        """uqm_do_config()
{
	# Show the menu and let people set things
	do_menu MENU main "$BUILD_WORK/config.state"
	echo "Configuration complete."
}
""",
        """uqm_do_config()
{
	if [ "$HOST_SYSTEM" = "AROS" ] && [ -r "$BUILD_WORK/config.state" ]; then
		echo "Using pre-seeded AmigaChrome AROS configuration."
		return
	fi
	# Show the menu and let people set things
	do_menu MENU main "$BUILD_WORK/config.state"
	echo "Configuration complete."
}
""",
        "non-interactive AROS config",
    )
    build_config.write_text(text, encoding="utf-8")

    host = sc2 / "build" / "unix" / "config_proginfo_host"
    text = host.read_text(encoding="utf-8")
    text = replace_once(
        text,
        'SYSTEM_HOST_CFLAGS=""\n\n# LDFLAGS\nSYSTEM_HOST_LDFLAGS=""\n',
        '''case "$HOST_SYSTEM" in
	AROS)
		SYSTEM_HOST_CFLAGS="--sysroot=$AROS_SDK -m68040 -fno-delete-null-pointer-checks -I$AROS_SDK/include -I$AROS_SDK/include/SDL2"
		SYSTEM_HOST_LDFLAGS="--sysroot=$AROS_SDK -m68040"
		;;
	*)
		SYSTEM_HOST_CFLAGS=""
		SYSTEM_HOST_LDFLAGS=""
		;;
esac
''',
        "AROS system flags",
    )

    text = replace_once(
        text,
        'case "$HOST_SYSTEM" in\n\tWINSCW|GCCE|ARMV5)\n\t\tLIB_SDL2_DETECT="false"\n',
        '''case "$HOST_SYSTEM" in
	AROS)
		LIB_SDL2_DETECT="true"
		LIB_SDL2_CFLAGS="-I$AROS_SDK/include/SDL2"
		LIB_SDL2_LDFLAGS="-lSDL2"
		LIB_SDL2_VERSION="2.32.10"
		;;
	WINSCW|GCCE|ARMV5)
		LIB_SDL2_DETECT="false"
''',
        "AROS SDL2",
    )

    text = replace_once(
        text,
        '''case "$HOST_SYSTEM" in
	Darwin)
		if not have_framework libpng; then
''',
        '''case "$HOST_SYSTEM" in
	AROS)
		LIB_libpng_CFLAGS="-I$AROS_SDK/include"
		LIB_libpng_LDFLAGS="-lpng -lz"
		LIB_libpng_DETECT="true"
		LIB_libpng_VERSION=""
		;;
	Darwin)
		if not have_framework libpng; then
''',
        "AROS libpng",
    )

    text = replace_once(
        text,
        '''case "$HOST_SYSTEM" in
	MINGW32*|CYGWIN*|cegcc)
		LIB_zlib_LDFLAGS="-lzdll"
''',
        '''case "$HOST_SYSTEM" in
	AROS)
		LIB_zlib_LDFLAGS="-lz"
		LIB_zlib_DETECT="true"
		LIB_zlib_VERSION=""
		;;
	MINGW32*|CYGWIN*|cegcc)
		LIB_zlib_LDFLAGS="-lzdll"
''',
        "AROS zlib",
    )
    host.write_text(text, encoding="utf-8")

    marker = sc2 / ".amigachrome-aros-bootstrap"
    marker.write_text("UQM 0.8.0\n", encoding="utf-8")
    return sc2


def config_state() -> str:
    return """CHOICE_debug_VALUE='nodebug'
CHOICE_graphics_VALUE='sdl2'
CHOICE_sound_VALUE='mixsdl'
CHOICE_mikmod_VALUE='internal'
CHOICE_lua_VALUE='internal'
CHOICE_ovcodec_VALUE='none'
CHOICE_netplay_VALUE='none'
CHOICE_joystick_VALUE='enabled'
CHOICE_ioformat_VALUE='stdio'
CHOICE_accel_VALUE='plainc'
CHOICE_threadlib_VALUE='sdl'
INPUT_install_prefix_VALUE='/PROGDIR'
INPUT_install_bindir_VALUE='$prefix'
INPUT_install_libdir_VALUE='$prefix'
INPUT_install_sharedir_VALUE='$prefix'
"""


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("--write-config", type=Path)
    args = ap.parse_args(argv)
    sc2 = apply(args.source)
    if args.write_config:
        args.write_config.parent.mkdir(parents=True, exist_ok=True)
        args.write_config.write_text(config_state(), encoding="utf-8")
    print(sc2)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

