# S5 metadata and state boundaries

OpenLoco pin: `6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e`.

Patch 0014 decodes the 32-byte S5 header, save-preview metadata and complete
scenario-options metadata explicitly as little-endian. Text, preview pixels,
padding and byte fields remain bytes. Scenario options include all four 16-bit
fields, three 32-bit objective quantities and three embedded object headers.
The original files and save format are unchanged.

The saved simulation is a separate schema: companies, towns, stations,
industries, entities, routings, orders and map elements. Decoding a header does
not convert that state. Big-endian full-state loads therefore stop with
`S5 game-state endian conversion is not implemented`. Save export also refuses
on those hosts, before the path overload opens/truncates a file or the stream
overload modifies state. This temporary guard must remain until both import
and compatible export are implemented and tested. It does not make OpenLoco
playable.

For supported little-endian state loads, exact chunk sizes are required before
copying. This prevents short state chunks from being overread and prevents
scenario chunks from overrunning their actual destination regions. Legacy
fix flags are read only after a bounded two-byte read. Tile chunks must contain
whole elements and fit the engine's maximum. This is a local S5 boundary fix;
it does not claim a general allocation-limit audit of the Sawyer decoder.

## Reproduce

Use the offline port recipe to export the patched source, then configure
`ports/openloco/probes` as described in ASSET_ENDIAN.md. The `port-probes`
target now includes `s5-wire`, and CTest includes `s5-wire-boundaries`.

```sh
cmake --build build/endian-probes --target port-probes -j2
ctest --test-dir build/endian-probes --output-on-failure
build/endian-probes/s5-wire /path/to/Locomotion/Data/title.dat /path/to/scenario.SC5
```

The no-argument synthetic tests cover unaligned header input, non-palindromic
metadata values, preservation of byte arrays, truncated/oversized chunks,
unchanged destinations on refused copies, tile alignment/count limits, legacy
flags, unsupported versions and simulated/native endian guards. Assertions
remain enabled in Release builds. File arguments additionally check checksums,
metadata and expected state/tile chunk boundaries without importing simulation
state. Scenarios without a generated map correctly omit the tile chunk.

Build a separate HUNK `s5-wire` with the existing Amiga toolchain and verified
open3 stove. In an isolated guest, set `Stack 2097152` before running the probe.
A cross-link alone is not a target test. User-supplied assets and decoded state
stay local and must not be committed.

See [the dated evidence](evidence/s5-20261024/README.md) for actual results and limits. Gameplay, audio,
complete state conversion, save/load compatibility and clean-instance release
qualification remain open.
