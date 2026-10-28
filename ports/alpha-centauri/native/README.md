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

Original ordinal facility/project definitions are parsed with effective free-tech
kept disabled as in the original loaders: [scope](FACILITIES.md),
[evidence](FACILITY-EVIDENCE-20261025.json). No production eligibility or UI is
implied, and actual AmigaOS qualification remains pending.

Pure original facility availability over explicit facts is described in
[FACILITY-AVAILABILITY.md](FACILITY-AVAILABILITY.md). It is separate from placing
production orders or advancing the game; native execution remains held.

[WORLD-FACILITY-OPTIONS.md](WORLD-FACILITY-OPTIONS.md) binds those predicates to
actual saved bases/factions/projects/technology and neighbouring map tiles. This
read-only query grants no production-order authority; native execution is held.
Original production switching has a read-only classification/loss preview bound
to saved base facts: [scope](PRODUCTION-RETOOL.md),
[evidence](PRODUCTION-RETOOL-EVIDENCE-20261025.json). It does not place orders;
actual AmigaOS qualification remains pending.

[LOCAL-PRODUCTION-SESSION.md](LOCAL-PRODUCTION-SESSION.md) combines original
facility options and retooling in an owning offline local controller with private
revision-bound confirmation plans and atomic live queue/mineral/counter changes.
No frontend, turn or player-save interface is added; native execution is held.

[PRODUCTION-SNAPSHOT.md](PRODUCTION-SNAPSHOT.md) adds a separate, schema-sealed
buffer encoder for facility-order state. It preserves the original container;
filesystem/player save controls and native qualification remain pending.

### Restricted owned-world step integration (draft, not exposed to input)

`WorldActions::clear_land_step` joins the qualified speed, edge-price, movement
budget and transactional stack-storage paths. It changes owned runtime vehicle
coordinates, stack/tile occupancy, movement expenditure, moved flag and facing
only after all guards succeed. Original wire bytes remain the baseline snapshot;
this is not a save writer or replacement for original `order_veh`.

Supported input is deliberately limited to ordinary base-game human land units,
no special/granted abilities or standing orders, fully seen local terrain,
cleared non-ocean destinations, and no nearby foreign units/bases or territory.
Pod encounters (including source-derived generated pods), fungus, monoliths,
contact/combat/ZOC, wake/transport states, fractional attempts and expansion
mechanics are deferred. FREEABIL checks cover both vehicle and design owners.
No discovery/event callbacks, upkeep, artificial next turn or fake AI is added.

`test_world_actions.py` builds synthetic original containers and checks 46
boundary/sequence cases, including full decoded-state no-write checks
on rejection, runtime RNG preservation, untouched wire bytes and successful
stack validation. A cumulative round-trip consumes six movement points, then
rejects the next step without writes. Host address/undefined sanitizers pass. A private copied-data
probe resets the baseline for each of 160 original tutorial edges and supports
45 steps (Explore2/Build15/conquer28); all original source files remain unchanged.
This is an integration control, not a full original executable differential.
The normal SDK probe compiles, but actual AmigaOS and independent big-endian
qualification remain pending. Do not expose this draft API to player input yet.

```
python3 ports/alpha-centauri/native/test_world_actions.py --out /tmp/world-actions-host
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_world_actions_probe.sh /tmp/world-actions-probe

```

### Movement snapshot buffer encoder (draft)

`encode_movement_snapshot` preserves the complete owned original plain SC/SAV
container and every opaque byte, while encoding runtime tile and vehicle records
with explicit little-endian fields. It validates stack topology and references
before the first output write. Unsupported changes to prototypes, bases, faction
state/definitions, preferences, map metadata or UI state cause rejection; the
encoder cannot silently save their stale original copies. Output and receipt
remain unchanged on errors, including short/aliased buffers and invalid stacks.

This is a bounded movement-container encoder, not an in-game save command,
filesystem transaction, original executable save-compatibility proof or complete
campaign persistence. It does not create/replace player files or assume SDK
`O_EXCL` works. The probe writes only caller-selected test evidence files. The
original tick footer is preserved and never substituted for runtime RNG; the
current explicit RNG seed is returned separately in the receipt for the caller
to retain using a separately qualified persistence format. No struct dump or
custom single-scout codec is used. Player-facing save/load remains unavailable.

Host ASan/UBSan checks pass 93 synthetic cases over wire versions7..12, including
byte-exact unchanged output, independently computed storage edits, reload, and
transactional rejection. An additional42 private original-tutorial cases pass
(135 total), with source data untouched. The evidence probe rejects an existing readable target,
including its input; this guard is not an atomic replacement protocol. The normal SDK probe compiles, but
actual AmigaOS and independent big-endian qualification are pending. Do not
expose this draft encoder to user save operations before those gates pass.

```
python3 ports/alpha-centauri/native/test_movement_snapshot.py --out /tmp/movement-snapshot-host
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_movement_snapshot.sh /tmp/movement-snapshot-probe
```

### Saved faction speed grants (draft, native gate pending)

`world_speed_ability_flags` retains base raw-only abilities and supplies the expansion design owner's eligible FREEABIL speed bits to vehicle speed and land pricing. [Scope and controls](SPEED-ABILITIES.md) distinguish this change from historical native evidence and from legal player actions. [Evidence](SPEED-ABILITIES-EVIDENCE-20261025.json) records host/source/helper checks and pending native qualification.

[LOCAL-ACTIONS.md](LOCAL-ACTIONS.md) connects the strict clear-land action and
facility orders through one owning session/revision, and adds a combined
original-container buffer snapshot. Exhaustion is preserved across reload;
frontend/player save/turn/AI and native qualification remain pending.

## Original fresh tile yields

`tile_yields.h` adds checked original nutrient/mineral/energy calculations from a
decoded tile and explicit current-stage facts, plus map-seed resource placement.
It preserves edition differences and returns the original counter/scratch effects
as data. Saved base intake is not an input. No world stage, worker allocation,
turn or frontend is wired to this API. See [TILE-YIELDS.md](TILE-YIELDS.md) for
required facts, original behavior, numerical controls and remaining native gates.

## Owned-world tile-yield binding

`world_tile_yields.h` derives the tile/resource, physical facilities, technology,
project, faction and adjacent mirror facts from an owned original save snapshot.
The caller supplies the actual runtime edition and transient stage state. It
returns a checked result without changing the world or runtime RNG. This is a
snapshot query, not worker allocation or upkeep. See
[WORLD-TILE-YIELDS.md](WORLD-TILE-YIELDS.md).

## Original worker-radius availability

`world_worker_radius` derives the original 25 availability flags and 21-bit
unavailable mask from owned world records. It retains original visibility,
territory, convoy/hostile-stack and neighboring-base reservation behavior.
The explicit normal inspection stage is supported; editor and AI-goal stages
reject. This does not select workers or execute upkeep. See
[WORLD-WORKER-RADIUS.md](WORLD-WORKER-RADIUS.md).

## Fresh owned terraforming ecology

`world_terraform_ecology` computes raw terraforming damage from actual radius
items, physical Tree Farm/Hybrid Forest and the current owned worked-tile mask.
It preserves negative forests and per-tile signed reductions. It does not read
saved ecological or intake totals, allocate workers or execute upkeep. See
[WORLD-TERRAFORM-ECOLOGY.md](WORLD-TERRAFORM-ECOLOGY.md).

## Fresh intake from an existing human worker assignment

`world_stable_intake` composes actual satellites, fresh selected-tile yields and
signed raw ecology without feeding back saved intake. It supports an already
valid directed-human assignment with explicit transient stage facts. Unsupported
allocation, specialist and fungus branches reject. The owned world remains
unchanged; no upkeep or player action is added. See
[WORLD-STABLE-INTAKE.md](WORLD-STABLE-INTAKE.md).

## Original nutrient accounting after fresh intake

`base_nutrients` calculates checked original growth scratch, food consumption,
surplus and stored-food adjustments. `world_fresh_nutrients` binds it to actual
owned records and fresh stable intake, with explicit convoy facts. The same
normal worker inspection stage is retained; no upkeep or population growth is
executed. See [BASE-NUTRIENTS.md](BASE-NUTRIENTS.md).
## Normal human research pool (draft, native unrun)

`research_pool.h` queries the original directed-human candidate pool using the
PR86 owned eligibility API. It preserves original category ties, global first
offer, modulo-three ranking rotation and starting-ID exceptions. Blind, AI and
turn-zero paths explicitly require automatic selection; this addition does not
implement that selection, discovery awards or turn advancement. No choice
controls are exposed in the frontend. See `RESEARCH-POOL.md` and
`RESEARCH-POOL-EVIDENCE-20261025.json` for source and qualification limits.

CI runs 115 raw contexts plus two invalid snapshot controls and 64 owned worlds
(512 faction queries) on both host sanitizers and independent 68040. Normal SDK
raw/world builds pass; actual AmigaOS execution remains unrun and is a merge
gate. This stacked draft depends on PR86 and preserves separate PR89 unchanged.
