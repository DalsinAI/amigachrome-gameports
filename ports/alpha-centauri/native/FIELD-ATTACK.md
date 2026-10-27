# Stackless Field Battle Transaction

`encode_field_attack_snapshot` produces an entire AFTER container with a real
casualty removed and a next combat-state ledger. It is a transaction for the
qualified **battle-fight phase**, not another displayed-strength proposal.
The caller's world and stage remain unchanged. A later owning engine installs
the resulting bytes and ledger together under its command/revision authority.
No LocalPlayer/UI attack command is exposed by this change.

The policy is explicitly pinned Thinker default restoration policy, MIT commit
`dabe62f2d1abba03de40f3dd3048494843f6a9af`; it is not a recovered complete
original combat routine. Source inventory:

| Source | Relevant behavior |
|---|---|
| `veh_combat.cpp::mod_battle_fight_2`, lines1433–1494 | Real road-rate attack movement charge; hasty strength already derived by field facts; both units marked moved, attacker attack-state and visibility |
| `veh_combat.cpp`, lines2090–2110 | Post-round promotion odds and additional combat draws |
| `veh_combat.cpp::promote`, line1187 | Raw morale increase, caps and optional second promotion draw |
| `veh_combat.cpp`, lines2138–2170/2300–2308 | Winner diplomatic combat counters, runtime faction wins/losses, monument event |
| `veh.cpp::battle_kill`, line799 | Loser units-lost counter and casualty dispatch |
| `veh.cpp::veh_kill`, lines696–767 | Active/combat counts,52-byte record compaction, stack and boarded-transport vehicle references, runtime/UI references |
| `move.cpp::veh_kill_lift`, line485, and `veh.cpp::veh_lift` | Unique casualty tile occupancy cleared; old owner nibble remains |
| `faction.cpp::act_of_aggression/treaty_on`, lines703/285 | Mutual vendetta status masks and exclusion-bit cleanup |
| `monument.cpp::mon_enemy_destroyed`, line249, `game.cpp::game_year`, line181 | First-kill achievement, source first-achiever scan and pinned default-policy calendar |
| `savegame.cpp`, lines402–410 | Saved UI word1 is MapWin vehicle ID, not an opaque coordinate |

The existing field facts and separate CRT generator retain their qualification
and original instruction checks in `FIELD-COMBAT.md` and `COMBAT-CRT.md`.

## Atomic Output

1. Derive current-world legal bounded field facts and resolve the actual seeded
   duel. `FieldAttackStage` supplies the **separate CRT state** and explicit
   runtime faction combat wins/losses; normal source containers do not serialize
   those runtime arrays. Movement RNG is preserved separately.
2. Qualify supported aftermath and counter/reference integrity. Compute source
   promotion checks/draws and any raw morale increase, preserving truncation.
3. Encode the current live world through the existing qualified facility/move/
   order snapshot union into private scratch. Unsupported unencoded live upkeep
   changes reject; the encoder never reloads an old raw baseline and drops them.
   A fully serialized/reloaded post-upkeep world naturally becomes a valid new
   baseline and its other fields remain opaque/preserved.
4. Apply damage, movement charge, moved/attack state, attacker visibility,
   prototype combat-faction bits, mutual aggression masks, faction active/lost/
   total-combat counts, winner diplomatic combat counters and explicit runtime
   win/loss ledger. Check loser active/combat totals against current actor records.
5. Remove exactly the loser52-byte vehicle record and decrement `VehCount`.
   Compact the complete following byte sequence, including all base, scenario,
   monument and UI sections. Repair greater stack indices, source sentry/board
   first-waypoint vehicle IDs, and saved selected vehicle index. A board reference
   equal to the casualty clears its order exactly as source; its now-ignored old
   waypoint bytes remain unchanged. Unrelated actor fields and opaque bytes stay
   intact, including previously made live movement/base orders.
6. Parse the entire smaller container into an owning `OriginalWorld` and validate
   all resulting stacks. Only then copy AFTER bytes and receipt to caller output.

Failures leave the output buffer, receipt, source world and input stage unchanged,
even for late unsupported snapshot changes, short capacity, signed overflow or
reference/counter corruption. Counter overflow/underflow and inconsistent saved
active/combat totals reject explicitly rather than reproducing corrupt counters.
This guard is a qualification boundary, not a claim of original corrupt-state
behavior. Separate inputs and outputs may not overlap world/rule/stage storage.

## Supported Aftermath And Remaining Boundaries

The field engagement restrictions remain: current base-game human attacker,
mutual vendetta, visible adjacent land, ordinary conventional infantry, one unit
per combat tile, no combat abilities, native/air/naval/mobile/base/bunker/fungus
branches or pending attacker orders. Sensors/GSP and human-versus-AI difficulty
modifiers are connected. Unrelated valid stacks and bounded vehicle references
elsewhere in the world are retained and remapped.

First-kill monument creation is implemented by `world_battle_monument`. It scans
all8 source event records, preserves highest-index first-achiever tie behavior
and source enemy fallback, patches only the appropriate enemy-destroyed event
and latest-event ID, and leaves alien/already-achieved cases unchanged. The year
uses **pinned Thinker default policy**: qualified `normal_starting_year` plus
current turn, with signed overflow rejection. OpenSMACX instead names an original
runtime `StartingMissionYear` global; that global is not silently inferred from
the saved displayed year. This policy distinction remains explicit. The receipt
carries the monument event for later owning UI presentation; no popup ran here.

Objective units, assistant-worm events, in-transport combat actors and third-party
terrain diplomacy reject. There is no native
credit income, artifact capture, collateral damage, base capture/elimination or
retreat branch in this qualified two-infantry engagement.

Promotions are implemented, including both source random decision points and
raw morale caps. `promotion_draws` is separate from duel draws; callers must use
the receipt's final CRT state. `promoted` reports a raw morale increment. Owning
presentation code can recompute effective morale before deciding a popup.

The source battle routine leaves a surviving attacker on its **source tile**.
Advancing into a vacated target belongs to outer movement-command continuation;
this stage does not invent an advance or charge a second move. The receipt gives
the removed old index and remapped surviving index. Owning runtime code must
repair any additional nonserialized UI/current-actor references, invalidate
support/AI caches, process presentation events and decide movement continuation.
`recompute_required`, `selection_remap_required` and `presentation_required` make
those obligations explicit; they do not falsely report that UI/AI ran.

This serializer grants no replay/turn/player authority by itself. Persisting and
installing the AFTER bytes **with** the next explicit ledger is an owning engine
operation. The original input world remains available until that atomic install.

## Validation

`test_field_attack.py` compares complete output containers against an independent
raw-file oracle. Qualified fixtures cover both outcomes, multiple casualty/actor
index orders, current live movement/base orders, promotion draws, faction counts,
stack-link and board-reference repairs, selected-vehicle remapping and every
opaque following section. First-ever, later, tied first-achiever, enemy fallback
and alien monument no-op cases compare whole output bytes. Rejections cover
calendar overflow, objectives,
third-party territory, short buffers, invalid references, unencoded live upkeep,
counter consistency and arithmetic overflow. Every probe checks immutable source
wire/decoded state, input ledger, output bounds and alias rejection.

The initial51-case host/native batch is in `FIELD-ATTACK-EVIDENCE.json`.
The expanded56-case first-kill follow-up and its native status are in
`BATTLE-MONUMENT-EVIDENCE.json`.
No full original Windows battle execution or AmigaOS3.2.3 qualification is claimed.

```
python3 test_field_attack.py --out /tmp/alpha-field-attack
CXX=m68k-amigaos-g++ sh build_field_attack_probe.sh /tmp/FieldAttack
```

Native probe CLI:
`FieldAttack RULES WORLD CRTSEED ATTACKER DEFENDER MODE OUTPUTFILE`.
Modes are controlled test cases; `live` injects scoped preexisting decoded move/
order changes and `unsealed` injects an unsupported decoded upkeep change. A failed
evaluation writes an empty test output file only after verifying the caller
buffer/receipt were untouched. Successful output is a full source-format
container, not a dead-flag-only fixed-size pseudo-save.
