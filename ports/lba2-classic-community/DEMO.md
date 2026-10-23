# Original playable demo data

The original 2 June 1997 LBA2 / Twinsen's Odyssey demo is available as
[lba2demo.zip at DOS Games Archive](https://www.dosgamesarchive.com/file/twinsens-odyssey/lba2demo).
It was downloaded and inspected on 23 October 2026 without running any of its
DOS/Windows executables.

- Archive: 14,794,949 bytes, SHA-256
  `5ed55123eca9c699ba04ba56aae0471d557e495bdb085f6ab26402bd9fba1894`.
- Data directory inside the archive: `LBA2/`.
- Confirmed: `RESS.HQR`, `CITADEL.ILE`, `CITADEL.OBL`, `SCRSHOT.HQR`, music,
  and `SAVE/DEMO0.LBA`, `DEMO1.LBA`, `DEMO2.LBA`. ZIP CRC checks pass.
- The retail marker `LBA2.HQR` is absent, matching the upstream demo contract.

`demo-data.json` records the source URL, exact archive hash and every member's
hash and size. This is a checksum/provenance inventory, not a redistributed
copy of the assets. Downloaded data stays under ignored `build/demo-data/`.

## Separate executable

Upstream explicitly requires `LBA2_BUILD_DEMO=ON`. The retail executable must
not be used with these files. The demo uses separate settings/saves and has
its original three-scenario behaviour. The separate manifest and wrapper keep
the existing retail build unchanged:

```sh
STOVE=/path/to/open3-stove SDL3_SDK=/path/to/openrtg-sdl3-output \
  sh ports/lba2-classic-community/build-demo-amigaos3.sh --out "$PWD/build/lba2-demo"
```

Once staged on an Amiga, point this demo executable's `--game-dir` at the
extracted `LBA2` directory and use a separate writable `--user-dir`. Original
installers and Windows/DOS sound DLLs are not needed by the community engine.
The demo does not ship retail voices or cutscenes, so it cannot qualify those
features of the full game.

## Data policy

The archive's README identifies the package as the playable demo, dated
2 June 1997, and reserves Adeline Software International's rights. The mirror
describes it as a playable demo. Public availability is not treated as a grant
to rebundle, modify or republish those assets. Keep the complete original
download and notices locally; do not commit assets or include them in our
release package without a separate redistribution-rights determination.

The source engine's GPL licence covers its code, not these assets. Demo-data
discovery and successful host loading do not establish Amiga runtime or
release qualification.
