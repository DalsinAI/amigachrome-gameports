# Alpha Centauri native rules-reader prototype

A bounded first implementation for separate Alpha Centauri and Alien Crossfire
ports. It reads **TECHNOLOGY, RULES, WORLDBUILDER, SOCIO and BONUSNAMES**. Technology has 89 records, case-insensitive
keys, None/Disable sentinels, priorities, binary flags and resolved forward
prerequisites. It is not a game engine or launcher.

Derived from Brendan Casey's [OpenSMACX](https://github.com/b-casey/OpenSMACX),
revision `2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`:
`text.cpp` token interface, `alpha.cpp::read_tech` two-pass key resolution and
`alpha.cpp::read_basic_rules` field order and bounds.
GPL-3.0-or-later; original licence retained in LICENSE. This is an adaptation,
not an unmodified upstream build. The caller owns the input and Rules state.
There are no Windows headers, fixed game addresses, original DLL calls, packed
native-memory records, dynamic allocation in the parser or rendering dependencies.

The reader accepts CRLF/LF/CR, an unterminated final line, blank/comment lines,
space/tab padding, original positive 0b/0x/0d number prefixes and signed decimal
values. It rejects missing/duplicate/oversized keys, unknown prerequisites,
overflow, truncated/extra rows and long or NUL-containing lines. It intentionally
rejects malformed numeric suffixes rather than copying atoi's permissive behavior.
No filename precedence/scenario resolver, string table or section cache is present.
An unsuccessful read leaves partial output; use state only after Error::ok.

## Controls

```sh
python3 ports/alpha-centauri/native/test_rules.py --out /tmp/alpha-host
python3 ports/alpha-centauri/native/test_rules.py --out /tmp/alpha-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

The host runs ASan/UBSan (leak checking disabled for sandbox compatibility).
The Linux cross control runs generated big-endian 68040 code under qemu-m68k.
Both use generated synthetic data; no original assets are fetched or included.
An independent Python reader fingerprints every name/key, priority, prerequisite
and flag with explicit byte order, so matching counts alone cannot pass.
Use --data /path/to/alpha.txt --data /path/to/alphax.txt for owner-provided data.

Private 25 October controls using the owner's CD files find 89 records in each,
77/86 active records and 136/152 prerequisite edges. The Amiga GCC 16.2.0-open3
build additionally passes a raw 68040/QEMU control against fingerprints of both
actual files. That raw control checks the parser's generated m68k instructions,
not AmigaOS startup. The actual AmigaOS command-line probe also passes both files in the existing
isolated lab (RC 0), and rejects a duplicate-key file with RC 20. The lab was
stopped; its ROM/module/startup were unchanged. No live-instance execution or
full-game qualification is claimed. The CI Linux ABI is not an AmigaOS ABI qualification.

The remaining rules sections, faction reader, gameplay algorithms, original-game differential
checks, UI/audio, saves and launchable game separation remain future work.
Neither GPL source licensing nor this prototype conveys rights to game assets.

## Basic and world-generation parameters

`read_basic_rules` takes an explicit `Variant::base` or `Variant::alien_crossfire`
and an already parsed technology table from the same input. The base game's
RULES section ends two rows earlier: its subspace-generator fields are zero and
must not be used. Choosing the wrong variant fails instead of consuming a
section header or ignoring expansion-only rows. Both outputs require Error::ok.

The 77 named basic fields (including two expansion-only fields) and 31 named
world parameters use explicit signed 32-bit storage. They are not packed game
memory or a save format. Input numeric values clamp to OpenSMACX's bounds;
technology references resolve to table indices, None (-1) or Disable (-2).
Unknown references, missing/extra rows, missing grouped values and trailing
fields fail. The two `.inc` lists expose the reviewed file order, bounds and
row grouping; the parser and diagnostic dump use those lists.

This follows **OpenSMACX's patched behavior**, including retool strictness up to
3 and Boolean minima of 0. It does not prove those choices identical to every
original executable revision. Base-file support is a native adaptation because
the pinned upstream reader targets the expansion process.

```sh
python3 ports/alpha-centauri/native/test_basic.py --out /tmp/basic-host
python3 ports/alpha-centauri/native/test_basic.py --out /tmp/basic-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

The synthetic fixture is asset-free and records expected named fields separately
from the implementation. Controls check all fields, every truncated row boundary,
short groups, wrong variants, malformed/overflowing numbers, clamping, unknown
keys and input bounds. The diagnostic CLI `basic-probe base|expansion FILE`
prints every named field so local original files can be compared to a separate
reference. World parameters are only inputs: **no world generator, faction
behavior, combat, production or turn processing runs yet**.

## Social and faction-bonus lookups

`read_social_rules` loads eleven effect keys/labels, four social categories and
four models per category, with a prerequisite and eleven signed effect values
per model. The separate BONUSNAMES table has 29 keys for the base game or 41
for Alien Crossfire. Its index defines the bonus kind; custom labels retain
that order. Case-insensitive lookup functions return an index or -1.

Model modifiers preserve upstream sign counting (`++` means 2; `+-` means 0),
last repeated effect wins, and a key without signs means zero. Faction SOCIAL
bonuses use a different zero rule and are not applied here. Unknown effect
names now fail instead of being silently discarded. Duplicate effect keys,
category names, names within one category and bonus keys fail to prevent
ambiguous lookups; translated effect labels need not be unique. Names have
explicit bounded storage, shown in the public structs.

One terminal comma is accepted, as used by the original files. Extra empty
columns cannot hide subsequent fields: the shared token-tail check now checks
the remaining input directly. Technology/basic regression controls cover this
case too. All readers require successful prior technology parsing from the
same file; the caller must discard partial outputs on any error.

```sh
python3 ports/alpha-centauri/native/test_social.py --out /tmp/social-host
python3 ports/alpha-centauri/native/test_social.py --out /tmp/social-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Optional local original files: `--data base=/path/alpha.txt` and
`--data expansion=/path/alphax.txt`. `social-probe base|expansion FILE` dumps
all named strings and values. The independent Python reference compares every
one, rather than just counts. The synthetic controls are asset-free.

These tables enable faction parsing next. They do not apply faction bonuses,
choose social policies, run AI or qualify a playable game. The localized
four-character ideology matching and faction data-file fallback rules are
also outside this reader's scope.


## Eight-row faction definitions

`read_faction` takes caller-supplied bytes, a requested section and a filename
section fallback, plus technology and social tables qualified together. It
reads the first eight data rows: names/genders and five AI preferences, bonus
pairs, two ideology records, adjective/abbreviation, assistants, leader/faction
adjectives and leader insult. Further narrative, city names and art are not
consumed. The ignored upstream abbreviation is retained as owned text.

Defaults follow `clear_faction`: numeric rules zero, hurry/technology cost100,
flags zero and no slot bonuses. The original nineteen slot-bonus IDs and flag
bits are preserved as explicit native values; these are not packed save records.
BONUSNAMES position determines semantics, including translated labels. Named
TECH resolves against the technology table; a nonzero strict numeric value
sets selectable-tech count. SOCIAL/ROBUST/IMMUNITY signs sum, with zero becoming
one (different from SOCIO). IMPUNITY/PENALTY resolve category and model. At most
eight slot bonuses are allowed; surplus bonuses fail instead of disappearing.
Flag parameters remain present but their value is ignored, as upstream does.

Ideologies use full ASCII case-insensitive matching by default. Explicit
`localized_first_four` matches the first four bytes as pinned upstream does;
ambiguous matches fail. `nil`/`None` indicate no ideology/effect. Unknown names
fail rather than silently choosing model zero or dropping modifiers. This uses
**OpenSMACX's corrected AI-emphasis field**, not the original executable's -1
bug. It does not prove original-AI equivalence. Character data remains bytes;
no Unicode or locale-dependent folding is introduced. Overlong fields and
malformed/overflowing numbers fail; no silent truncation/atoi-prefix behavior.
All original files have a pair-bearing bonus row; a missing row is an error.

```sh
python3 ports/alpha-centauri/native/test_faction.py --out /tmp/faction-host
python3 ports/alpha-centauri/native/test_faction.py --out /tmp/faction-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Optional local original files: `--owner-dir /path/to/private-data`, containing
alpha/alphax and the fourteen faction text files. The control compares every
field in seven base and fourteen expansion faction/rules combinations with
an independent Python reader. `faction-probe base|expansion RULES FACTION SEARCH
FILENAME [localized]` is the native diagnostic. Synthetic controls cover all41
bonus kinds, defaults, eight-slot boundary, malformed input, fallback priority,
character bytes and localized matching. No original assets are included in CI.

Loading faction definitions does not apply them to bases, units or a running
turn. Starting-game state, AI, original save compatibility, UI/audio and actual
playable gameplay remain separate qualification gates.


## First owned-state calculations

`calculate_social` adapts pinned OpenSMACX compute_faction_modifiers/social_calc
into explicit qualified rule/faction inputs, selected policies and project/
facility facts. It computes eleven effects, including impunity/penalty, project
penalty removal, faction base modifiers, immunity and robust. Quick mode skips
base/project additions and immunity/robust, but still removes relevant project
penalties. Full mode preserves original bonus ordering and signed division.
Alien temple research requires the expansion flag as well as the alien flag.

`calculate_nutrients` adapts one base_nutrient calculation: growth modifiers,
convoys, citizen consumption, surplus, starvation/recovery markers and upkeep
stage-one faction totals. It does not advance population, accumulate a turn's
food or choose worked tiles. Supply the current pre-convoy intake/totals each
time; the helper does not reset those upstream stages for the caller.

Both use owned state, no original global addresses or callbacks. Invalid state
or a signed overflow returns an error without modifying the supplied output.
Checked wider intermediates reject arithmetic outside the original signed32
valid domain instead of inheriting undefined overflow from malformed mod data.
Population is bounded to the original signed-byte domain (0..127), consumption
rules to the parsed 0..100 range, and convoy/stage inputs must be nonnegative.

```sh
python3 ports/alpha-centauri/native/test_logic.py --out /tmp/logic-host
python3 ports/alpha-centauri/native/test_logic.py --out /tmp/logic-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Controls compare all outputs with vendored GPL test-only pinned function bodies
in upstream_logic_reference.h. Its explicit platform/layout adapters replace
Windows types, global addresses, project/facility queries and unsafe pointer
arithmetic across struct members; branch/arithmetic order remains the source's.
An independent Python digest also checks 16,384 social and 2,001 nutrient cases.
Thirteen invalid/overflow controls verify unchanged output. No original assets
are needed by CI. Source-function equivalence is not differential verification
against the original executable, nor proof of an integrated or playable turn.


## Components and starting-unit blueprints

`read_unit_definitions` loads CHASSIS, REACTORS, WEAPONS, DEFENSES, ABILITIES
and the declared UNITS rows. The explicit base/expansion variants have9/9
chassis,4/4 reactors,21/26 weapons,10/14 armour and24/29 abilities. UNITS may
declare0..64 rows; the stock tables declare14/23. Component and technology
references resolve against owned state; ambiguous or unknown keys fail.
Chassis noun names/gender/plurality remain available alongside numeric fields.

Storage is native explicit-width state, not packed save structures. Values
must fit their original byte/word domains instead of silently wrapping casts;
triads are0..2, missile flags0..1. Empty ability abbreviations/descriptions and
terminal commas are allowed. A unit must reference actual components, rather
than passing None/Disable indices into a prototype constructor. Ability masks
remain unsigned32 values; interpreting those bits belongs to derived stats.

The pinned OpenSMACX reactor-power enhancement is retained. Its optional last
UNITS reactor column defaults to1, except prototypes17/18 (Ogre Mk2/Mk3) default
to2/3. Explicit0 also selects that default. Raw plan -1 and cost/carry0 overrides
remain raw: **this does not implement make_proto or auto-calculated statistics**.
The next step must derive those before treating a blueprint as a live unit.

```sh
python3 ports/alpha-centauri/native/test_units.py --out /tmp/unit-host
python3 ports/alpha-centauri/native/test_units.py --out /tmp/unit-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Optional local original files: `--owner-dir /path/to/private-data` containing
alpha.txt/alphax.txt. `unit-probe base|expansion FILE` dumps all component and
blueprint fields:750 for the stock base file and938 for expansion. Python
compares every field independently. Synthetic cases include all truncated
row boundaries, duplicate/unknown keys, byte bounds, unsigned masks, optional
columns and reactor defaults, including mod-sized64-row tables. Original
assets remain outside the repository and CI.


## Derived predefined prototypes

`derive_predefined_prototype` takes a qualified component/blueprint set, a
predefined ID0..63 and road rate1..100. It applies pinned make_proto's basic-ID
path, proto_cost, transport_val, blueprint overrides and speed_proto. It returns
owned initial cost/capacity, plan, original role code, flags and road-point speed
with identity/components preserved. The role code remains named as such because
its full AI semantics are not established. Fungal Tower ID19 is immobile.

Custom designer IDs>=64, prior-prototype history, faction-granted abilities,
project bonuses, damage and morale are outside this API. The predefined path
has faction0 for the upstream Cloudbase project query; it is not an arbitrary
faction's live-unit speed. Disabled prerequisites are retained; constructing a
stored prototype does not prove that a faction can build it.

`calculate_prototype_cost` is the separate pure pricing operation. Ability bits
must exist in the selected variant's table, reactor IDs must be1..4. Invalid
state, signed arithmetic overflow or a derived storage value outside its byte
range fails without changing output, rather than inheriting wrap/truncation
from invalid mod values. Explicit blueprint cost/carry overrides apply after
automatic calculation as in read_units. Predefined plan overrides do not
recalculate the constructor's role code, matching the pinned source.

```sh
python3 ports/alpha-centauri/native/test_prototype.py --out /tmp/prototype-host
python3 ports/alpha-centauri/native/test_prototype.py --out /tmp/prototype-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Asset-free controls compare2,304 price cases and514 derived cases with pinned
source bodies in upstream_prototype_reference.h and an independent Python
checksum. Source adapters use owned globals and reject unexpected faction/
history callbacks; these are not production callbacks. Six invalid/overflow
controls verify rejection. `prototype-probe base|expansion FILE` loads original
rules and prints all initial predefined stats; those originals remain private.
No movement, combat, complete turn, designer support or gameplay is implied.


## Map geometry and adjacent-step prices

`MovementMap` borrows a caller-owned native tile array and explicit geometry;
it is not an original map/save decoder. Width is even, cells have parity x+y
even, and indexing is `(x+width*y)/2`. Bounds use the original nonnegative
signed16 coordinate domain. `movement_wrap_x` preserves one source xrange
step, including far-out values still outside the map afterward; latitude never
wraps. `movement_distance_x` handles flat and wrapped valid coordinates.
Terrain altitude/rockiness bytes are already masked original encodings; mixed
rock bits fail rather than being mistaken for a valid rocky tile.

`adjacent_movement_cost` adapts pinned hex_cost for qualified predefined units
and valid adjacent endpoints. It preserves sea-shelf fungus/native exceptions,
roads/base tiles, zero-point magtubes, neutral/native/Xeno fungus movement,
diagonal river shortcuts (including the seam), hover/antigrav, rough/forest
penalties and plan/planet-dependent full-speed fungus costs. Prototype stats
are derived for the same road rate. Context explicitly supplies project and
planet facts, replacing original global callbacks.

This is **pricing**, not terrain passability, occupied-tile handling, combat,
ZOC, movement expenditure or turn dispatch. For example an immobile tower can
have a priced endpoint while a move dispatcher must still reject moving it.
Only predefined IDs0..63 and factions0..7 are accepted. All outputs remain
unchanged after invalid input. The caller must still check action legality.

```sh
python3 ports/alpha-centauri/native/test_movement.py --out /tmp/movement-host
python3 ports/alpha-centauri/native/test_movement.py --out /tmp/movement-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
```

Controls compare24,576 generated prices with the pinned hex_cost test adapter
and an independent Python digest. Twenty named cases check specific branch
values, including wrapped/vertical/horizontal rivers, neutral magtubes,
negative planet, fungus exemptions and ignore-rough. Another1,200 geometry
cases include the maximal coordinate domain;14 invalid cases check rejection.
Source test callbacks use owned objects and mark unexpected requests invalid.
No original assets are needed by CI and no gameplay is claimed by this helper.

### Offline movement expenditure

`resolve_move_budget` owns the integer RNG and expenditure stage of Thinker's
`order_veh` (pinned dabe62f2; MIT notice retained). It preserves over-budget moves,
fractional failure spending, and retry allowance. It does not relocate units,
check access or occupancy, execute full `veh_skip` side effects, or run turns.
Inputs must describe an already legal move and effective vehicle speed. Failed
fractional checks mutate even non-executing requests, as in the source: a UI
preview must use a copy. Multiplayer RNG and tutorial exits are outside scope.
Byte storage overflow is rejected rather than wrapped. No floating RNG is used.

`test_move_budget.py --out NEW_DIR` checks 331,776 budget cases, 30 full-width
integer RNG cases and five invalid-state cases against an independent Python
model. CI runs host sanitizers and big-endian 68040 controls. Native qualification
is recorded in `MOVE-BUDGET-EVIDENCE-20261025.json`. These controls establish the
budget stage, not original-executable fidelity or an integrated game.

### Interactive native movement slice

`exploration_window.cpp` opens a Workbench window using intuition/graphics. Run
`alpha-movement base alpha.txt` or `alpha-movement expansion alphax.txt` with your
own original rules file. Both parse qualified technology/basic/unit definitions.
Click an adjacent land cell or use 7/8/9, 4/6, 1/2/3 for eight directions. N starts
the next **movement-only** turn; Q or the window close gadget exits normally.
Road, magtube, fungus, forest, rough and sea marks are placeholders, not game art.

The scenario is deliberately empty: an undamaged predefined land scout, no
faction speed/project bonuses, enemies, bases, event tiles or ownership effects.
Unsupported tile bits fail rather than silently skip events. The dispatcher uses
qualified coordinate/price/budget modules, rejects ocean access, relocates on
success and preserves fractional failure and exhaustion. N resets spent movement
as in repair_phase; it does not claim to run repair or economic upkeep.

`test_exploration.py --out NEW_DIR` exercises road exhaustion, cylindrical wrap,
ocean blocking, rough overspend, 128 fractional outcomes and invalid/overflow
transactionality. CI covers this core on host sanitizers and big-endian 68040.
The frontend additionally compiled with the normal private SDK and passed actual
AmigaOS keyboard/mouse/exit checks with both original rule variants and a private
current Instance-32 ROM pair, continuous active display polling, followed by lab
stop and config/ROM/module/startup restoration. See
`EXPLORATION-EVIDENCE-20261025.json`. UI evidence is bounded to that native build;
the host CI does not compile the Amiga window. This is an interactive movement
slice, not a complete game: bases, economy, AI, combat, savegames and campaign
state are absent. No original assets are distributed.

### Early colony gameplay slice (supersedes the movement-only window)

Build with `CXX=/qualified/sdk/bin/m68k-amigaos-g++ ./build_colony.sh OUTPUT`.
The compiler supplies the normal NDK and libnix; no installed SDK overlay, ROM
change or replacement library is required. Run `alpha-colony base alpha.txt` or
`alpha-colony expansion alphax.txt`, using your own original rules file. These
are separate launch modes of this small scenario, not two completed ports.

The hand-authored map starts with a neutral human colony and a scout. Move the
scout with the existing keys/clicks. W toggles worker mode: click a land cell in
the twenty-cell base radius to assign/remove a citizen. N advances the colony's
food/growth phase and restores scout movement. A new citizen begins unassigned;
assign it explicitly. Watch food intake, consumption, surplus, stored food and
population. Worker choices can grow or starve the colony. Q closes normally.

`RESOURCEINFO` reads nine positional rows/four signed values. The source ignores
trailing annotations, and its unused improved-land mineral/energy `*` cells
become zero. Other malformed numeric cells fail, missing/extra rows fail, and
failed parsing leaves output unchanged. No original rule text is distributed.

`colony.cpp` owns human nutrient/mineral cost factors, plain/base/forest land
yields, and population advancement. It explicitly preserves Thinker's minimum
cost of one for tiny modded multipliers, rather than OpenSMACX's possible zero.
Growth is split at `base_compute`: the caller recomputes **both** the food store's
starvation marker and the surplus at the new population before finishing. Boom
clamping happens after that recomputation. It does not reuse the old population's
surplus. `colony_scenario.cpp` connects those functions with the qualified nutrient
calculation and a final post-growth recomputation as in base upkeep. Starvation
trims excess work slots highest-slot first as a stated **scenario allocation
policy**, not a reconstruction of the original governor.

The scene has no projects, facilities beyond HQ, resource bonuses, landmarks,
fungus work, technology unlocks, improvements beyond inert roads/magtubes, events,
foreign ownership, support, riots or specialist output. Mineral/energy **yields**
are displayed; they are not falsely credited as production/research. Population
limits use the selected rules. Existing owned base indices are passed explicitly
to movement; unknown base bits still fail. There is no base founding, production,
research, combat, AI, savegame, world generation or full campaign yet. This is a
playable food/worker/scout slice with placeholder geometry, not a finished game.

Validation in `COLONY-EVIDENCE-20261025.json`: 34 resource parsing cases; 12,348
cost, 6,912 yield and 23,040 population controls against independent Python;
overflow/invalid-state checks; a 31-turn scenario covering growth, starvation and
colony removal. Generated OpenAmigaGCC 68040 code also checks all 72 resource
fields from the owner's two original rule files against independent decoding.
Actual AmigaOS gameplay passed with **both** variants: scout movement/exhaustion,
mouse worker allocation, population 1 to 3, then starvation back to 1 across 29
turns, and RC0 exit. Display polling remained active; a private then-current
Instance-32 ROM pair was used, followed by stop and verified lab restoration.
Host CI covers portable controls on sanitizers and big-endian 68040. The Amiga
window is additionally qualified by that recorded native build/run, not compiled
by host CI. No original-executable differential fidelity is claimed.

### Original Planet map import

`original_map.cpp/h` decodes the original `TERRANMAP` version5 `.MP` files using
the reconstructed Thinker `map_read/map_init` layout. This replaces neither a
full game loader nor the test scene yet. The original map view borrows immutable
input bytes: keep that buffer alive and unchanged. Tiles are decoded field by
field with explicit little-endian reads; use them to initialize owned world
state. Never cast the bytes to native packed records or import serialized PC
pointers. Unknown metadata/tile bits remain available in the bounded raw spans.

Width must be even2..4096 and height1..4096. Counts and abstract dimensions derive
from those dimensions exactly as in source; stored counts cannot drive memory
allocation. An inconsistency is flagged. File length must match exactly. Version5
is supported explicitly; other versions fail. Landmark count is bounded64. All32
label bytes are retained, with an added terminator in native state. Raw off-map
coordinates are retained and flagged, not used as array indices: two such labels
occur in each supplied large Planet map. The remaining original
`fixup_landmarks` binding is **not executed or claimed reconstructed**. No editor
field clearing, unit/base creation, gameplay initialization, original SAV/SC
state loading or original-executable differential fidelity is claimed by this
file reader.

Build the CLI probe using the qualified SDK with `original_map.cpp` and
`original_map_probe.cpp`; run `map-probe ORIGINAL.MP`. Host/independent big-endian
controls: `test_original_map.py --out NEW_DIR` (or `--cc m68k-linux-gnu-g++
--m68k-linux`). They cover66 cases: every decoded tile field, signed owner,
landmark bytes and flags, metadata/world fields, recomputed counts, inert pointer
bytes, edge geometry, malformed/truncated/trailing data and unchanged outputs on
failure. Independent Python interprets the byte format directly.

Native qualification is recorded in `ORIGINAL-MAP-EVIDENCE-20261025.json` after
all four original CD maps and the synthetic corpus pass. The normal SDK's large
structure copy uses real Exec CopyMem, so the standalone raw Linux syscall
harness is not used or replaced with stubs. Actual AmigaOS tests run the final
OpenAmigaGCC16.2.0-open3 binary, plus file probes of all original maps, against
independent expected fields/fingerprints. Original media remains private and is
not distributed with the source.

### Original tutorial/scenario and saved-game decoding

`original_game.cpp/h` reads the original plaintext `TERRAN` outer version0x56,
with data versions7..12. The recovered `game_data` layout includes legacy/current
preferences, current-player selection, faction blocks, the embedded map, replay,
continents, 512 prototypes, counted vehicles/bases, scenario/objective/monument
blocks, version12 tectonic counters, the saved UI state and the always-written
tick-seed footer. The footer is preserved; it is **not automatically the gameplay
RNG seed in ordinary saves**. Encrypted, extra network state and `TERRAE` variants
are explicitly unsupported. Other versions fail; trailing bytes do not silently
become accepted state. Counts and spans are bounded before access.

The native record tables derive from the pinned MIT source. Each field is decoded
explicitly, including signed8/16/32-bit values. Serialized PC data is never cast
to native structures. Legacy preferences extend 33 project entries to64 with-1;
legacy bases extend8 facility bytes to12 with0, as in source. Strings preserve
all wire bytes and gain a terminator. `original_game_wire.json` records the
source-derived wire schema used by independent Python interpretation. This is a
**format decoder**, not semantic validation of every game reference: opaque
faction/other spans require later decoding, and actor coordinates/IDs require
validation before constructing the owned world.

Raw gameplay IDs remain tagged with their source version. In particular the
old weapon>=12 to SMAX+3 and old production<=-34 to SMAX-31 migrations are not
implicitly applied to base-game rules with different component numbering.
World initialization must select a consistent base/expansion ID space. The
source's post-load prototype icon corrections, landmark fixup, UI resets and
other engine callbacks are not pretended to execute. The borrowed input buffer
must remain alive and unchanged. All failed reads/accessors leave outputs
unchanged. `read_original_map_block` additionally returns its bounded consumed
length for embedded maps, preserving both outputs on failure; the standalone
`.MP` reader still requires exact EOF.

Build the native diagnostic with `CXX=.../m68k-amigaos-g++ sh build_game_probe.sh
OUTPUT`, then run `OUTPUT ORIGINAL.SC` or `OUTPUT ORIGINAL.SAV`. Portable controls:
`python3 test_original_game.py --out NEW_DIR` and the independent big-endian
`--cc m68k-linux-gnu-g++ --m68k-linux` path. The90 synthetic controls include all
six supported versions, scenario/no-scenario, empty/max actor counts, expanded
headers with older data, maximum replay, malformed headers/versions/counts/maps,
block truncations, trailing bytes, signed widths, full strings and unchanged
outputs. Controls compare all decoded preferences, actor/prototype/base fields,
map tiles, opaque spans and UI fields against independent byte interpretation.

`ORIGINAL-GAME-EVIDENCE-20261025.json` records the normal SDK AmigaOS build/run:
all90 synthetic cases and the owner's original Explore, Build and Conquer
scenarios passed, including every decoded actor field/fingerprint. The existing
map66-case regression also passes after the embedded-map refactor. No original
assets are distributed. Loading their data does not yet execute the restored
tutorials, original turn loop, AI or combat. This work is part of restoring the
original games; the neutral test scene remains only a test bed.

### Saved faction state and faction definitions

`original_faction.cpp/h` decodes all eight saved `Faction` and `MFaction` records
from a validated original-game view. These restore the existing energy credits,
research progress, social settings, diplomatic flags/loans, queues, unit/region
accounting, saved AI goals/sites and faction-specific rules/names. They do not
execute research, diplomacy, AI or a turn. The complete209 state declarations
and55 definition declarations derive from pinned Thinker MIT layouts; reserved
and Thinker-labelled original bytes remain inert data. Gameplay reference
validation belongs to world initialization, not this format reader.

All signed8/16/32-bit fields use explicit endian decoding. Numeric and text
matrices retain their shape; every text row preserves its wire bytes plus an
added terminator. Goals decode their five fields explicitly, not through native
packing. Legacy data7..10 extends facility-announcement bytes8 to12 and project
intelligence words5 to8 with0, matching `convert_faction_v10_to_current`.
`original_faction_wire.json` separately records the current and original legacy
source layouts: independent Python uses the actual legacy offsets rather than
the native decoder's offset-delta implementation. Raw technology/production IDs
remain version tagged. No PC pointers or original engine callbacks are used.

`test_original_faction.py --out NEW_DIR` compares every field in all eight
records using fingerprints, tests full-length strings, matrix rows, goal signed
widths, legacy extensions and unchanged outputs on invalid indices/short spans.
The102 synthetic cases include the90 original container controls plus targeted
zero/all-ones/alternating faction blocks across versions7,10,11,12. Host
ASan/UBSan and independent big-endian68040 CI run these controls. Build the
actual AmigaOS probe with `CXX=.../m68k-amigaos-g++ sh build_faction_probe.sh
OUTPUT`. `ORIGINAL-FACTION-EVIDENCE-20261025.json` records all105 native cases,
including the three original tutorials. The lab is stopped and its original
configuration, ROM pair, module and startup verified restored. Assets stay
private. A decoded saved AI plan is not a working AI or restored full game.

### Owned original world

`OriginalWorld` in `original_world.cpp/h` owns a copy of the original complete
container and native decoded tiles, all512 prototypes, counted vehicles/bases,
and all8 faction-state/definition records. Opaque blocks remain in that owned
snapshot for later interpretation; they are not replaced by invented defaults.
None of its views depend on the caller's buffer. Original data versions and raw
ID spaces remain unchanged, including faction-specific prototypes such as320 in
the original Explore tutorial. This is a read-only import foundation, not a
working turn loop or a new game. No raw native structures are serialized.

Import checks coordinate parity/ranges, vehicle faction/prototype/home/stack
index bounds and base faction/queue bounds before the world becomes available.
These are **partial reference checks**, not complete gameplay legality, stack
reciprocity/cycle validation or rule-component/version mapping. Later actions
must validate their actual dependencies. Unknown state is preserved without
pretending to execute it. Runtime RNG seed is supplied explicitly at world
level; an ordinary save's tick/map seed is not silently substituted for runtime
random state. No independent per-unit seeds are initialized.

A64MiB default per-world budget is checked with64-bit arithmetic before any
allocation. It counts the original snapshot, all native arrays and object size.
It does not claim to cover peak memory while an old world and caller input
coexist during replacement. Platform allocation failure remains possible and
returns an error. Records get explicit C++ object lifetimes in normal allocated
storage. RAII frees partial allocations. A failed load preserves the previous
world; a successful self-reload copies input before releasing its predecessor.
The interface is noncopyable and cleared safely more than once.

`test_original_world.py --out NEW_DIR` covers64 synthetic import/lifetime,
resource-budget, malformed/truncated and actor-reference cases on host ASan/UBSan
and independent big-endian68040 CI. Successful cases poison and free caller
memory, compare every owned record field to the qualified snapshot decoder,
check failed reloads preserve state/seed, and reload from their own snapshot.
The actual normal-SDK AmigaOS binary additionally passes all3 original tutorials,
including all custom prototype/faction records. Build with
`CXX=.../m68k-amigaos-g++ sh build_world_probe.sh OUTPUT`; evidence is in
`ORIGINAL-WORLD-EVIDENCE-20261025.json`. The lab is stopped/restored; no live
instance or original user data was changed. Full restored gameplay remains work
in progress: importing original state alone does not execute it.

### Saved and faction-specific unit speed

`saved_speed.cpp/h` restores the recovered `speed_proto/speed` arithmetic for
all512 saved design slots. It uses the actual saved component IDs/abilities,
including unnamed, flags0 faction-specific designs. It does not replace them
with the predefined unit table or require that a design has a stored name.
Air/transport/slow/antigravity/fuel/air-superiority ordering, project bonuses,
elite eligibility, damage rounding, artifact hitpoints and sea minimum speed
follow the pinned OpenSMACX functions.

The dependencies are explicit: Cloudbase ownership belongs to the design's
faction (`id/64`), Maritime ownership to the vehicle's faction, and effective
morale is the resolved `morale_veh(veh,true,0)` result. It is **not simply the raw
saved morale byte**. This API does not invoke an unported morale/project callback
or claim these dependencies are already bound into the world. Road rate and
qualified base/expansion component tables are required. The byte-capacity guard
rejects movement speeds above255 rather than wrapping; no mod-specific maximum
is applied implicitly. The Fungal Tower exception is enabled for expansion
rules; the documented base-profile adapter disables it. Failure leaves outputs
unchanged. Speed is not passability, expenditure or an action/turn dispatcher.

`upstream_saved_speed_reference.h` contains the two source function bodies with
explicit test adapters for global state and callbacks. Unexpected callbacks are
errors. It runs portable recovered C++ logic, not residual original x86 code.
`test_saved_speed.py --out NEW_DIR` checks1,492,992 source-reference cases,
including242,586 storage-overflow rejections,12 malformed-input controls and1024
independent Python vectors. Host sanitizers and independent big-endian68040 CI
run the controls. The diagnostic accepts `base|expansion RULES ORIGINAL.SC` and
checks controlled dependencies on the saved designs. It rejects old-base game
IDs mixed with SMAX component tables, rather than silently misinterpreting them.

Native normal-SDK recipes are `build_saved_speed_control.sh OUTPUT` and
`build_saved_speed_probe.sh OUTPUT`. `SAVED-SPEED-EVIDENCE-20261025.json` records
the actual AmigaOS control sweep, all20 vehicles from the three original base
tutorials, and a synthetic expansion container using the owner's original
alphax tables:384 controlled original/synthetic design contexts match source
reference and independent Python. The synthetic expansion is not an original
SMAX campaign/save. Effective morale/project ownership remains supplied test
context in those probes. The lab is stopped/restored; assets remain private.
These calculations support restoration but do not yet let the original games
execute full turns, AI, combat or saves. No original-executable differential
fidelity is claimed.

### Unit dependencies bound to the original world

`world_unit_stats.cpp/h` binds saved-unit speed to the actual owned original
world. It restores `morale_veh(..., versusNative=0)`, including ordinary faction
social/rule morale, negative Creche mitigation, Brood Pit ordering, immunity and
the recovered source's riot-morale fix. Probe morale uses SE probe, Telepathic
Matrix and achieved probe-improving technologies. Basic psi morale uses the raw
unit value as in source. Planet lifecycle uses the saved turn thresholds,
monolith state, Locust bonus and native flags; expansion Fungal Towers count
nearby fungus at shelf-or-higher altitude, preserving original wrap/duplicate
neighbor behavior. This movement-context API does not qualify opposing-faction
atrocity/tectonic lifecycle calculations for combat.

Project queries use the original **SP array index**, not the facility ID:
Maritime10, Telepathic30, Cloudbase35 (Thinker facility IDs80,100,105, with
SP_ID_First70). Project ownership follows the referenced base's actual faction.
Cloudbase belongs to the design owner, Maritime to the vehicle owner. Facility
bits use `id/8` and `1<<(id%8)`. Technology queries preserve faction0 exclusion,
None/Disable semantics, the disabled-prerequisite special case, ownership bits
and exclusion of Transcendent Thought88. Failed callbacks are errors, not canned
answers. Base-profile tower/spore constants are disabled explicitly. Unsafe
arithmetic and base references fail with unchanged outputs instead of reproducing
undefined behavior from corrupt inputs. Skip-morale speed does not require or
pretend to query the absent technology/morale dependencies.

`upstream_world_stats_reference.h` retains the recovered `has_tech`,
`morale_alien` and `morale_veh` bodies. Its test adapters read actual owned data
for projects, facilities, terrain and technologies. Unrecognized accesses fail
the test. `test_world_unit_stats.py --out NEW_DIR` compares144 synthetic worlds
against those source bodies and independent Python interpretation of raw original
wire fields. It covers both layouts, turn/lifecycle boundaries, raw/social/rule
morale, facilities/riots/immunity, probe technologies/ownership, projects by
owner, empty worlds, a narrow wrapped map, damage, speed storage bounds and
malformed arithmetic/references. Each valid world also compares1248 project/tech
queries and failed-call output preservation. Host sanitizers and independent
big-endian68040 CI run the controls.

The native recipe is `CXX=.../m68k-amigaos-g++ sh
build_world_unit_stats_probe.sh OUTPUT`. Actual AmigaOS qualification in
`WORLD-UNIT-STATS-EVIDENCE-20261025.json` passes147 cases, including the three
original tutorials with real saved morale/project/research state. The lab is
stopped/restored and assets remain private. These bindings remove supplied test
settings; they do not move units, grant terrain access, run AI/upkeep/combat or
advance research/turns. No original-executable differential fidelity is claimed.


### Movement prices on restored worlds

`world_movement_price.cpp/h` prices an adjacent edge for a real saved unit and
its saved design, including faction-specific slots64–511. Terrain comes from
owned original map bytes, Planet from the actual faction, and Xenoempathy
from project array index8 and its referenced base owner. Prototype speed is
recovered for the land/fungus branch. Antigrav/Slow queries also resolve
FREEABIL grants from the design owner and its actual prerequisite technologies;
the current vehicle owner does not receive another faction's bonus. These are
the only ability queries used by land pricing; radar/algorithm bonuses cannot
affect them. This does not yet extend the earlier vehicle-speed API to every
faction-granted ability. No supplied morale or project switches
stand in for world state. Base and expansion component profiles must match;
expansion-only Sealurk/Spore native exceptions are disabled in base mode.
The recovered source's Sealurk fix is retained explicitly.

`terrain_price.h` shares the original ordered arithmetic with the earlier
predefined API, whose stricter tile/native-ID contract remains unchanged.
The world API accepts all four raw rock encodings, including0xc0. In fungus,
a faction-specific psi design is distinguished from a predefined native unit:
the original condition is `id>=64 || offense>=0`. Roads, tubes, diagonal
rivers, ocean shelf fungus, Planet/Xeno/native exceptions, hover/antigrav,
rough terrain and former/artifact ordering follow recovered `hex_cost`.
Unknown item bits are inert for this price calculation, granting no action
permission. Invalid references/components fail with unchanged output.

`test_world_movement_price.py --out NEW_DIR` compares248 synthetic cases
against the recovered function body with owned-data adapters and an independent
Python interpretation of raw wire fields. It includes16 named cases per profile,
empty/narrow wrapped worlds, raw rock bits, varied road rates, faction-specific
psi, native profile differences and malformed references/components. Existing
predefined movement regression controls remain unchanged. Host sanitizers and
independent big-endian68040 CI run these cases.

The normal SDK recipe is `CXX=.../m68k-amigaos-g++ sh
build_world_movement_price_probe.sh OUTPUT`. Actual AmigaOS qualification in
`WORLD-MOVEMENT-PRICE-EVIDENCE-20261025.json` passed251 cases and25,588 edge
prices including all three original tutorials; the isolated lab is stopped and
its original configuration, ROMs, startup and module restored. Assets stay
private. These are prices, not unit actions: passability, stacks, ownership,
ZOC, events, spending, combat and turns still need integration. No original
executable differential fidelity is claimed.


### Original tutorial graphics view

`original_map_window.cpp` loads an owned original world and displays its real
terrain, unit coordinates and bases with the owner's original PCX artwork.
`pcx_image.cpp/h` bounds eight-bit one-plane version5 PCX decoding, scanline
padding, run lengths, palette and dimensions; it owns indexed pixels and RGB
palette and preserves an existing image on failure. No PC pointer, packed host
struct or endian-dependent pixel cast is used. Required faction image names
come from saved faction definitions, are restricted to plain alphanumeric,
underscore/hyphen basenames and lowercased before adding `.pcx`.

The view projects texture.pcx cells into a flat isometric map. Native unit
sprites come from Units.pcx. Original faction base sheets select land/water,
perimeter/tachyon appearance and a documented presentation population tier.
These asset coordinates and transparency colours were checked against GLSMAC
as layout facts; no AGPL source code was copied. Human CVR models remain
explicitly marked with labels, not substituted with invented unit art.
Slopes/coast joins, full improvements/roads and original fog-of-war are still
missing. The view reveals the map for inspection and does not execute actions
or turns.

Build with `CXX=.../m68k-amigaos-g++ sh build_original_map_window.sh OUTPUT`.
On AmigaOS RTG with cybergraphics.library41+, run:

```
alpha-original-view base alpha.txt Explore.SC texture.pcx Units.pcx
```

Place the required original faction PCXs in the current directory with lowercase
names, such as `gaians.pcx` and `spartans.pcx`. Missing required assets fail
explicitly. WASD scrolls, Tab selects the next actual unit, C centres it, B
cycles/centres bases, I opens the selected base snapshot, ESC closes it,
minus/equal selects zoom1/2, and Q exits normally.
This is a read-only rendering checkpoint, not a complete playable game.

`test_pcx_image.py --out NEW_DIR` runs23 synthetic image/malformed-input controls
with host sanitizers and independent big-endian68040 CI. Actual indexed pixels
and palettes from10 original image sheets match independent Pillow decoding.
`ORIGINAL-ART-EVIDENCE-20261025.json` records33 actual AmigaOS imports and an
interactive original Explore.SC run with selection, scrolling, both zoom levels,
real base/native sprite display and RC0. The muted isolated lab was stopped and
its original inputs restored. Assets and captured screens remain private.


### Original-asset restoration menu

`original_frontend.cpp` provides separate Alpha Centauri and Alien Crossfire
selectors, their original title PCXs, original TrueType lettering, the original
menu WAV, a base Explore tutorial shortcut and an original scenario/save file
requester. It embeds the qualified read-only world viewer and restores keyboard
focus when that view or the requester closes. This is a restoration preview;
it does not offer an implemented full campaign behind a New Game button.

Build with `CXX=.../m68k-amigaos-g++ sh build_original_frontend.sh OUTPUT`.
This uses the normal SDK SDL2_ttf and SDL2 libraries. In addition to the world
view's original data, place `openinga.pcx`, `xopeninga.pcx`, `ALPHC___.TTF`,
`arialn.ttf` and `opening menu.wav` beside the executable. Run `alpha-restoration`
from that directory. A/X selects the game/rules profile, E opens the base
original tutorial or requests an expansion scenario, O opens the requester,
M pauses/resumes music, and Q exits. Menu buttons also accept mouse clicks.
The file requester's Cancel button returns to the menu. Missing original fonts
or title images fail explicitly; unavailable audio leaves the menu operable.

Menu labels use the supplied Alpha Centauri font at20 points; supporting text
uses supplied Arial Narrow at18. These are original font assets, while the
menu arrangement/sizes are current presentation choices. Glyphs are rendered
through the SDK SDL2_ttf, converted to explicit RGBA bytes and alpha blended
onto RGB screen pixels without packed endian-dependent casts. The stock menu
WAV is PCM16LE/mono22050 and is converted to native sample order before the
looping audio callback. AMB procedural music is not implemented here.

`FRONTEND-EVIDENCE-20261025.json` records actual AmigaOS menu/title selection,
original lettering, nonzero muted output PCM, pause silence, resumed output,
original tutorial return, requester cancellation/focus and normal RC0 exit.
The final test used a private current target ROM/module with continuous frame
polling; the isolated lab was stopped and its original inputs restored. An
earlier test caught the requester leaving the menu inactive; this is fixed
and the full final sequence passed. Assets and captured screens stay private.


### Owned CVR model input

`cvr_model.cpp/h` independently reads the observed original CVR10.1/10.2
single-header geometry profiles: flags8 and flags10 with offset-free jump
groups. It owns voxel coordinates/normal indexes/colours, per-object animation
flags/locations/matrices, and indexed shade tables. Input bytes and failed
replacement loads cannot invalidate an existing model. Size, nested chunks,
counts, duplicate fields, finite values, coordinate bounds and termination
are checked. Chained geometry and offset jump headers fail explicitly.

The layout reference is the [Caviar format description](https://alphacentauri2.info/wiki/Caviar.html),
checked against original bytes. No GPL2-only CVR-Colorizer code or AGPL GLSMAC
code is incorporated. Original assets remain external and private.

`test_cvr_model.py --out NEW_DIR` exercises32 synthetic semantic/malformed
cases with sanitizers; `--cc m68k-linux-gnu-g++ --m68k-linux` runs them on an
independent big-endian68040. Build the actual AmigaOS probe using
`CXX=.../m68k-amigaos-g++ sh build_cvr_probe.sh OUTPUT`, then run
`cvr-probe MODEL.cvr`. `CVR-EVIDENCE-20261025.json` records all32 cases and two
original models on actual AmigaOS: the infantry's58 objects/17 frames/42,810
voxels and a reactor's12,198 voxels. Every geometry/animation/palette fingerprint
matches a separate Python interpretation; both input poisoning/freeing and
transactional failed replacement are checked. The isolated lab was stopped
and its original inputs restored.

This is a reader qualification. Original transforms, material/lighting and
chassis/weapon/armour/reactor assembly still require restoration and validation.
It does not turn the original-world viewer into a restored campaign.


### Saved-turn console and base information

The original-world viewer now requires original `console.pcx` (800x257),
`arialn.ttf` and `ALPHC___.TTF` in addition to its original world/faction art.
The normal build scripts link SDK SDL2_ttf/SDL2. `original_ui_text.cpp` renders
original font glyphs into explicit RGB pixels, replacing palette-pen assumptions
for the information areas. The frontend's existing menu and audio remain active
while the child viewer runs.

The original console frame contains the actual selected unit/owner, selected
base population, stored nutrients/minerals, saved surpluses and production name
when its current unit design is known. Facility names remain unavailable until
the corresponding reader is restored. A navigation thumbnail uses the actual
saved map with colours sampled from loaded terrain. Clicking it changes only
the camera. Clicking a base or pressing I opens its read-only details; B cycles
bases and ESC closes the details. All displayed accounting is a saved-turn
snapshot, not new-turn economic computation. Production and turn orders remain
unavailable.

The frame/fonts are original assets; the HUD labels, layout, thumbnail and base
details wiring are new restoration UI, not the original recovered screen code.
`HUD-EVIDENCE-20261025.json` records actual AmigaOS controls and checks both
original base snapshots against an independent raw save interpretation, including
legacy field offsets. A separate menu integration run checked fonts, nonzero
music output during the child view, return, requester cancellation and RC0.
Both tests continuously streamed the display, preserved the original scenario,
and stopped/restored the private lab. No live instance or ROM was changed.

## Fixed-depth projected CVR pixel stage

`cvr_raster.h/cpp` is an independently written bounded pixel stage for already
projected points. It is **not the CVR camera, lighting or a faithful unit renderer**.
The caller supplies signed 24.8 screen coordinates, a wrapping unsigned 16.16
depth accumulator, a 2048-entry normal-to-shade table and a 24-by-256 colour table.
The upper 16 depth bits are compared unsigned; strictly smaller wins and equal
depth retains the earlier pixel. No host byte-order assumptions or floating
point conversions are used. Invalid inputs fail before either output changes;
valid buffers, input points and tables must not overlap.

Two explicitly selected original pixel profiles are represented. Single pixels
use exclusive right/bottom clip limits. Two-by-two quads include right/bottom,
accept anchors one pixel left/above the clip and clamp those edge anchors inward.
This deliberate border behavior is not an ordinary geometric rectangle API.
The surface must encompass the whole clip and its stride is measured in pixels.

```sh
python3 ports/alpha-centauri/native/test_cvr_raster.py --out /tmp/raster-host
python3 ports/alpha-centauri/native/test_cvr_raster.py --out /tmp/raster-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_raster.sh probe /tmp/raster-probe
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_raster.sh control /tmp/raster-control
```

The synthetic corpus covers 464 batches and 9,568 points, including fractional
negative positions, both clip profiles, random existing depth/pixels, variable
shade/colour tables and near/far/tied depth. Another 26 controls check rejection,
transactional behavior, empty input and extreme coordinates. Public CI requires
both sanitizer-host and independent big-endian 68040 execution.

Privately, the same corpus matched execution of the owner's unchanged original
x86 pixel helper and its own address-binding patcher, without PE startup, Windows
imports or the full game. It also matched the normal SDK's actual AmigaOS binary
byte-for-byte in the isolated lab. The lab was stopped and its configuration,
ROM pair, module and startup restored. Only numeric receipts/hashes are public
in `RASTER-EVIDENCE-20261025.json`; no original code or assets are included.
The existing approximate model preview has **not** been corrected merely by
qualifying this stage: camera, light/shade preparation, faction palette mapping
and exact unit assembly remain separate work.

## Procedural CVR normal lighting

`cvr_lighting.h/cpp` generates the 24-by-2,137 reachable shade lattice from
spherical geometry. It includes no copied executable tables or game data.
The lattice uses 89 azimuth positions, a clamped 24-position polar grid, the
absolute normal/light dot product and rounded angular shade levels. The offset
of 88 in the normal indexing is deliberate; the older preview's normal decode
and clamped directional dot lighting did not reproduce these values.

The direction selector accepts finite light coefficients **already transformed and
scaled into object space**. It folds negative Z into the other hemisphere, selects quantized light
orientation and returns the 2,048 normal shades. Its azimuth scale deliberately
rounds 2*pi to float before computing the float scale; rounding the full-precision
ratio instead selects the wrong bin on the negative X axis. It does not normalize these coefficients: original animation matrices
and intensity change their magnitude. It rejects non-finite coefficients and bad buffer sizes or invalid selected lattice values before changing
output or orientation indices. Inputs and output must not overlap.

```sh
python3 ports/alpha-centauri/native/test_cvr_lighting.py --out /tmp/lighting-host
python3 ports/alpha-centauri/native/test_cvr_lighting.py --out /tmp/lighting-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_lighting.sh probe /tmp/lighting-probe
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_lighting.sh control /tmp/lighting-control
```

Public controls compare all 51,288 lattice bytes, 380 generated light vectors
(including signed-zero/cardinal axes) and 15 rejection/transactional cases.
The private original reference additionally checks every reachable original
lattice byte and the complete normal-shade outputs of 1,370 actual original
shade-preparation calls: 384 with four orthonormal object matrices and lights in
both hemispheres, plus 986 from all 17 frames of the original 58-object infantry
model (including non-unit transformed light coefficients). This is a lighting-stage comparison, not original camera,
scene light setup, matrix inversion, faction palette, unit assembly or full-game
qualification. Those remain separate integration work.

The final normal-SDK AmigaOS probe generates all 51,288 lattice bytes and matches
both the synthetic corpus and all 1,370 original selected tables byte-for-byte.
Fifteen invalid/transactional controls also pass. The isolated lab is stopped
and its configuration, ROM pair, module and startup are restored. Details are in
`LIGHTING-EVIDENCE-20261025.json`.

A native edge case was caught before qualification: the target math library's
`atan2(0,0)` returns NaN. The original FPATAN specifies signed-zero axes, so this
stage handles all axes explicitly instead of passing that case through the target
library. No installed SDK replacement or invented math-library stubs are used.

## Integrated compact UI and menu-audio lifecycle

The fallback UI profile uses the owner's Arial Narrow and Alpha Centauri fonts
at13 and16 pixels, crisp glyphs, ascent-correct baselines and separate text ink
roles. The preferred original Arial face is described below. The original console's unit and terrain columns have independent bounds;
selected terrain is read from the owned original map. These are presentation
choices guided by reference screenshots, not recovered exact original font
metrics. Bases and native-unit sprites paint in map-row order so a record ID
does not decide foreground overlap. Flat coast joins, original terrain slopes,
human CVR assembly and faction colours remain separate renderer work.

The opening-menu WAV pauses while the world is open. Returning resumes it only
if the user had enabled music. The world remains silent until its own audio
system is restored; the opening loop is not used as substitute ambient music.
Actual native PCM capture verifies nonzero menu output, zero output inside the
world, automatic resume, preservation of user mute through a second world visit,
and manual re-enable. The complete frontend exits normally. The isolated lab
stops and restores configuration, both ROMs, SDL module and startup hashes.
`UI-AUDIO-EVIDENCE-20261025.json` records bounded evidence; original assets,
screenshots and PCM remain private.

## Original UI face selection

The recovered original font setup requests the Arial family for status/datalink
text, including normal, italic and bold roles. Status text uses logical height10
at800px and12 otherwise; datalink roles include12/14/18. These are Win32 logical
heights, not a claim of identical SDL_ttf hinting or pixel metrics.
`OriginalUiText` now prefers owner-provided `Arial.ttf` at12 and
`Arial_Bold.ttf` at14. If the bold file is absent it uses the regular file's
synthetic bold style; if Arial is unavailable it retains the prior original
Arial Narrow13/title16 fallback and reports that choice. The port includes no
font files. Menus still use their existing separate title-font profile.

## Basic infantry in the original-world viewer

`cvr_infantry.h/cpp` composes a static frame of the owner's original VI infantry
body and Viptr00 reactor backpack. It reuses the bounded model reader, procedural
normal lighting and strict depth raster. Every component retains its own palette
while sharing the depth surface; a later component cannot overwrite an equal-depth
pixel or change a previously painted component's colours. Lazy facing/zoom RGBA
caches preserve the caller's background and are bounded below 3 MiB. Model profiles
are limited to 100,000 total voxels and all failures precede output writes.

The actual original-world viewer uses this path only for base-format, non-alien,
basic human infantry with chassis0, weapon0, armour0, reactor1 and no design
abilities. Other designs retain explicit CVR labels; an infantry body is never
substituted for the original tutorial's rover. VI.cvr and Viptr00.cvr are optional
owner assets in the game directory. No original models or palettes are included.

The camera tilt, heading convention, root component translation, base scale and
single-pixel profile follow recovered original facts. This viewer maps that scale
to its own 64-pixel tile basis. Tile-foot anchoring is new presentation wiring,
not recovered original viewport placement. Only frame0 is shown. Original shadow,
complete animation, terrain-dependent camera, full faction/ability assemblies,
global indexed-palette remapping and original whole-scene pixel fidelity are not
claimed. The terrain view and game interactions retain their existing limits.

```sh
python3 ports/alpha-centauri/native/test_cvr_infantry.py --out /tmp/infantry-host
python3 ports/alpha-centauri/native/test_cvr_infantry.py --out /tmp/infantry-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_infantry.sh probe /tmp/infantry-probe
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_cvr_infantry.sh control /tmp/infantry-control
```

Public synthetic controls exercise separate component colours, equal-depth
occlusion, hidden parts, singular transforms, size/frame/profile limits,
16 facing/zoom combinations, transparent caching and transactional rejection.
Actual normal-SDK AmigaOS qualification passes the9 synthetic CLI cases and
16 original-model facing/zoom outputs, with all hashes and the rendered PPM
matching host results exactly. The actual original Explore world shows its basic
infantry body/backpack at both zooms, selects the next unit and returns/exits
normally. Owner Arial12/Arial-Bold14 fonts are selected. The isolated lab actively
streams102 frames, then stops and restores all configuration, ROM/module/startup
hashes. `INFANTRY-EVIDENCE-20261025.json` records this bounded proof; original
whole-scene pixel equivalence and the other stated limits remain unclaimed.

## Original vehicle stacks and relocation storage

`vehicle_stack.h/cpp` adapts the recovered `veh_lift`, `veh_drop` and `veh_top`
storage operations. It validates every stack link before writing: bounded
references, reciprocal links, matching coordinates, unique roots, no cycles,
and agreement with the map's vehicle-present bits. Corrupt state is rejected
without silently repairing it. Storage arrays must be disjoint and exclusively
owned by the caller; suspended/off-map actors are not supported.

Relocation preserves the original source tile's owner, updates both neighbors,
inserts the actor at the destination's stack head, preserves base ownership and
the high nibble of the tile owner byte, and marks supply suppression for
non-native land/sea units. Moving within the same square promotes the actor to
the head, as lift/drop does. All validation and allocation precedes all writes.

This is an internal storage primitive, **not a legal movement command**. It does
not establish player authority, passability, transport rules, zone of control,
events, movement expenditure, animation, upkeep, turns or save serialization.
It is not yet exposed by the read-only world or UI. Those action gates must be
implemented before wiring it to player input.

```sh
python3 ports/alpha-centauri/native/test_vehicle_stack.py --out /tmp/stack-host
python3 ports/alpha-centauri/native/test_vehicle_stack.py --out /tmp/stack-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_vehicle_stack.sh control /tmp/stack-control
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_vehicle_stack.sh probe /tmp/stack-probe
```

Controls compare 18,432 relocations against a separate recovered-source-flow
reference and reject 20 malformed/no-write cases. The original-file probe works
on copied decoded tutorial records, leaving the imported world and original
file unchanged. Host sanitizers pass all three owner's tutorials (498 storage
relocation controls); normal SDK builds pass. Actual normal-SDK AmigaOS
execution passes those same 18,432/20 controls and all 498 original-tutorial
relocations with RC0. The isolated lab is stopped and restored. Independent
big-endian CI remains required before merge; evidence is recorded in
`STACK-EVIDENCE-20261025.json`.

### Static basic human speeder profile (native qualification pending)

The actual original-world viewer can now render the base-game basic human
speeder (chassis1/weapon0/armour0/reactor1/no raw abilities), including the custom
prototype320 in the original Explore tutorial. It does not substitute infantry
for the rover. The ten original components are appended in source-backed order:
VRCP00, VGMC, VGMCP, VB, VBP, VSP, vsptb, vsptf, VLIGHTS, VR00. Assets stay private
and are loaded from the viewer's directory using those lowercase filenames.
Missing parts or unsupported variants keep the existing explicit labels.

The composition reuses strict shared depth, separate component palettes and
bounded transparent facing/zoom caches. `CvrUnitAssembly` retains the prior
`CvrInfantry` API alias. Infantry remains capped at100,000 voxels; the ten-part
speeder at160,000. The original infantry's16 hashes and PPM remain identical.

VLIGHTS requires CVR10.0 and five chained geometry LODs. The reader now validates
up to16 ordered levels, including unselected geometry, and selects using the
original1.414199948310852 threshold. Exact float-input products are compared in
double: a bounded original x86 helper exposed premature float rounding at LOD
boundaries. This bounded static profile rejects assets whose selected LOD
changes between its two supported zooms; the owner lights choose the same
coarsest level at both. Non-STOP single streams and the observed CVR10.0 opaque alternate
palette length are supported; unknown flags/offset jumps remain rejected.

Run `test_cvr_lod.py` for40 synthetic LOD/legacy/boundary rejection controls and
`test_cvr_speeder.py` for34 ten-part ordering/palette/hidden/transactional/cache
controls. Both have independent big-endian68040 CI commands. The normal SDK
`build_cvr_speeder.sh probe OUTPUT` builds a16-facing/zoom original-asset control;
run it as `PROBE PRIVATE_MODEL_DIRECTORY OUTPUT.ppm`. The actual frontend is
built normally with `build_original_frontend.sh`.

This is static frame0 and raw per-model colours. Original faction-table
remapping for VGMCP/VBP, full animation, global palette/shadows and exact original
viewport remain unfinished. Source-derived camera/facing/scale is mapped into
our viewer's64px tiles; the foot anchor is our own UI wiring. No gameplay or
full-scene equivalence claim. Native execution and integrated original tutorial
qualification are **pending**; the user's live watch session is untouched.
See `SPEEDER-EVIDENCE-20261025.json` for the explicit current validation bounds.
### Original global/faction colour mapping (core only; native/integration pending)

The original CVR pixel stage writes **global game-palette indices**. The model's
embedded RGB triplets are not the final display palette. Normal appends use the
model's shade-index table; faction appends replace it with a generated table.
This explains why displaying each component through its own RGB palette cannot
reproduce all original unit colours.

`cvr_palette.h/cpp` now provides two independent bounded mathematical pieces:

- `build_cvr_default_global_palette` takes the owner's `programs/palette.pcx`
  RGB palette. Source entries0..235 become game entries10..245. Under the
  original default gamma1 path each channel is capped at235; reserved slots
  follow the normal indexed Windows branch and are computed procedurally.
- `build_cvr_faction_palette` takes an explicit global RGB palette, neutral
  24x256 shade-index table and RGB tint. Each source global colour is multiplied
  by the tint with truncating division by255. A squared-RGB nearest-colour
  search over all256 global entries keeps the first index on equal distance.

The global gamma helper actually uses normalisation by235 and output scale
235.99, rather than255. Five private original-helper gamma cases match an
independent formula. Only the default gamma1 integer path is implemented here;
custom gamma and the alternate OS-system-palette branch are excluded.

Faction file metadata supplies the tint: the raw pixel at(435,745) is shifted
by10 with byte wrapping before indexing the global palette. The preceding
intensity metadata at(161,757), and the tint pixel, must not be the raw255
transparent marker for the per-faction override to run. Initial/system defaults
and missing metadata require the original fallback path; this core API leaves
that choice explicit. The source neutral table for faction panels is VBP.

Private controls actually execute original palette initialisation/DIB copying,
gamma/CRT maths, and the complete faction inner loop plus nearest-colour helper.
All256 generated default global RGB entries and all43,008 shade bytes for the
seven original faction metadata tints match the new APIs exactly. Original
instructions, palettes, shade tables and recordings remain private.

`test_cvr_palette.py` checks24 generated faction tables (147,456 entries),
14 invalid/overlap cases, three generated global palettes and six invalid global
inputs on host and independent big-endian68040. `build_cvr_palette.sh faction`
and `global` build the normal-SDK CLI probes; the former reads an ACPA/version1
container (RGB768, neutral6144, tint3), the latter reads raw PCX RGB768.

**Not connected to the viewer yet.** Actual AmigaOS execution, faction/default
selection and shared renderer cache binding remain required before this can be
called a visible colour fix. This scope neither changes PR87 nor touches the
user's lab session. See `PALETTE-EVIDENCE-20261025.json`.


## Optional game-palette viewer integration (native execution pending)

The scoped integration now binds the default global palette from the owner's
`palette.pcx` to supported infantry and basic speeder rendering. Speeder panels
VGMCP/VBP additionally use the actual unit faction's valid tint metadata. All
other components retain their own shade-index tables; their resulting indices
resolve through the global palette. The original final blit key9 remains
transparent. No original asset or palette table is included in this repository.

Changed global RGB, tint, or mode invalidates all sixteen facing/zoom cache
entries transactionally. Repeating a configuration preserves cached results;
invalid inputs leave the previous configuration intact. Missing game palette
uses the explicitly labelled raw-model preview. Missing/unsafe faction metadata
uses explicitly labelled neutral global colours, without inventing the original
system/default tint. Unsupported unit profiles are unchanged from PR87.

`test_cvr_palette_render.py` exercises both faction-panel positions, eighteen
cache transitions/288 draws, and sixteen original-key9 draws with synthetic
assets under host sanitizers. Original-owner host controls preserve the earlier
raw infantry image exactly and exercise global infantry plus three actual
faction rover colours. The normal SDK frontend and CLI probes build without
toolchain overlays. Native CLI execution and integrated Explore UI validation
are still pending while the user's live watch session owns the lab. This is
not yet a verified visible/native colour fix, and does not restore original
shadows, custom gamma, alternate system palettes, or additional unit profiles.

### World transition masks and coast pixel stage (26 October)

`world_terrain_masks` computes four three-bit coast sectors and eight-neighbour
roughness/fungus/jungle masks from owned map records. The view supplies displayed
centre items explicitly; missing context rejects transactionally. This is not a
complete human-visibility/fog resolver. Roughness includes neighbours at least as
rough as the centre, with original ocean and disabled-landmark rules.

`draw_terrain_coast` draws four vertices using supplied primary/mask textures and
lookup: zero preserves destination, one selects sampled texture plus signed
light, and other values select a constant palette index. Fog stripes affect only
nonzero lookup pixels. Unlike the ordinary triangle kernel, original coast light
is not clamped to -4..4. Validation rejects source/output overlap and invalid
sampled indices before writes. Caller ordering, palette/lookup generation,
secondary coast material loading and complete water/coast scene assembly remain
unbound. The two new tests run public synthetic fixtures on host and independent
big-endian 68040; private original numeric/pixel controls are recorded in
`COAST-EVIDENCE-20261026.json`. Normal SDK builds are available through the two
new build scripts; actual AmigaOS execution and frontend integration are unrun.

### Explicit original coast resources and caller UVs (26 October)

`prepare_terrain_coast_resources` binds the first original coast mask crop in
RAINFALL.PCX (raw source rectangle 2,333,56,56) and six original dynamic palette
samples. It requires the edition's explicit 53-class/53-by-8 rule metadata;
these owner data are not bundled, inferred from gameplay state, or replaced by
invented colours. Source palette indices add 10 modulo 256, with the loader's
252/253 replacement rule. The supported first mask crop contains no transparent
pixels; otherwise preparation rejects rather than guessing an OS allocation's
initial contents. Unknown nontransparent classes retain the source zero mapping.

`prepare_terrain_coast_sector` turns the qualified ordinary ground mesh into
W/N/E/S coast quadrilaterals with source signed midpoint flooring and 27/28 UV
seams. It builds the dynamic 53-entry lookup, including the shore-mask-seven
coordinate-parity exception. Outputs reject overlaps with source data and
commit transactionally. Explicit caller inputs remain responsible for view
state and ocean-detail lights; this does not execute the full World caller or
restore water/coast scene composition. Host and independent big-endian controls
use synthetic metadata and original owner data privately. The original lookup
numeric region was separately executed; OS loader/Sprite/Buffer initialization
and actual native execution remain unrun. See the resource evidence JSON.

### Original ocean detail and bounded coast overlay (26 October)

`world_ocean_details` restores all sixteen original sector/vertex contour detail
queries: source contour clamping, fixed-four-neighbour corner means, map wrap
and edge zeros, +/-7/3 land adjustments, and the 57/67 coast-side exceptions.
The complete original numeric helper was run against all three tutorial maps;
55,296 contexts match on host and independent 68040. The public synthetic suite
checks 6,144 further contexts, invalid queries and transactional alias rejection.
`light_ocean_details` requires the actual edition's explicit 80-entry renderer
light table. It does not invent a colour gradient or obtain it from gameplay
RNG, save pointers, or a guessed allocation identity.

`render_terrain_coast_overlay` connects the qualified mesh, supplied World shore
masks, ocean lights and owner coast resources. It crops the original prepared
primary material at (280,79,56,56), prepares source sector UV/lookup, and draws
active sectors in original order over existing ground pixels. Supported input
must explicitly establish an ordinary land coast context and fog -1/65. The
small indexed destination is staged so any invalid later sector leaves the
entire destination unchanged; input/material/descriptor overlaps reject.

Forty-eight synthetic and forty-eight owner-material/World-shore-light patches
match the actual unchanged original coast pixel helper on both architectures.
Patch geometry remains an explicitly supplied, qualified mathematical mesh
profile: this is not a full World caller execution, view/fog resolver, actual OS
loader or complete sea/water scene. Original owner class/rule/light tables and
assets remain private. Normal SDK builds pass; native execution, frontend
integration and remote CI are unrun. See `OCEAN-COAST-EVIDENCE-20261026.json`.

### Source water primary/threshold pass (26 October)

`build_world_water_plan` and `render_world_water_patch` restore the bounded
ordinary ocean pass at original SMAX 46452c..464720. The unchanged original
region computes its World contour detail, sector XY/UVs, renderer lights and
both pixel layers; the private oracle executes those actual dependencies and
RAM Buffer calls. The owned plan matches the source's asymmetric water UVs
(which differ from coast UVs), flat ocean geometry, deep-base condition (any
pointlight >=16), then threshold-16 primary order. Materials are the prepared
TEXTURE atlas crops (280,136,56,56) and (280,79,56,56). The independent host and
big-endian comparisons include all three original tutorial maps.

An important runtime boundary is explicit: only land sets primary-light clamp
bounds to -4..4. Original initial data and function-end reset are -256..256;
ocean inherits the actual global bounds. The new primary quad kernel and World
adapter require supplied bounds, without silently selecting the land profile.
Both profiles were separately executed and compared. The first oracle's -4..4
profile is preserved as a control, not claimed as normal ocean-reset proof.
Threshold drawing is unclamped and compares signed interpolated shade against
16. Its fog rows fill only after the all-vertices-at-or-above-threshold early
skip. Primary fog retains the original odd-row texture prefetch lag.

Source context must establish normal Scale25, zoom1/2, ocean classification,
source draw flags excluding 82000000, fog -1/65, actual caller coordinates and
runtime light bounds. The renderer's edition-bound 80-entry light table remains
explicit private owner metadata. The adapter stages the small indexed surface
and rejects World/input/material/descriptor aliases transactionally. Public
synthetic checks run without original assets or executable bytes. Native/OS
allocation, complete MapWin view resolution, full world scenes, base-Terran
renderer differential and frontend integration remain unrun. No private render
allocation identity or gameplay RNG is invented by this deterministic pass.
See `WATER-EVIDENCE-20261026.json` for exact scope and counts.

### Owned terrain observation and caller ordering (26 October)

`terrain_observation` binds the closed original tile-observation prelude to the
owned World. The edition, viewer, current faction, source zoom, draw flags,
rules, more-preferences and game-state flags are explicit runtime input. They
are not guessed from saved preferences. It preserves actual versus per-faction
remembered items, source viewer-0 landmark access, rainfall and rockiness,
concealment and the proposed landmark-cache bit. The World remains immutable.
Neighbour/sensor/actor visibility and the original unsafe negative-viewer
faction lookup return distinct held results without changing output.

`terrain_caller_order` records the original outline gate and ordinary terrain
pass ordering. The source outline argument is explicit and deliberately unnamed;
values above 21 outline. A non-revealed tile also outlines under the original
rules/visibility test. Land changes the light clamp to [-4,4] before observation;
ocean inherits the supplied runtime bounds. The later normal exits restore
[-256,256]. Source hidden-terrain mode excludes terrain. Other modes, except
ordinary flags 0/1, are held. This describes the source gate and ordering; it
neither calls renderer helpers nor draws an outline, terrain or a full scene.

The existing dispatch API's compatibility names need care: its `rockiness`
field takes source **rainfall** `(climate >> 3) & 3`, and its `moisture` takes
source **rockiness** `val3 >> 6`. The existing world-mask `roughness` is a
rainfall transition mask. Their frozen arithmetic is unchanged here.

The public synthetic controls actually run 6,912 observation and 8,640 caller
contexts on host sanitizers and independent big-endian 68040, plus malformed,
transactional/alias and missing-runtime controls. Private unchanged source
regions from each original executable match 1,656 observation contexts and
2,592 outline decisions per edition on three owner tutorials. These executions
stop before unresolved visibility and renderer helpers: no full PE, Windows,
whole MapWin or full-world pixel claim. Normal SDK probes build; native is
unrun. Four new workflow commands retain every inherited command.

Full composition still requires source-backed neighbour visibility, active
height/cache and render-identity context, special landmark/material branches,
items/actors and actual OS/caller qualification. Neither this stage nor the
existing bounded land/coast/water pixel controls enable the frontend.

### Source-backed neighbour, sensor and radar visibility (26 October)

`terrain_visibility` completes the neighbour decision when the closed
observation prelude needs it. Moving actor coordinates/id, expansion-enabled
state and actual runtime map flags are supplied explicitly. A map-flag geometry
mismatch is held. It uses valid reciprocal stacks and base-present bits, the
original 25-neighbour order and one-step longitude wrap, diplomatic stack
searches, sensor ownership, matching land/sea/air radar and the expansion base
facility radius. Corrupt topology/base bits are rejected before queries; no
original bit-repair, log or OS path is substituted.

Base terran and SMAX differ. Base radar tests the raw prototype bit only and its
sensor test uses the raw tile item. SMAX also resolves design-owner alien radar
and technology-gated free ability 1; its sensor query uses the closest base
(last equal-distance base wins) and Geosynchronous Survey Pod facility 41. The
additional expansion radius is separately controlled by the explicit runtime
expansion-enabled flag. Owner and design-owner are not conflated.

The original reverse diplomatic check can address faction[-1] when a tile has
no occupant. The owned query never reads before an allocation or presumes those
bytes are zero. It takes eight explicitly observed low pact bits for that
runtime memory and returns a distinct held result if the check is reached
without them. A source negative-viewer faction lookup also remains held. Valid
short-circuit visibility returns before an unneeded held dependency.

Public controls cover 38,016 contexts on eleven independent synthetic worlds,
including both editions, moving actors, raw/alien/free radar, technology absent,
present and disabled, sensors and base facilities. Actual host ASan/UBSan and
independent big-endian 68040 agree exactly, with nine invalid-domain controls,
fourteen full/partial alias checks, missing-runtime, held-geometry and partial
input controls. The normal SDK probe builds; native remains unrun.

Private bounded unchanged original code, including the real helper
implementations, agrees with the owned host and independent big-endian outputs
in 57,232 contexts: 51,072 on the three owner tutorial Worlds and 6,160 on
synthetic inputs. Both original executables are exercised separately. These
runs stop at the observation boundary; no Windows callback, bit-repair path,
whole MapWin, original PE startup or full game executes. Oracle setup failures
were retained and corrected against actual instructions, not hidden by changing
expected outputs.

The query commits only its owned observation result. It proposes rather than
mutates the landmark cache bit, and does not reproduce the diagnostic global
BaseFindDist write made by the original nearest-base helper. This still is not
full caller composition: active height/cache provenance, source render identity,
special material/landmark branches and later item/actor/OS layers remain
separate gates. No frontend or watched instance is changed. Whole-world
stack/base validation currently runs per query; native performance and a safe
validated-scene cache have not been qualified.

### Ordinary indexed World terrain scene (private qualification, 26 October)

`terrain_scene.h` joins the owned observation/visibility and numeric caller stages
with the qualified height, lighting, allocation-seeded material/UV, ground,
coast-mask, and deep/threshold water stages. It renders the ordinary flags0
terrain path into a caller-owned small indexed RAM surface. The source private
view must have a fresh active cache, normal height25, no global scale override,
and matching source zoom-5 or5. Original tile allocation identity is mandatory;
there is no gameplay-RNG, saved-pointer or host-pointer fallback.

The complete destination update is transactional, including a late coast/water
failure after ground has succeeded. It rejects full and one-byte partial aliases
against World storage, rules/unit definitions, every resource and descriptor.
The public synthetic suite exercises432 scenes in three Worlds, both normal
zooms and clipped placements, plus feature/height holds and missing/invalid/alias
controls on host sanitizers and independent big-endian68040.

Private owner-data comparison now enters the unchanged original SMAX
`MapWin_gen_terrain_poly` at its actual function entry and reaches its normal
return, rather than replacing internal callbacks or jumping around the caller.
It uses the real RAM Buffer handle-null texture path, actual original height and
light cache helpers, and original source metadata with privately prepared owner
textures. For Explore, Build and conquer,384 supported contexts match every
indexed pixel on both host and independent68040. Another192 contexts are held
transactionally (concealment/features or changed survey rainfall); their source
outputs are retained without claiming equivalence. The source harness supplies
real guest thread-local SEH storage; chain, stack, lock count and exit light
bounds are checked. No Windows API, original exception handler, PE startup,
physical display or full game ran. This source oracle uses SMAX code with the
three original base tutorial Worlds; it is not a base-executable differential.

This remains a bounded ordinary terrain compositor, not a complete map frontend.
Outline/hidden modes, fungus, jungle, special height features, terrain overlays,
items/actors/labels, global RGB palette installation and original screen blits
remain outside this contract. The original cache-bit/global side effects are
not committed to the immutable World. Actual native/UI qualification and viewer
integration are unrun. No lab or watch session was touched.

### Concealed ordinary terrain continuation (26 October)

The child scene contract now closes the original `GameRules & 0x100` unexplored
observation branch. It retains original raw height/coast geometry while the
observed centre rainfall/rockiness become zero. It disables the rainfall
transition mask, uses source fog light2 without querying the brightness cache,
and suppresses jungle/fungus only where the original concealment flag does so.
Revealed feature layers remain held. Raw rainfall/rockiness3 are explicitly held
for land material dispatch, while concealment's genuine zero override permits
them. Special landmark heights remain held even when concealed.

Private comparison uses the correct `alphax.txt` rules/abilities for the unchanged
SMAX caller, with actual source expansion-runtime flag0 and observed negative
owner pact bits supplied explicitly. All576 earlier complete-call contexts now
match every pixel;10 additional concealed original fungus/jungle contexts also
match on host and independent68040. The former22 observation holds included a
probe metadata mismatch (base rules supplied to a SMAX context); no ability
counts were fabricated to get past it. The old frozen evidence remains intact.

The public suite retains432 ordinary scenes and adds432 concealed scenes in
three more synthetic Worlds. These include raw class3 fields and concealed
fungus/jungle, both normal zooms and clipping. Separate tests retain six revealed
feature/height/class holds,16 invalid/context controls and50 full/partial aliases.
Both architecture digests are fixed expectations. The source `0x01000000` draw
flag is a different flat-colour polygon mode and is still held; it must not be
confused with the GameRules observation branch. Native/frontend/global RGB/OS
and complete world/game claims remain outside this proof.

### Revealed jungle and fungus (26 October)

`terrain_keyed_quad` closes the original unlit keyed quad used by jungle and
fungus. It tests the raw indexed key before writing; fog affects only non-key
pixels on even absolute rows. It has no light addition. Importantly, its edge
UVs do not use the lit texture path's half-texel bias. The initially reused bias
was rejected against unchanged original helpers and is preserved privately.

`terrain_feature_layers` uses the instruction-backed original 15-role blob
selector, original UV rotation and genuine prepared texture atlas crops. The
ordinary flags0 order is ground/water, jungle, land/ocean fungus, then coast.
Jungle is unlit without fog stripes; fungus uses the source fog parameter.
Concealment suppresses both and ocean fungus starts at altitude2. Special
landmark heights, raw land material class3 and other source modes remain held.
A later forest implementation must respect its source position between jungle
and fungus; it cannot simply be appended after this flags0 combined layer.

Public checks now run288 keyed quad cases,4100 layer compositions covering all256
raw neighbour masks, and1296 ordinary/concealed/feature World scenes on host
sanitizers and independent big-endian68040. Transactional invalid/alias checks
include a failure in the second layer after the first layer has drawn. The
existing ordinary and concealment pixel fingerprints are unchanged.

Private unchanged original helper execution independently matches all4100 layer
compositions using actual source class/rotation metadata and5120 original helper
calls. This uses explicit synthetic numeric textures/flat caller vertices and is
not a whole MapWin claim. Separately, the complete unchanged original MapWin
entry/normal-return comparisons now cover618 original tutorial contexts,
including32 revealed fungus cases, and96 jungle contexts from unchanged owner
planet/planetx MP terrain. The latter use explicitly synthetic actorless
containers, not original saved campaigns. All714 supported complete-call pixel
contexts match both architectures. No original assets/tables/code bytes are
committed. Normal SDK probes build; native/UI, RGB palette/OS loader/screen blit,
full-world placement and complete-game qualification remain unrun.

### Source-ordered forest layer (26 October 2026)

The private scene candidate now accepts ordinary source draw bit1 for the
original shaded forest pass, between the existing unlit jungle and fungus
layers. Four diagonal neighbours supply the 16 raw forest-bit variants; the
prepared owner atlas provides the actual 56-pixel crops. The Gouraud quad tests
the original raw key before adding interpolated indexed shade, with the
source half-texel edge setup. Forest does not inherit fungus fog stripes.

The same source bit also enables land farms and ocean-platform sprites. Both
remain explicit transactional `held_features` contexts. No frontend uses this
candidate. All earlier caller/cache/source-allocation, landmark, OS-loader and
physical-display exclusions remain.

Portable fixtures run 576 shaded quads, 2,048 layer combinations and 432
additional World forest scenes on host sanitizers and actual independent
big-endian 68040. All earlier 1,296 scene digests and 4,100 plant-layer controls
remain unchanged. Private original-code controls match 576 complete shaded
helper calls and 128 complete MapWin entry/normal returns. The latter use the
original Explore terrain and genuine owner textures, with explicitly controlled
forest/jungle/fungus bits covering all 16 forest masks. These are not an original
saved forest scene. The original oracle is SMAX code with legacy base-world
data, not a base-executable differential. Normal Amiga SDK builds exist;
native execution, UI and deployment are UNRUN. See
`FOREST-SCENE-EVIDENCE-20261026.json` for the exact limits and digests.

### Land farms and sea-platform boundary (26 October 2026)

This child supersedes the forest checkpoint's land-farm hold. Source draw bit1
now selects the original farm primary texture, preserving its source-address
variation, unrotated UVs, rainfall overlay, geometry/light and plant/coast order.
The sea-platform sprite remains explicitly `held_features` and leaves the
destination unchanged. No frontend has been enabled.

The unchanged Conquer tutorial contains a real farm at tile 587. Its eight
complete original MapWin calls match all pixels on host sanitizers and actual
independent big-endian 68040. Another 72 complete calls cover 18 controlled
material profiles using original terrain and owner textures, with rainfall and
rockiness overrides and both source-identity variants explicitly recorded.
Portable fixtures add 432 farm scenes; all earlier scene digests and the 127
inherited workflow commands remain. The previous 618 tutorial and 128 controlled
forest original-call comparisons were rechecked on the new production source.

Sea platforms use the original TER1.PCX crop 607,190 of 100x62 pixels, rather
than a ground quad. The recovered source path trims it and draws with source
scale/cache state. Its nonempty RAM sprite branches unconditionally require
the imported Windows IntersectRect function. No substitute callback was used;
complete extraction and sprite execution remain unqualified. Crop/trim facts
are private numeric observations, not an enabled or validated sea draw.

A normal Amiga SDK scene build is retained; native execution, UI and
deployment remain UNRUN. See `FARM-SCENE-EVIDENCE-20261026.json`.

### Original road/magtube scene layer (26 October 2026)

Ordinary source flags0/1/0x20/0x21 now admit roads/magtubes after the
plant/coast passes. The owned plan preserves raw source base bits, the first
matching base record, base visibility, viewer-specific neighbour snapshots,
eight-direction order and isolated road/tube fallback. It draws the genuine
prepared owner texture crops through the qualified shaded keyed quad.

The original caller has a separate later exploration gate: terrain reveal
does not grant roads/improvements on unexplored tiles. The first original
comparison exposed 12 such mismatches; the failed source/binary/receipt remain
private, and the source-backed gate now matches. Stale base bits stay held
instead of executing or pretending source repair/log/OS callbacks. Visible
crop-yield and improvement/landmark sprites sharing bit0x20 remain explicit
transactional holds, as does the sea platform. No frontend is enabled.

Public controls run 4,340 plans against an independent raw-file reader, 1,024
layer compositions and 432 new World road scenes, plus late-gate/alias/failure
controls, on host sanitizers and actual independent 68040. All earlier scene
digests remain unchanged. The numeric caller retains its prior digest and
adds 432 road-flag cases. All 127 parent workflow commands remain, plus two
new road-control commands.

Private complete unchanged MapWin entry/normal-return comparisons match all
pixels in 180 unchanged tutorial contexts and 128 controlled connection contexts
using owner textures. The latter explicitly alter terrain/item bits; they are
not original saved road worlds. A first fixture edited the wrong neighbour
location and was rejected as coverage despite matching pixels; its history
remains. Corrected coordinates cover all eight road/tube directions and the
isolated fallbacks. Earlier 826 original contexts were rechecked unchanged.
Normal SDK builds are retained; native/UI/RGB OS qualification is UNRUN.
See `ROAD-SCENE-EVIDENCE-20261026.json`.


### Original river scene layer (26 October)

Source draw bit `0x400` now composes the original river patch after coast and
before the separate late exploration gate. Its four diagonal connections use
raw neighbour river bits; centre source bit `0x200` adds mask zero a second time.
These raw river effects are retained in fog/concealed terrain exactly as in the
source caller. The keyed Gouraud fog shader preserves raw key 9 on both row
parities, uses fog on even absolute destination rows and interpolated light on
odd rows. No Windows import or sprite-scale/cache callback is substituted.

Public host sanitizers and independently executed big-endian 68040 controls cover
1,152 quad cases, 256 layer compositions, 1,296 new World scene contexts and
864 additional caller cases. All 2,592 previous synthetic scene results and both
previous caller prefixes remain unchanged. All 129 inherited workflow commands
remain, plus four river commands. Private full original entry/normal-return
comparisons match all pixels in 384 controlled owner-terrain contexts (all 16
connection masks, source overlay, two zooms, clipping and three observation
profiles). These deliberately modified fixtures are not original saved river
worlds; the three original tutorials contain no raw river tiles. The original
prepared atlas is private. SDK builds are build-only: native, watched preview,
frontend deployment, physical RGB/OS loading and full-world qualification remain
UNRUN. Existing sea-platform and later sprite holds are unchanged.
See `RIVER-SCENE-EVIDENCE-20261026.json`.


### Original terrain grid (26 October)

The ordinary RAM scene accepts original draw bit `0x04000000` and grid preference
`0x100`. It draws only each tile's two shared north edges, after ground and before
plants/coast/rivers. Both raw endpoints must be land for indexed colour 37.
Explicit **MorePreferences** `0x800000` enables water edges, which use colour 190
when either endpoint is water; this flag is not GameState. Pole/outside neighbours
skip and cylindrical X wrapping follows the source once. There is no inferred
visibility or palette authority.

The owned integer line raster matches 1,313 complete original RAM helper returns,
including reversed lines, clipping, strict positive-error ties, horizontal and
vertical endpoints, and degenerate points. RAM clip and DIB dimensions agree;
physical display and imported surface paths are not executed. Public host
sanitizers and independently executed 68040 controls cover 400 plans against a
separate raw-file model, 72 compositions, descriptor/partial-alias rejection,
1,296 additional scene contexts and 1,296 preference-equivalence calls. Parent
3,888 synthetic scenes and all prior caller prefixes retain their exact outputs.
All 133 parent workflow commands remain, plus four grid commands.

Private full MapWin entry/normal-return comparisons match every pixel in 2,560
unchanged tutorial contexts across grid flag/preference, sea-grid, visible/fog/
concealed profiles, both zooms and clipping. Earlier 1,518 original pixel contexts
were rechecked unchanged. Two rejected implementations are retained: the initial
sea flag bound to GameState, and a land-centre-only edge colour. Original results
were not weakened; both corrections follow instruction-backed branch behavior.
Normal SDK binaries build; native/UI/RGB OS qualification remains UNRUN. Imported
sprite/sea-platform scale and cache dependencies and other later modes stay held.
See `GRID-SCENE-EVIDENCE-20261026.json`.


### Original base-work grid (26 October)

Original map preference `0x200` now draws the base-work boundary after rivers and
the late raw exploration gate, before roads. It classifies raw `val3 & 7`
(using-faction), viewer diplomacy bit 8 and source cursor map-range below three,
then compares the four diagonal neighbours. The lines use original index 249.
This is the base-work grid, not political territory rendering. No ownership,
visibility or cursor authority is invented: the caller must supply the actual
source cursor globals' coordinates. Missing or invalid cursor context rejects
without changing pixels. This scope admits only on-map, parity-valid cursors;
source outline and off-map-cursor contexts remain held.

Public host sanitizers and independently executed 68040 controls cover 2,624
plans against a separate raw-file model, all 16 edge masks in 128 compositions,
3,888 new World scenes, missing/invalid cursor rejection, aliases and late
failures. All parent 5,184 synthetic scene outputs remain unchanged. All 137
parent workflow commands remain, plus two base-grid commands.

Private complete original MapWin entry/normal-return comparisons match every
pixel in 5,696 unchanged tutorial contexts using two explicitly controlled source
cursor positions, plus 944 combined ordinary-grid/base-grid contexts. The
controlled cursor values are not claimed to be the live game's cursor. Earlier
4,078 original pixel contexts were rechecked unchanged. The production overlay
matched the original on its first comparison. Rejected test scaffolding is
preserved: a legacy/current faction-stride mixup, cursor assertions placed ahead
of an intentionally earlier feature hold, and a receipt-variable shadow.
Final controls preserve the original hold priority and exact pixel expectations.
SDK binaries build only; no native, frontend, watched-preview, physical RGB/OS
loading or full-world completion is claimed. Imported sprite scale/cache and
other unsupported scene modes remain held.
See `BASE-GRID-SCENE-EVIDENCE-20261026.json`.


### Base-work grid byte-span alias inspection (26 October)

The frozen rendering World keeps both `game().bunglist` and imported monument
spans inside `original_bytes()`. The existing raw-buffer guard therefore already
rejects aliases into them; this branch does not claim a current separate-storage
bug. An explicit bunglist-span guard now documents that additional read-only
input contract. Forty added no-write cases per architecture cover span starts,
interiors, ends and partial boundary overlaps, comparing every byte of each span
and the complete raw World. They also check the stated current raw ownership.

No speculative adapter or later gameplay World was imported. In particular,
the newer separately owned `current_monuments()` allocation is absent from this
historical World and remains a distinct, held integration requirement. Existing
plan/layer/scene/original pixel outputs remain exact. Native/UI and watched
preview remain untouched.

### Combined road and ordinary overlay ordering (26 October)

The road/tube implementation is unchanged. New combined controls cover its
composition with jungle, forest, fungus, rivers/source, coast, ordinary grid and
base-work outlines rather than only isolated layers. On 128 explicitly modified
owner Worlds, 2,048 complete unchanged original SMAX MapWin returns match every
owned pixel on host and independent big-endian 68040. These cover two source
zooms, clipped placements and distinct visibility/fog profiles. The render
identity, height-cache profile and cursor are supplied controlled source facts.

The existing scene test command also runs 1,296 additional combined synthetic
contexts, retaining all earlier output fingerprints and rejection checks. The
normal SDK probe is byte-identical to its frozen parent. This does not enable
a viewer or close imported sprite/scaling/cache/OS paths; later unsupported
features remain held. See `ROAD-COMPOSITION-EVIDENCE-20261026.json`.

### Sprite scaling and clipping preparation (26 October)

`sprite_scale` owns the 800-entry reciprocal scale table and bounded clipped-axis
plans used by the original RAM sprite path. Complete unchanged original scaling
and X/Y expansion/shrink preparation helpers match every output byte across
1,152 numeric cases on host and independent big-endian 68040. Another 48 axis
contexts use the private sea-platform numeric trim dimensions at the source
11/16 and 21/16 scale profiles, with explicit supplied intersections. These do
not claim original sprite extraction, live placement or complete drawing.

Expansion plans retain clipped replication counts; shrink plans retain source
advances. Invalid/empty ranges, exhausted tables and input/output overlap fail
without writes. Thirty-two boundary/no-write controls run on both architectures.
All 139 inherited workflow commands remain, plus two new scale-control commands.
The normal SDK probe/control compile; native execution is unrun.

The full nonempty `Sprite_draw3` RAM wrapper still calls actual USER32
`IntersectRect`. That import has not been replaced or forced to succeed. Original
loader/extraction, cache/minimum-size and scaled pixel-copy/caller dependencies
remain held. No improvement sprites are enabled by this numeric module. See
`SPRITE-SCALE-EVIDENCE-20261026.json`.

### Scaled sprite pixel-copy boundary (26 October)

`sprite_pixels` consumes explicit prepared axis plans and indexed RAM pixels.
It copies non-key texels using the original shrink advances or clipped expansion
replication counts. The private oracle starts at original instructions after
the OS rectangle boundary, executes the actual clipping helpers and raster
suffix, then the genuine handle-null `Buffer_free_data` and normal suffix
epilogue. It does not execute the complete `Sprite_draw3` entry or replace its
Windows import. Supplied acquired-buffer and intersection facts are explicit.

Every output pixel matches unchanged original suffix instructions for 248
synthetic contexts and 71 private owner sea-platform numeric-trim contexts on
host and actual independent big-endian 68040. Ratios include source 11/16 and
21/16 plus controlled shrink and 2x/3x/4x expansion, clipped edges and padded
pitches. These are bounded indexed-copy results, not complete sprite loader,
physical display, palette or game-scene qualification. Thirty-five transactional
rejections and one positive control cover descriptors, source buffers, partial
aliases, bounds and source pixel-step limits. All parent workflow commands remain
and two commands run the new controls. Normal SDK builds are execution-unrun.

The nonempty full RAM wrapper still needs genuine USER32 `IntersectRect`, while
original extraction, cache/minimum-size and full caller integration remain held.
This branch enables no imported improvement sprites or frontend. See
`SPRITE-PIXELS-EVIDENCE-20261026.json`.

### Acquired-RAM sprite extraction (26 October)

`sprite_extract` accepts an already decoded indexed RAM image, explicit crop and
byte key. It reports compact pitch/dimensions, original full crop dimensions and
trim offsets, copying compact rows into caller-owned storage. Valid empty crops
produce the original empty descriptor without touching pixel storage. Invalid
ranges, aliases and insufficient storage are strict transactional rejections;
legacy invalid/failure mutation fidelity is not claimed.

This now has a whole-entry original observation, rather than only a suffix:
actual `Sprite_extract2` at 5e39a0 executed all real handle-null RAM acquire/release
operations, its genuine no-op `do_sound`, original arena dispatch/bump allocation
and original memcpy. The test supplies a sufficiently large existing pool and an
initially empty Sprite; it executes no pool constructor, backing malloc fallback,
old-Sprite free or OS image loader. No import or callback was replaced. These
facts narrowly supersede older blanket extraction/sound holds for this profile.

All descriptor and compact-pixel bytes agree on host ASan/UBSan and actual
independent 68040 across 216 synthetic cases and five private owner-PCX crops,
including empty crops. The owner's 100x62 sea-platform crop is now confirmed by
the whole original extractor as 72x40, pitch72, offset(14,4), key9; its bytes
exactly match the previously frozen copier input. Forty additional controls cover
transactional errors, partial aliases, output bounds and empty/nonempty success.
All 143 inherited workflow commands remain plus two new extraction commands.
The normal SDK builds are native-execution-unrun. Full USER32 sprite drawing,
physical loader/cache lifecycle and whole-scene integration remain held. See
`SPRITE-EXTRACT-EVIDENCE-20261026.json`.

### Owned sprite placement and draw composition (26 October)

`sprite_draw` joins explicit original caller coordinates, qualified trim metadata,
scale/minimum-size context, owned mathematical rectangle intersection and the
qualified RAM copier. Scaled placement truncates only **after** adding the caller
coordinate; splitting that operation changes positions crossing zero. Missing
source context is typed-held, not replaced with live cache/minimum-size defaults.
Caller coordinates are bounded and original nearest/double numeric scope is
explicit. Inspection results use our typed statuses, not original return codes.

The unmodified original `Sprite_draw3` prefix was actually observed for 60 fresh
or coherent-cache cases. Each process faults at the untouched unresolved
on-disk USER32 import target; a Linux observer captures its original stack and
register state then exits, never returning an API result to original code. All
computed rectangles match on host and actual independent 68040, including eight
cross-zero cases. This is an interrupted prefix observation, not a normal
whole-wrapper return or physical Windows call.

A separate original minimum-size suffix gives 40 genuine normal no-draw returns
and 40 observations stopped at its second unresolved import. Using explicitly
supplied owned intersections, 54 original unscaled/shrink/expand pixel suffix
returns match the owned composition's private owner-image output exactly. The
source prefix and suffix observations remain separate: no fake `IntersectRect`
was installed or forced to succeed. The private pixels are exactly the trim
confirmed by the earlier whole-entry acquired-RAM extraction.

All 432 public synthetic contexts and 46 boundary controls pass host ASan/UBSan
and independent 68040; all 145 inherited workflow commands remain plus two new
ones. Normal SDK builds are native-execution-unrun, including stack/performance
qualification. Live MapWin sprite anchors, cache/minimum-size authority, original
loader/Windows wrapper and full scene/frontend integration remain held. See
`SPRITE-DRAW-EVIDENCE-20261026.json`.


### Explicit sea-platform RAM late layer

`terrain_sea_platform` binds the qualified fixed owner trim (72x40, full100x62,
offset14,4, key9) to the owned sprite drawing composition. It accepts explicit
resolved observation, late visibility, normal source path, scale-cache and minimum
size facts. It does not infer those runtime facts from a saved World. Existing
ordinary scene/frontend behavior is unchanged, including its typed sea hold.

Twelve unchanged original MapWin entry observations actually reach the genuine
sprite prefix and stop at its untouched unresolved USER32 import. These correct
an earlier source-reading mistake: normal textured terrain **restores the original
caller coordinates** before the sea call; its temporary polygon half-width/height
translation must not be added. Raw ocean `MapWin_get_alt` returns zero before
reading a cache. The call follows river/base-work grid and precedes roads.

Nine separately executed original pixel suffix returns with explicit owned
intersections match every owner pixel on host and independent 68040. These are
separate prefix/suffix observations, never full original wrapper returns or fake
`IntersectRect` results. All360 public cases and31 no-write/alias/boundary controls
pass host ASan/UBSan and actual independent 68040. All147 parent workflow commands
remain plus two. Normal SDK builds are native-unrun. Source minimum-size globals
remain explicit, as do prepared owner pixel/cache authority. Shared drawbit20
sea-farm contexts remain held because later crop-yield callbacks are unresolved.
See `SEA-PLATFORM-EVIDENCE-20261026.json`.


### Explicit sea-platform bounded scene composition

`render_world_scene_with_sea_platform` composes the qualified water/feature/grid
prefix and acquired-RAM sea sprite transactionally. It resolves actual displayed
items and the separate late exploration gate from the owned World, retains all
normal parent source-view/height/cache/identity gates, and requires explicit
prepared owner pixels, clip, minimum dimensions and coherent source-scale cache.
Only the normal source zoom-5/5 profile is supported. Original callerXY is retained.

The unchanged parent water renderer is called with bit1 disabled: in this bounded
water-only source path that bit solely enables the late sea sprite; observation,
water, fungus and grid behavior is unchanged. The sprite follows the base-work
grid. Land delegates unchanged. Sea-farm bit20 stays held for unresolved later
crop-yield callbacks; no missing layer is silently omitted. The existing parent
API and frontend remain unchanged, including their original sea-feature hold.

All2304 synthetic water contexts (1536 active/768 inactive sea contexts),1152 land
delegations,12 extra destination/input alias controls plus inherited parent
controls,288 padded-surface cases and5 late-failure/held no-write controls pass host ASan/UBSan and actual
independent68040. Source profile/minimum/cache failures are checked with
the inherited validation precedence before drawing. Every late failure leaves
the original destination unchanged after private terrain staging.

Twelve unchanged original MapWin/Sprite entry prefixes supply actual water and
base-work-grid pixels at the untouched unresolved USER32 import. Nine separate
original sprite pixel suffix returns compose over those actual prefix pixels;
three clipped cases retain the prefix. All12 owner-image results match exactly
on both architectures. This is joined explicit-RAM evidence, NOT a full original
MapWin/Sprite_draw3 return, USER32 execution, original loader or physical OS scene.
All149 parent workflow commands remain plus two. Normal SDK builds are native
execution-unrun. See `SEA-SCENE-EVIDENCE-20261027.json`.


### Sea-farm crop-call and remaining sprite intent

`inspect_sea_late` is an inspection-only intent plan, not a yield evaluator or
renderer. It preserves the actual required source crop call and orders the
supported later water item sprites. It never removes the scene's sea-farm bit20
hold. Caller supplies actual observation/late gate, CurrentFaction, using-faction
and runtime ExpansionEnabled. Land, special modes and landmark/bonus/goody paths
remain held.

The original sea-platform bit1 draw precedes the road boundary and the crop
call. Crop is called when bit20+displayed farm8000+sourceZoom>-8, or flag1000
forces it regardless of zoom; both remain behind the late exploration gate.
Its actual arguments are `(resolved faction,-1,mapX,mapY,0)`. A negative viewer
uses raw using-faction or actual CurrentFaction; a nonnegative viewer uses itself.
At sea the return is not used for land farm icons, but the real call and scratch
side effects cannot be silently skipped or replaced with zero. No real scene
CurrentBase/ControlUpkeep/yield-scratch authority is inferred from a controlled
leaf harness or substituted base0.

Six hundred unchanged original MapWin prefixes confirm first call/arguments and
source sprite selection:112 stop before crop execution,88 before Sprite_draw3,
29 before Sprite_draw_dest, and371 genuine no-event normal returns preserve
stack/SEH/RAM-buffer state. Deliberately unmapped actual callee entries trigger an
observer that exits without returning any fabricated result. Later sprite assets
are unacquired and no callee pixels, USER32 or original crop result is claimed.
Future ordering after a required crop remains source-instruction intent, not an
executed callback chain.

All25926 public plans and24 no-write/alias/mode controls pass host ASan/UBSan and
actual independent68040. The600 matching first-event/crop-argument plans also
agree on both. All151 parent workflow commands remain plus two. Normal SDK probe
and controls build only; native execution unrun. See
`SEA-CROP-BOUNDARY-EVIDENCE-20261027.json`.


### No-crop acquired-RAM later water sprites

`render_water_item_sprites` is a separate late RAM layer for the original water
item10,40,400000,800000 and1000000 sprites (including the last one's alternate
landmark variant). The six source TER1 crop rectangles and exact trim profiles
are recovered from loader instructions and complete original RAM extraction.
Owner pixels stay private; no replacement art is generated. Physical PCX loading
and global palette installation are not claimed.

Every required crop event holds BEFORE any writes. The sea-platform operation
and highbit Sprite_draw_dest/cache route are separately held rather than omitted.
Caller supplies original observation/late gate, ExpansionEnabled, normal zoom-5/5,
callerXY, clip, minimum dimensions and coherent scale-cache/resource authority.
All active resources must match their qualified trim profile. Failure rolls back
all earlier sprite writes; inputs/resources cannot alias the original destination.
Padded surfaces preserve padding. No World, parent scene or frontend is enabled.

All720 synthetic packed/padded/order/gate cases and36 boundary/alias/rollback
controls pass host ASan/UBSan and actual independent68040. Six owner crops match
complete original Extract2 metadata and every pixel. Forty-eight owner RAM
contexts use actual MapWin-prefix water/base-grid pixels,96 separately executed
original Draw3 geometry prefixes, and separate real original raster suffix waves
in source intent order. Every final pixel matches on both architectures. This is
joined RAM evidence, NOT a complete original callback/USER32/MapWin wrapper run;
no original wrapper is resumed with a fabricated result. All153 parent workflow
commands remain plus two. Normal SDK builds are native-unrun. See
`WATER-ITEM-SPRITES-EVIDENCE-20261027.json`.


### Owned-World no-crop water-item scene composition

`render_world_water_item_scene` joins the qualified ordinary terrain and sea-
platform prefix with the six acquired-RAM later water sprites, in original
source order. It resolves displayed items, raw landmarks, altitude, using-faction
and the separate late visibility gate from the owned World. CurrentFaction,
ExpansionEnabled, original render identity/cache/height/scale and all resource/
clip/minimum authority remain explicit runtime inputs. There are no defaults
pretending to supply original runtime provenance.

Every required crop call and highbit DrawDest operation still holds before any
writes. Even water invokes the original bit20 base query before its land-only
road draw guard: stale-base repair therefore stays held. Only after validating
that boundary is bit20 split from the qualified water prefix; bit1 sea-platform
pixels precede later items. Land delegates unchanged to the parent scene. All
input/resource aliases are rejected against the original destination before
full-pitch private staging. Failures roll back all layers and preserve padding.

All2304 public cases,192 padded cases,18 extra alias controls and5 explicit
late-failure/hold controls pass host ASan/UBSan and independent68040. Forty-eight
controlled owner-World contexts match every pixel of frozen genuine original
MapWin-prefix buffers plus separately observed Draw3 prefixes and real raster
suffix waves. This is source-ordered joined RAM proof, not a whole returned
MapWin/USER32/import/cache call chain. All155 inherited workflow commands remain
plus two. The normal SDK probe builds; native execution and frontend enabling
remain unrun/held. See `WORLD-WATER-ITEM-SCENE-EVIDENCE-20261027.json`.


### Graphics crop(-1) runtime recovery: hold retained

The original normal water caller passes the resolved graphics faction, base-1,
mapXY and flag0. The crop gate sees displayed/snapshot farm bits, while the leaf
rereads raw actual tile data. A negative viewer resolves to raw using-faction
when nonzero, otherwise explicit CurrentFaction; territory-overlay modes remain
excluded because they can overwrite that working faction.

A substantive SMAX dependency remains: its ocean-kelp branch tests facility36
without guarding the negative base index. With base-1 the unchanged instruction
reads byte97cf9c (immediately before Bases, physically Factions[7]+2028) bit10.
This is not base0 facility state, and constructed owner realms do not establish
live-runtime authority for that byte. Seven source-entry profiles are sensitive
to this byte; the returned yield can stay2 while the scratch effect changes.

The leaf accumulates BaseTerraformReduce without resetting it.672 controlled
complete original crop returns, using112 genuinely observed graphics-entry
argument profiles, match explicit nutrient/technology/runtime/alias contexts.
Another672 paired calls confirm repeated accumulation. Real original bonus,
bitmask and technology helpers execute; no callback/import result is fabricated.
Nonzero CurrentBase/id, ControlUpkeep and energy-scratch canaries stay unchanged
on this profile. These are separate numeric helper returns, not a resumed
MapWin callback chain. There is no portable evaluator, renderer crop-hold removal,
base-edition or native qualification here. All prior sources remain unchanged.
See `GRAPHICS-CROP-CONTEXT-20261027.json` for the exact contract and limits.
