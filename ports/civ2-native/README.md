# Civ II MGE: experimental native 68040 port

The reconstructed game is compiled to wasm32 on the build PC, translated to C
with wasm2c 1.0.41, then built with GCC 16.2.0-open3 as an Amiga Hunk executable.
There is no WebAssembly interpreter or browser in the Amiga execution path.
See SOURCE.json for the exact reviewed pin. This is not a RELEASE entry.

## Results on 24 October 2026

- Full reconstructed game built to WebAssembly, then generated C.
- All eight generated game C files, WABT runtime and native host files compiled
  and linked for 68040/FPU. The initial executable is approximately 5.1 MiB.
- The synthetic memory test passed on big-endian m68k Linux/QEMU and in an
  isolated AC090 Amiga lab with exit code 0: unaligned LE integers, float,
  host strings, bulk copy, memory growth, bounds trap and teardown.
- The actual game imported its native launch options, loaded original MGE
  assets/fonts and rendered the first-run diplomatic-herald preference dialog.
- The upstream menu replay reached its screenshot and logged script completion,
  but the first-run dialog prevented reaching the menu, and guest clean exit
  has not returned. Replay mode deliberately ignores physical mouse/keyboard.
  Do not mistake the visible dialog for a demonstrated input failure.

Outstanding gates: resolve exit/SDL teardown, handle fresh-profile setup in the
replay, then interactive menu, new game, city founding, turns, save/load, audio,
clean exit and repeat. No speed, complete compatibility or gameplay claim.

## Build

Clone the pinned upstream source outside the gameports repository. Install its
host-side tools with the reviewed `tools/fetch-toolchain.sh` (WASI SDK 25 and
WABT 1.0.41); this downloads build tools, not game assets. Then:

```sh
python3 ports/civ2-native/build_native.py \
  --source /absolute/path/to/pinned-civ2 \
  --stove /absolute/path/to/team1-open3/prefix \
  --output /absolute/path/to/separate-build-directory --jobs 2
```

`--skip-generation` reuses the already-generated upstream C. `--prepare-only`
prepares adapted source without compiling. Native objects, generated game C,
font files, original data and executables belong outside git. This recipe is
not connected to catalog.json while gameplay/exit gates remain open.

The native C99 runtime explicitly uses one module on one guest task. It has
forward-addressed little-endian linear memory, malloc/bounds checks instead of
large virtual guard mappings, ordinary setjmp/longjmp instead of POSIX signal
recovery, and no TLS requirement. Shared memory, SIMD WebAssembly modules and
concurrent callers are unsupported. The scalar-memory adaptation is checked
against the pinned wasm2c template and fails if the expected form changes.

The host bridge converts scalar values, events and directory records; uses
byte-defined BGRA pixels; supplies missing vectored I/O; maps virtual volume
paths to Amiga paths; and imports explicit launcher options because this
native C runtime did not populate the POSIX environ array from Shell SetEnv.

## Lab launch

Use an isolated lab with the current qualified native Open libraries. Import
MGE resources into Games:Civ2 and put the upstream font data in NativeData.
No original assets are bundled here. Example bounded upstream replay:

```text
Stack 4194304
CD Games:Civ2
Games:Civ2/OpenCiv2 Games:Civ2 --data NativeData --replay menu-smoke.txt --shots Shots --show
```

Launch without `--replay` for actual interactive input; `--show` displays a
replay but does not turn it into an interactive game. First-run preferences
must be covered explicitly by a replay. Our tested lab ROM was the existing
24 October 11:00 all-main baseline, not the pending interim ROM.

## Memory probe

`tests/memory-probe.wat` contains no game data. Generate its C with WABT 1.0.41,
copy the matching wasm-rt headers/runtime into the build folder, run
`compat/prepare_forward_memory.py BUILD`, then compile it with probe-main.c and
the three wasm-rt implementation C files. Use the same native flags as the
recipe. The probe's PASS is a runtime-boundary result, not a game qualification.
