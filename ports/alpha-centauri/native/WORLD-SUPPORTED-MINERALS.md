# Fresh support connected to fresh mineral accounting

`world_supported_minerals_probe.cpp` composes the connected
`world_base_support_policy` calculation with `world_base_minerals`. Its mineral
stage receives the fresh support result for the same base; the caller supplies
no maintenance number. This is a read-only composition proof for the owning
accounting integration, not a second mutation or turn-dispatch API.

Support follows the explicitly adopted Thinker default restoration policy
described in `BASE-SUPPORT.md`; recovered-original support parity is unproven.
Mineral accounting follows the recovered OpenSMACX routine and its connected
bindings in `WORLD-BASE-MINERALS.md`. Neither component is relabelled to imply
the same provenance. Only human base-game, supported stable-worker worlds are
accepted. No unsupported AI, worker allocation, support-shortage disbanding,
production completion, combat, diplomacy or End Turn is synthesized.

## Genuine Boreholes result

The actual promotional scenario was imported unchanged with the genuine
`alpha.txt` currently staged for the native frontend. Its input SHA256 values
are recorded in `WORLD-SUPPORTED-MINERALS-EVIDENCE-20261026.json`; proprietary
inputs are not committed. The report enumerates all 11 bases. Bases 0–8 are
explicitly rejected by the human-policy boundary, and bases 9–10 calculate:

| Base | Supported units | Paid units | Fresh support | Mineral intake | Consumption | Surplus | Eco damage |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 9 | 4 | 2 | 2 | 6 | 2 | 4 | 0 |
| 10 | 3 | 1 | 1 | 7 | 1 | 6 | 0 |

Both bases have pending SUPPORT/POLICE 0, two free units, zero pacifism units,
and zero incoming/outgoing mineral convoys. Base 9's paid vehicles are 20 and
21; its free vehicles are 2 and 19. Base 10's paid vehicle is 23; its free
vehicles are 7 and 22. The support projection equals the existing flags in
this particular scenario. The fresh calculation is independently verified;
saved flags and maintenance were not used as answers.

## Validation and reproduction

`test_world_supported_minerals.py` uses the independent support index-set and
wire/mineral integer models. Its synthetic cases derive actual support slots
and paid units from imported actors rather than pass maintenance numbers.
All 32 synthetic cases passed host ASan/UBSan and strict warnings; 16 of those
had nonzero maintenance, including mineral deficits. An additional genuine
Boreholes case passed, with every support summary, projected home-vehicle
state and mineral result checked against the models.

```sh
python3 ports/alpha-centauri/native/test_world_supported_minerals.py \
  --out /absolute/new/evidence-directory \
  --original /private/Borehole.SC --original-rules /private/alpha.txt

CXX=/absolute/toolchain/bin/m68k-amigaos-g++ \
  sh ports/alpha-centauri/native/build_world_supported_minerals_probe.sh \
  /absolute/world-supported-minerals
```

The native HUNK built with strict 68040/FPU flags and ran inside a separate
FS-UAE 3.2.35 / AROS 20261023 copy, using the previously documented disposable
libnix FPU40 compatibility patch. Guest command:

```text
Stack 1048576
world-supported-minerals alpha.txt Borehole.SC >report.log
```

The command returned **0**. Its complete 714-byte report matched the host
report byte for byte, SHA256
`66515cea7d217fe95494daac2a6551d52f09c3aec5538b0c9f1d4864a4e59582`.
This proves this calculation path under AROS, not AmigaOS 3.2.3 qualification
or a complete playable turn.

## Projected-state serialization boundary

Support controls exactly the vehicle `state` mask **0x00600010**:
`0x10` requires support, `0x200000` pacifism drone, and `0x400000` free skip.
The original vehicle `state` is **uint32 little-endian at record offset 0x4**.
It is distinct from the uint16 `flags` at offset 0x8. The home-base reference
is int16 little-endian at offset 0x2e. Preserve every unrelated state bit.

The existing `local_actions_snapshot.cpp` encoder already writes each vehicle
field through `original_vehicle_fields.inc`, including that full state word.
No wire-layout patch is needed. Applying a projection requires the owning
transaction's explicit authority and validation boundary; this probe does not
modify that boundary, write a save, or advance runtime state.
