# Separate Combat CRT State

`combat_crt_draw` reproduces the original demo's 32-bit CRT `rand` arithmetic:

```
state = state * 214013 + 2531011   (unsigned 32-bit wrap)
value = (state >> 16) & 32767
```

`CombatRandomState` is explicit and separate from `OriginalWorld::runtime_seed`
and the recovered movement `integer_random` generator. The latter uses a
different recurrence and a different range mapping. No map seed, saved tick,
movement seed, wall clock or implicit initial value creates combat authority.
The caller must provide and eventually persist the actual separate CRT state.

`resolve_seeded_combat_rounds` composes the qualified conventional round
resolver with this generator. Only actually consumed draws advance public
state; zero/one-strength branches consume none. It uses a bounded private tape
and commits the corresponding endpoint, preserving state/result on rejection.
`world_resolve_seeded_field_combat` adds current-world strength facts atomically
under the same support boundaries as `world_field_combat`. It remains a proposal:
it does not spend moves, change world RNG, remove casualties or commit an attack.

## Original Instruction Evidence

Private original `terranx.exe`, extracted from the hash-pinned official Alien
Crossfire demo used by `scripts/stage_alpha_demo_assets.py`, SHA256:
`28608395dc4184b0ac972ea0886a8becd57d91a07bbabaef6f798fc5ca94e6b6`.

The actual x86 CRT routine is at `0x63724d`. It calls thread-data lookup at
`0x63a3e3`, reads/writes a32-bit state at `PTD+0x14`, multiplies/adds the above
constants and returns the masked high15 bits. Its neighboring `srand` routine
at `0x637240` stores the explicit supplied seed into the same location.

`test_combat_crt.py --original-demo PATH` executes the actual private original
machine instructions in Unicorn2.1.4. Only the thread-data lookup is intercepted
to supply a controlled thread-local data pointer; the arithmetic, state store,
return value and call/return instructions execute from the original bytes.
8512 calls over266 initial seeds all match an independent arithmetic model.
Complete68096-byte state/value output SHA256:
`1231fe0559196da2dcc259f9375e57284f78e2a5c5bc0e71f16ea2ba36fbd4a2`.

The same test executes the actual original combat round-decision block at
`0x4ffd8d`: attacker CRT call `0x4ffd9b`, defender call `0x4ffdb6`, comparison
`0x4ffdc1`. Controlled strength locals cover zero, one, small/large strengths,
all combinations,32 distinct seeds and1152 decisions. Every actual state,
attack roll, defense roll and selected damage branch matches. This directly
checks attack-first draw order, modulo mapping, no-draw zero/one branches and
defender-wins-ties. Complete18432-byte result SHA256:
`792b16a7fe9c8a1a9177cb4f3a5a5512742b0b87a472769932580326e5bc9543`.

Execution stops before each damage branch. This is scoped original instruction
evidence, not a full Windows game run, recovered whole combat routine, damage
loop parity, or AmigaOS3.2.3 test. The conventional full-duel policy remains
explicitly attributed to pinned Thinker in `FIELD-COMBAT.md`. No original code
bytes or executable are committed or redistributed by these tests.

## Host And Native Verification

8074 strict ASan/UBSan cases cover2070 generator sequences and6004 seeded
duels, including reactor combinations, wounds, zero/one strengths, wrapping
states, rejected inputs and output/state alias controls. Genuine AROS68040/FPU
HUNK returned0; all355256 bytes equal the independent oracle, SHA256:
`539af31ba94d1b0814f33ef621fda51244834d503629526c216dcde14448c2eb`.

The connected seeded world probe additionally checks118 imported worlds
(105 accepted,13 explicitly rejected), current-world immutability, seed
preservation on failure and all field/sensor modifiers before round resolution.
Its native execution status is recorded separately in `COMBAT-CRT-EVIDENCE.json`.

```
python3 test_combat_crt.py --out /tmp/alpha-combat-crt
python3 test_world_field_combat.py --seeded --out /tmp/alpha-field-seeded
CXX=m68k-amigaos-g++ sh build_combat_crt_probe.sh /tmp/CombatCRT
```

The optional original-instruction check needs `unicorn==2.1.4` plus the private
hash-matching original demo executable; ordinary host/native controls require
neither. Native `CombatCRT` reads32-byte LE controls and writes44-byte results.
The connected probe CLI adds literal `seeded` after the rules/world paths and
uses an explicit test seed `0x12345678`, never a campaign default.

## Casualty Integration Still Required

Pinned Thinker `veh.cpp::veh_kill` (lines696–767), `move.cpp::veh_kill_lift`
(line485), `veh.cpp::battle_kill` (line799) are the available restoration-policy
sources. Removal requires faction active/lost counters, stack/tile removal,
actor-array compaction, every affected stack reference and boarded transport
vehicle reference remapped, plus runtime/UI actor references and combat aftermath.
It changes container record count and offsets. The existing fixed-count move/
production encoder cannot qualify such a mutation by merely dropping a record.
Persistent attack scheduling, dynamic container encoding, winner advance,
promotions/collateral/capture and replay authority must be connected before an
attack command is exposed. The source inventory does not claim those tasks done.
