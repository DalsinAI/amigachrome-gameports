# Human riot and Golden Age transitions

`drone_transition` implements the bounded human base-game restoration policy
for pinned Thinker `mod_base_drones` and `mod_drone_riot` (`base.cpp` around
2792/2865), MIT, commit `dabe62f2d1abba03de40f3dd3048494843f6a9af`.
It is not a recovered original `drone_riot` routine. Thinker deliberately removes
the revolt event, and this policy does not invent it. AI resets, alien artwork
and optional postponed-riot configuration are outside this human default policy.

The stage returns complete flag/physical-facility/notification proposals and
the exact resulting separate game CRT state. It never changes a world or opens
UI. The caller must execute it at the actual scheduler boundary: the pinned
upkeep calls production, then growth, then fresh stage1 base accounting, then
drones when `CurrentTurn > 0`. This module is the drones function itself, not
permission to reorder those stages or a complete turn scheduler.

## Transitions and notifications

Golden Age is exactly `drones == 0 && population > 2 &&
talents >= (population + 1) / 2`, matching `BASE::golden_age()`.

The only state flag changes are Golden Age `0x4` and active riots `0x2`.
All other bits survive. Golden Age start sets its flag and emits its start
notice. End clears it; the end notice is suppressed when drones exceed talents.
Riots end when drones do not exceed talents, or nerve stapling or Telepathic
Matrix suppresses them. The end notice is suppressed during an active Golden
Age. Managed-citizen notices use both governor bits `0x80000000` and `0x40`.

Actual unrest starts riots, sets the flag and returns without facility damage
or any CRT draw on that first turn. Managed first-turn unrest has its distinct
quiet-start notice; ongoing unrest always has the ordinary warning. Notification
order follows the source, including Golden Age end before riot end, and the
ongoing warning before a destroyed-facility notice.

The result marks resource recomputation for changed flags or damaged facilities,
and redraw for riot start/end or damage. These are engine obligations, not
claims that resources or UI have already been updated. A qualified riot stage
runs once; a requested resource recomputation must not recursively execute the
riot damage stage again in the same upkeep segment.

## Ongoing damage and separate CRT

The stage uses the already verified `combat_crt_draw` recurrence: unsigned
32-bit `state = state * 214013 + 2531011`, result `(state >> 16) & 32767`.
This is the same explicit game CRT stream used by combat, not a movement RNG,
map seed or tick seed. It is never initialized or inferred here. Draw count,
carry and unsigned wrap are preserved exactly.

Ongoing unrest evaluates gates in source order:

1. Police at most `-6` passes without a draw. Otherwise consume one draw and
   require `draw % (police + 7) == 0`.
2. Only then require no physically built headquarters, assimilation below 45,
   and more than one actual owned base. Consequently a headquarters base can
   still consume the preceding police draw.
3. Population below four consumes a draw and skips damage unless `draw % 3 == 0`.
4. Each attempt consumes a draw. An eligible, physically built, unprotected
   facility is removed and stops the search. Otherwise stop after exactly
   `population + difficulty + ranking + 9` unsuccessful attempts (the source
   increments `iter` and tests `iter > modifier`).

The bounded facts domain is population 0..127, valid fresh talent/drone counts,
police -100..100, assimilation 0..255, actual owned bases 1..512, difficulty
0..5 and ranking 0..7. The world query requires an existing population above
zero. Invalid facts and any overlapping input/result/state buffer reject
without advancing the CRT or touching the result.

## Explicit base-game ordinal adaptation

The pinned source uses SMAX's first project 70 and Sky Hydro Lab 65, drawing
`rand % 70` and considering facility ordinals 2..64. The qualified base-game
table has first project 39, Sky Hydro Lab 34 and Stockpile Energy 38.

This base-game restoration therefore draws `rand % 39` and considers only
ordinals 2..33. Headquarters 1, repeatable facilities 34 onward, Stockpile 38,
project locations and all unrelated built bits remain intact. This is an
explicit table adaptation, not an original-base-executable random-distribution
parity claim.

`RULE_FACILITY` (bonus 2) protects its facility unconditionally.
`RULE_FREEFAC` (bonus 12) protects it when that facility's prerequisite
technology is currently known. The facility loader's overridden free-tech
metadata is not substituted for that prerequisite.

## Fresh world composition

`world_drone_transition` accepts a qualified current-population citizen
assignment and the current psych allocation from that same assignment. It
checks centre/mask/count and active nibble encoding, then recomputes fresh
psych through `world_base_psych_policy`. Physical worker legality and economic
specialist availability remain prerequisites of the qualified worker/allocation
producer. Saved talent, drone, superdrone or psych display totals never supply
fresh counts.

The adapter derives flags, governor mode, actual assimilation/nerve state,
physical headquarters, live owned base count, pending police, ranking,
difficulty, Telepathic Matrix ownership, physical facilities and faction/tech
protection from the same world. The live owned count replaces a potentially
stale saved aggregate, corresponding to the current count required by the
source engine. It qualifies base rules with 72 slots, first repeatable 34,
first project 39 and 33 projects; expansion worlds are unsupported.

All world/rules/assignment sources stay unchanged. A late psych/technology/fact
failure leaves both output and CRT untouched. The result includes the complete
fresh psych facts and transition facts for inspection and qualification of the
future private transaction's flag/counter/facility writes.

## Verification

`test_drone_transition.py --out NEW_DIRECTORY` strictly compiles with
AddressSanitizer/UndefinedBehaviorSanitizer. LeakSanitizer is disabled because
the isolated runtime cannot inspect `/proc` threads.

The 10,914 independent integer cases cover count boundaries, exact Golden Age
rounding, notice ordering, first-turn immunity, all police branches, headquarters
and assimilation gates, low population, protected/excluded facilities, bounded
attempt exhaustion, repeated CRT carry/wrap, invalid inputs and partial aliases.
Invariant checks additionally seal every flag except `0x6`, permit at most one
eligible/unprotected built bit to clear, and forbid setting any facility bit.

The 55 world cases compare independently decoded wire/tech/faction facts and
fresh psych/transition outputs, including actual Boreholes Hive bases 9/10 at
low/high current-stage psych, poisoned saved drone/owner-count aggregates,
notice composition, conditional free-facility protection, expansion rejection
and late psych overflow. World, raw bytes, prototypes, actors, factions, tiles,
rules, proposed assignment, caller output and RNG are checked for unintended
changes. The optional `--original`/`--original-rules` inputs remain private.

`build_drone_transition_probe.sh OUTPUT` builds the same controls as a strict
68040/FPU HUNK. Native evidence records actual AROS68k/FS-UAE execution and
complete output comparison separately; it does not qualify AmigaOS3.2 or turns.
