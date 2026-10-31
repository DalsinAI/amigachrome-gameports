# S5 wire conversion and startup fixes

OpenLoco pin: `6d7f5b1bfb4abd5ae2f1a2ade628a2b0d46f861e`.

Patch 0014 introduced explicit little-endian metadata decoding and exact
state/tile chunk boundaries. Patch 0015 extends that boundary to the full
fixed simulation schema and replaces its temporary big-endian state/save
refusal with import/export conversion. On little-endian machines the data
stays native; on big-endian machines scalar fields are converted at the wire
boundary. Padding, text and byte arrays remain bytes. Export uses temporary
wire records without swapping the live game state.

The walk covers modern and legacy state, companies, towns, stations,
industries, entity variants, routing, required object headers and packed map
fields. Currency48 converts its low 32-bit and high 16-bit members separately.
Entity discriminators select the active payload. Packed order data already
uses explicit byte operations. Only actual TVector2/TVector3 specializations
use the vector walk: records with x/y fields still visit all other fields.
`schema/s5-state-fields.json` records the pinned field declarations used in
the audit; it contains no retail data or local source paths.

Patch 0016 recovers from empty/truncated scenario caches without reporting
an allocation failure, validates the cache extent before allocating, and
fixes text content sharing an image/content union on big-endian machines.
Text reads use the numeric low 16 bits and preserve the null string marker.
The G1 run count also uses an unsigned size for strict compiler checks.

## Validation and limits

The six host probes pass. The complete state probe also passed the title
file and all 45 locally supplied scenarios, reproducing the original wire
payloads after conversion. On the 68k lab, its explicit named-field/wire-byte
checks passed and title/scenario decoded-value hashes matched the host.
The separate widget probe passed on both host and 68k, including OK text,
content low bits, assignment and the null string marker. Assertions remain
enabled in Release builds.

Earlier full-app tests reached the title and a scenario with an incomplete
state walk; those screenshots do not qualify this corrected converter.
Full-app repeat, save/load compatibility, audio and the intended OpenKick
ROM test remain open. The current Amiga recipe still disables audio. These
patches are engineering progress, not a playable or release certification.
The older `evidence/s5-20261024/` records describe patch 0014's checkpoint.

## Reproduce

Export the exact pinned source with the offline port recipe, then configure
`ports/openloco/probes` as described in ASSET_ENDIAN.md. Build `port-probes`
and run CTest. `s5-state` accepts user-supplied title, scenario or save paths;
it checks the Sawyer checksum, schema bounds, typed values and unchanged
wire round trips. A forced double-swap alone is not sufficient: explicit
wire-byte and named-field checks are included too.

```sh
cmake --build build/endian-probes --target port-probes -j2
ctest --test-dir build/endian-probes --output-on-failure
build/endian-probes/s5-state /path/to/Locomotion/Data/title.dat /path/to/scenario.SC5
```

Build separate HUNK probes with the verified open3 stove. In an isolated
guest set `Stack 4194304`, then run `widget-content` and `s5-state`. A cross-link
alone is not a target test. Keep retail files, decoded state and raw local
logs outside commits and public artifacts.

## Lab result (31 October)

On the 68040 lab (Instance-32 copy) the converter loads the retail Boulder
Breakers scenario, saves it (763,801 bytes, 148,484 tile elements), autosaves,
and loads that save back from the title screen. The earlier "save hangs at
Please wait" was not in S5: it was the SDL3 cursor loop fixed by patch 0018.
