# Alpha native frontend SDK restoration

`scripts/build_alpha_native_sdk.sh` rebuilds the actual OpenGPU SDL dependency
set used by the Alpha frontend. It uses the project's real AmigaOS backend,
SDL module loader and SDL_ttf library. It does not substitute host SDL or
emulate Amiga APIs with source stubs. The SDL `libSDL2.a` is the project's
normal module dispatch link library; `SDL2.module` contains the implementation.

## Prerequisites and build

Use a disposable Linux x86-64 OpenAmigaGCC **16.2.0-open3** installation and
its matching source release from the [official open3 release](https://github.com/DalsinAI/openamigagcc/releases/tag/v16.2.0-open3). The release contains the compiler, sfdc, fd2sfd,
fd2pragma and the official NDK installer. Host tools needed are Bash, Python
3.12 or newer, a C compiler, make, git, curl, patch, rsync and sha256sum.
The script builds lhasa and vasm from pinned source commits.

```sh
GCC_PREFIX=/absolute/path/openamigagcc-16.2.0-open3-x86_64-linux \
GCC_SOURCE=/absolute/path/openamigagcc-16.2.0-open3-src \
  bash scripts/build_alpha_native_sdk.sh /absolute/path/alpha-sdk-lab \
  > /absolute/path/alpha-sdk-build.log 2>&1

CXX=/absolute/path/openamigagcc-16.2.0-open3-x86_64-linux/bin/m68k-amigaos-g++ \
OPENUP_SDK=/absolute/path/alpha-sdk-lab/build \
  sh ports/alpha-centauri/native/build_local_player_frontend.sh \
  /absolute/path/alpha-restoration-local
```

Use a fresh output directory after a failed source extraction or patch step.
Repeated downloads are hash checked. The output is a local build workspace,
not a redistributable archive: it contains NDK and third-party developer
headers. Do not commit or publish that directory. Only this recipe, source
changes, hashes and evidence belong in the gameports repository.

The NDK installer modifies only the selected compiler prefix. On the cloud
AROS lane, apply the documented **local libnix FPU40 compatibility patch** to
that disposable compiler before linking the frontend. See
`toolchain/libnix-fpu40-check.patch` and `docs/ALPHA_FS_UAE_LAB.md`. Preserve the
original archive. The patch accepts `AFF_68881` or `AFF_FPU40`; it retains the
no-FPU failure path. This script does not alter the runtime check or silently
patch a user's installed compiler.

## Pinned inputs

| Input | Revision or SHA256 |
| --- | --- |
| OpenAmigaGCC binary archive, open3 | `9fcc136ab59e3bb5b91d2764d0bf290b709d72d7380be4ce0cdf005edf8eff21` |
| OpenAmigaGCC source archive, open3 | `d8b7cf2101cf70df1cf719d199230260790420f43ec6e899f3d061a85ff3bcda` |
| openamigartg source commit | `fb9b556f15f73dc9b6fb1f3dc8f461508efb34fe` |
| lhasa source commit | `75ed83559f23e9538e0045c62f53f77ab03d03d6` |
| vasm mirror source commit | `a13e7e728a3dd5a9ba468bf479119e0bc23e70fd` |
| NDK3.2.lha | `96cabd4ad683dced632e147bf86dee0f50dcb1254386216c25c362916a6409bb` |
| CGraphX-DevKit.lha | `dd2610649a692863dc27bbce595520b0ee269a03e03f73a8858396127df0fb0c` |
| m68k-amigaos-ahidev.lha | `de3571879210ad9ca179fbbc7a8c0591dfd0b673d6785657d8796fc1d7a147ec` |

The pinned `openamigartg/library/modules/sdl2/UPSTREAM.json` supplies source
URLs and SHA256 values for SDL **2.32.10**, SDL_ttf **2.24.0**, FreeType
**2.14.3** and libxmp **4.7.3**. Alpha does not link libxmp; its source is
needed because the satellite Makefile includes its build rules.

The legacy CyberGraphX proto header contains compiler-specific pragmas. The
recipe generates genuine GCC proto/inline headers from the archive's FD and
C prototypes using the released compiler's tools. The frontend uses external
SDK headers as system includes so their generated helper functions do not
disable strict warnings on project code. The patched SDL_ttf header uses
`typedef struct TTF_Font TTF_Font`, matching the frontend's forward declaration.

## Runtime staging and observed build

Copy `build/SDL2.module` to **LIBS:OpenGPU/SDL2.module** on the isolated guest.
The genuine loader falls back to DOS LoadSeg when `opengpu.library` is absent.
Set **SDL_OPENGPU=0** for FS-UAE's software lane. Retain AROS's existing
CyberGraphX implementation; the OpenRTG hardware driver is not required here.
The runtime font implementation is linked into the frontend through
`libSDL2_ttf.a`; no additional TTF module is needed.

On 26 October 2026 the source build completed and its module residency and
FPCR checks passed. These were the initial build outputs, before any later
runtime debugging changes:

| Output | Bytes | SHA256 |
| --- | ---: | --- |
| SDL2.module | 1001352 | `2bc785a1528ff814ea019b47b9f02e36c7a606ac43e20437780d508bdb14fc88` |
| libSDL2.a | 116388 | `1496a3ec8ebfa0dfb7d4f8817e50f22f4220a7a5b476826ba4fcc472df9da35b` |
| libSDL2_ttf.a | 595260 | `e2ac90da496ab300344604895e64897d6ab0f15d1ca3973ae001c46836a11b49` |

Treat hashes as evidence of that build, not a promise of byte reproducibility:
absolute source paths can appear in the module. The complete recipe was also
replayed successfully against the existing dependency workspace: its NDK
installation, source preparation, module/link-library build, and FPCR checks
all completed with return code 0. This checks restoration with cached inputs;
it is not a second independent clean build. A successful compile does
not qualify the game UI or AmigaOS 3.2.3 runtime. Record actual execution and
screenshots separately in the FS-UAE lab evidence.

## Fresh restoration on 27 October 2026

The compiler and matching source release were downloaded and hash checked
again into a fresh workspace. The real SDK recipe completed with return code
zero; all three output hashes above matched the earlier build exactly. The
genuine AROS archive was hash checked and 23,579 files extracted. A rebuilt
native local-player controller executed in FS-UAE with 145 checks and guest
return code zero. Its complete 144,918-byte checkpoint matched the independent
host oracle, SHA256
`843665734047b513c4900a2fae0dc3acc714d3ad135ca3b55537c30e1793661a`.

The public scenario mirror returned HTTP403 to urllib's default User-Agent.
`stage_alpha_demo_assets.py` now identifies itself as `Alpha-native-lab/1.0`;
a fresh scenario download then matched the pinned digest and all 572 original
demo/scenario files were staged. The archive digest gates remain unchanged.

The native-world runner now observes the guest shell's `WORLD_RC` completion
marker and stops its own emulator child promptly. The hard execution deadline
still applies when the guest does not finish. Neither the input helper nor
the game executable is changed by this process-control adjustment. Recorded
receipts distinguish a completed guest from an emulator timeout.
