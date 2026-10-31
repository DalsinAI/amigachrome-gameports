# Build GrafX2 for OpenUp / AmigaOS 3.2.3

Use OpenAmigaGCC **16.2.0-open3** for a 68040 with soft float (no FPU needed),
and the bundled OpenNDK headers or NDK 3.2 R4. The executable uses libnix's nix20 CRT profile.

For the already-prepared source included in the package:

```sh
export OPENAMIGA_GCC=/absolute/path/to/openamigagcc-16.2.0-open3-x86_64-linux
export AMIGA_NDK=/absolute/path/to/OpenNDK/Include_H
export OPENAMIGA_SPECS=/absolute/path/to/openamigasdk/Lib/m68k-soft-m68040.specs
export GRAFX2_WORK=/absolute/path/to/a-new-build-directory
mkdir -p "$GRAFX2_WORK"
ln -s /absolute/path/to/source/grafx2 "$GRAFX2_WORK/grafx2"
python3 source/native-port/build.py
```

For fresh upstream sources, set the same variables, use an empty GRAFX2_WORK,
and run `prepare.py` before `build.py`. Preparation pins upstream and the 6502
archive, checks its checksum, and applies the complete native/OpenUp patch.
It refuses to overwrite changed prepared sources. Build SDK headers are bundled
under native-port/sdk; their exact repositories and commits are recorded in
SDK-PROVENANCE.json. SDK headers participate in the compile cache fingerprint.

Build flags include -m68040 -specs=$OPENAMIGA_SPECS -O2 -mcrt=nix20,
-fno-delete-null-pointer-checks and -fno-late-combine-instructions. The latter
avoids the GCC16 argument-copy issue also documented by OpenMulticore's build.
There is no static zlib, SDL, SDL_image, SDL_ttf, Mesa, X11, libpng or libtiff
in the link. OpenZlib supplies the compressed GPX handler through z.library.

The output is `GRAFX2_WORK/build/GrafX2`, alongside its link map.

## Checks

The native OS-call mocks generate an isolated pointer-width header shim when
using OpenNDK on a 64-bit host. They never modify the target headers:

```sh
export GRAFX2_SOURCE=/absolute/path/to/source/grafx2
sh source/native-port/tests/run.sh
```

They exercise clipping, input, mode rollback, teardown, image import, missing
writers and save/commit failures. They are host tests, not OS ABI qualification.
`tests/expand_host.c` compares the pure palette worker to a byte model under
ASan/UBSan, including padded pitches and sentinels.

`tests/openup_target.c` links the real integration sources and runs inside the
68k guest with installed OpenUp libraries. It checks OpenFont brushes,
OpenMulticore range conversion, OpenZlib data and error cases, PNG save/reload,
binary alpha export, and the full editor-sized PNG fixture. See the packaged
validation report for exact boundaries. A cores board and hardware GPU were
not available in FS-UAE.

## RC1 target checks and packaging

`python3 source/native-port/tests/build_target.py` builds the guest integration
checks, palette regression, WBStartup and icon launch probes, input helper and
classic icon generator, with the same soft-float flags as the editor.

Generate `colour-fixture.png` with `tests/make_fixture.py`, and place it beside
the editor under the isolated guest's `SYS:GrafX2/`. `openup-target` requires
that fixture and `SYS:DejaVuSans.ttf`, plus the installed OpenUp libraries.
`palette-target` tests the actual editor component-rounding function. Run
`workbench-target project` with no other editor instance open; exit the editor
normally to finish its visible-screen and WBStartup reply checks. With Workbench
running, `workbench-icon-target` asks it to open the packaged tool icon.

`make-icon` writes `SYS:GrafX2/GrafX2.info` using original two-plane artwork.
Supply that file to `package.py --icon PATH`. Packaging checks the HUNK and
classic icon signatures, includes the complete prepared source and ordered
overlay, and writes per-file SHA256SUMS plus an archive checksum. Runtime
libraries, OS disks and ROMs are not included. Install the OpenUp stack normally.

The source cache fingerprints include the compiler identity and NDK/SDK header
contents. `build/build-info.json` records compiler, flags, source pins, ordered
patch hashes, header hashes and the executable hash. TEST3's root manifest is
historical and is not substituted for this fresh build record.

## Cradle lab build

`build-amiga.sh` at the repository root is Cradle's stove_build entry point. It
fetches the pinned open3 compiler and source release by SHA-256, takes OpenNDK
and m68k-soft-m68040.specs from the source release, and writes OUT/GrafX2,
OUT/target-tests and OUT/colour-fixture.png for a lab.
