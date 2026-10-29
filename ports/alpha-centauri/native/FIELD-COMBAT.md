# Conventional Field Combat

This adds a read-only conventional infantry engagement proposal and an exact
finite round resolver for its supported branch. It does not expose an attack
command or claim complete combat/gameplay.

The round/field policy is pinned Thinker default restoration policy, not a claim
that the missing original `battle_fight` body has been recovered. Source:
`induktio/thinker` commit `dabe62f2d1abba03de40f3dd3048494843f6a9af`,
`src/veh_combat.cpp` (MIT; existing `THINKER-LICENSE.txt`), SHA256
`c8bb7fe7a5a1a333dceb1f1f3e816ad7d982ae2c7d8a5edb42ac06b11491180d`.
In particular: `combat_rand` at line14, field `mod_battle_compute` at line619,
default reactor choice at lines1397–1406, round loop at lines1883–1916.

Recovered OpenSMACX morale and prototype/speed dependencies are reused through
`world_unit_stats`; its existing attribution and tests remain authoritative.
OpenSMACX pin `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c` has a
`battle_compute` marked Complete–testing with unfinished/suspicious branches,
but no recovered `battle_fight` implementation. It is not treated as complete
combat evidence.

## Round Resolver

`resolve_combat_rounds(CombatDuel, CombatRollStream, CombatResult)` consumes an
explicit ordered stream of 15-bit CRT random draws, with a caller-owned cursor.
It consumes the attack draw first, defense second. Each uses modulo strength.
A strength of zero or one produces roll zero and consumes no draw. Defender
wins ties. Every lost round adds **one damage point** in conventional combat
under default `ignore_reactor_power=false`; reactor `r` supplies `10*r` hitpoints.
This deliberately does not apply the mod's ignore-reactor option or psi policy.
Initial damage is respected. At most79 rounds can occur with supported reactors1–4.

All calculations use private scratch. A short/invalid stream, invalid duel or
overlap leaves the public cursor and result byte-for-byte unchanged, including
failure after the first draw of a round or after many completed private rounds.
This makes recorded draws deterministic and replayable without fabricating a
new original-engine RNG state. Original combat `game_rand` is a different
generator from the recovered `integer_random` used for movement. No saved map
seed, tick or movement seed is silently reused. A future turn runtime must
provide the actual combat generator/recorded stream and persist its authority.

## Current-World Field Proposal

`world_field_combat` derives final strength and current HP from decoded live
world records and qualified matching technology/component/basic-rule tables.
It does not read displayed/saved combat totals. `world_resolve_field_combat`
atomically composes those facts with the round resolver.

Supported branch:

- Base game, active human attacker, faction-owned human or AI defender, mutual
  vendetta without pact/treaty/truce, adjacent visible land tiles.
- One reciprocal-link-free unit on each tile; matching map occupancy flags.
- Conventional combat designs, plans0–3, infantry speed1, ordinary weapons and
  armor, reactors1–4, no special abilities or pending attacker orders.
- Effective morale (including current faction morale and home-base riots),
  faction offense/defense bonuses and fanatic attack, forest/rocky terrain,
  road attack only when both tiles have roads/tubes, uphill/downhill modifiers,
  hasty remaining movement and low human difficulty against AI.
- Elite morale can increase available movement, but the source retreat
  comparison skips elite morale. Both designs have identical infantry
  prototype speed, so defender retreat cannot occur in this branch.

Base tiles, bunkers, fungus, native units, sea/air/mobile units, noncombat units,
abilities, stacked units and unqualified attacker states reject explicitly.
Sensor and Geosync Survey Pod defense are now derived from the current world;
their presence elsewhere does not globally reject an unrelated engagement.
These are support boundaries, not claims that excluded original engagements
are illegal.

`world_combat_sensor_mask` implements pinned Thinker default
`sensor_defense_ocean=0`: original25-tile radius, wrap/polar bounds, raw sensor
priority over GSP on the same square, nearest **global** base by source vector
distance with the highest base index winning ties. Land sensor squares qualify
on neutral/defender territory. Ocean sensor squares qualify only when the
nearest global base belongs to the defender; the search does not skip a closer
foreign base to find a friendly one. GSP status uses that nearest base's built
facility41 and absolute wrapped x-offset0/2 and y-offset0/2. Ordinary sensor
bonus and additional GSP bonus each apply once, preserving sequential integer
rounding. The helper is read-only and output aliases reject before writes.

No world records, raw container, movement, diplomacy, RNG authority, kill/removal,
promotion, stack collateral damage, capture, base occupation, UI or persistence
are changed. The proposal is not an authority token and cannot be committed as
a complete attack. AI faction actors can be defenders; AI decisions are absent.

## Verification

`test_combat_rounds.py` builds the strict C++17 ASan/UBSan probe and compares
6605 deterministic cases with an independent Python model: all reactor pairs,
initial wounds, strengths0/1, ties, large strengths, cursor offsets, short/invalid
streams and atomic failures/alias controls. 5605 accept;1000 reject as expected.
The genuine AROS68040/FPU HUNK execution returned0 and all290620 output bytes
equal the independent oracle. SHA256:
`6af480b471ad59e791e900c53b183d434cba6a7bc76b3595fd3e00dfd09a9b25`.

`test_world_field_combat.py` builds a strict ASan/UBSan imported-world probe,
derives expectations independently from raw fixture fields, and checks105
qualified engagements plus13 rejected branches, including near/far sensors,
hostile territory, ocean nearest-base tie ownership, GSP and raw-sensor priority.
Every probe checks raw and
decoded world immutability and output/cursor alias rejection. World native
execution is recorded separately in the evidence receipt; no original Windows
executable or AmigaOS3.2.3 comparison is claimed.

`test_world_combat_sensors.py` separately checks12264 complete mask queries over
49 imported worlds against an independent raw-container model, including base
versions7/9 and SMAX12, cylindrical/flat and larger maps, zero/multiple bases,
nearest-base ties, mixed land/ocean and faction territories, edge coordinates,
GSP and raw-sensor priority. Its outputs are independent of caller percentages.

```
python3 test_combat_rounds.py --out /tmp/alpha-combat-rounds
python3 test_world_field_combat.py --out /tmp/alpha-field-combat
python3 test_world_combat_sensors.py --out /tmp/alpha-combat-sensors
CXX=m68k-amigaos-g++ sh build_combat_rounds_probe.sh /tmp/CombatRounds
CXX=m68k-amigaos-g++ sh build_world_field_combat_probe.sh /tmp/FieldCombat
CXX=m68k-amigaos-g++ sh build_world_combat_sensors_probe.sh /tmp/CombatSensors
```

The leaf HUNK reads352-byte LE control records from stdin and writes44-byte LE
results. The field HUNK CLI is `FieldCombat RULESFILE WORLDFILE`; it evaluates
vehicles0/1 using a fixed deterministic test draw stream and writes a complete
text receipt. Neither CLI runs the game campaign.

The separate `field_attack_snapshot` stage now composes these facts and seeded
rounds into a complete count-changing AFTER container for a qualified infantry
battle. Its scope, authority boundary and native proofs are in `FIELD-ATTACK.md`.
