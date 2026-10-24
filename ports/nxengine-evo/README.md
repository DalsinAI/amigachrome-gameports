# NXEngine-evo — AmigaOS 3.x / AC090

Status: **COMPILED / TESTING**, not RELEASE.

Pinned upstream: `nxengine/nxengine-evo` commit
`1f093d1423cc395eb199230cd609b806ef1daa36`.

## Approved compiler build

Use `STOVE=/path/to/verified-open3 sh ports/nxengine-evo/build-amigaos3.sh`.
The recipe requires GCC 16.2.0-open3 and verified zlib/PNG/JPEG dependencies.
It enables portable adjacent data and the 68040/FPU null-pointer compiler
workaround. It no longer relies on a host-installed data directory.

The 24 October clean build produced an AmigaOS loadseg executable with SHA-256
`3b3773b7b1ec63c31e09f719695dc72b4d474a5b00a5b632c6065908c4c8f866`.
Unsupported visibility attributes in bundled fmt are warnings; target runtime
qualification remains separate from compile success.

## Local external data

Cave Story assets are not bundled. Prepare an owner-authorised local archive:

```sh
python3 ports/nxengine-evo/import_data.py \
  --archive /path/to/cavestory-nx-data.zip \
  --engine-data build/game-ports/sources/nxengine-evo/data \
  --out build/nx-test-data
```

The importer creates a new directory and digest receipt, flattens the old
`data/data` mirror layout, overlays the pinned engine metadata, and adds original
music filenames only where size and CRC match upstream's extraction table.
It rejects unsafe/conflicting archive paths and never replaces an existing
output. The observed external archive SHA-256 is
`444c4265b5acc11495b8d5b00ef7e7843b5ee8e0f6a5d9c4201ed36adbd018f0`;
there is no publisher digest in the manifest. This is local integrity evidence,
not a redistribution grant. The tested import contains 661 files including
26 music aliases. Keep the executable beside `data` in the isolated lab.

## Runtime gate

The earlier open3 candidate reached the title, opening dialogue and Start Point
room. Right-arrow input moved the player and the Quit menu returned RC 0.
The SDL software renderer cannot satisfy this engine's accelerated-renderer
request; leave `SDL_OPENGPU` unset for the OpenGPU path.

The fresh binary repeated the opening room and movement, captured a jump
in flight, wrote a save, exited RC 0, reloaded the save in a fresh process at
the saved disk position, then exited RC 0 again. The restored music files no
longer produce missing-song warnings. See [the run receipt](evidence/runtime-20261024.json).
Audio (including remaining missing-PXT warnings), weapon/fire, wider input and
window handling, and an intended clean-ROM repeat remain release gates. A muted lab run does not establish audible audio.
