# Alpha Centauri native rules-reader prototype

The [complete explicit energy composer](BASE-ENERGY.md) now combines both loss
calculations, specialist output, bonuses, penalties, stockpiling and psych.
Commerce, technologies and current-world facts still require caller bindings;
no player accounting mutation or turn is added. Full CI and AmigaOS execution
remain pending.

The separate [five-phase psych proposal](BASE-PSYCH.md) now has host and
independent68040 controls plus private original-code comparisons. It remains
unconnected to player input, World accounting mutation and turn processing;
AmigaOS execution and full CI are pending.

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
Original per-base unit mineral support is a read-only aggregate with separate
[scope and controls](UNIT-SUPPORT.md) and [evidence](UNIT-SUPPORT-EVIDENCE-20261025.json).
Actual AmigaOS qualification remains pending; this does not execute upkeep.

### Mineral/ecology numeric stage (draft, native gate pending)

`base_minerals.h` exposes explicit pre-stage numeric accounting without mutating a world or advancing turns. [Scope and original instruction corrections](MINERAL-ACCOUNTING.md) and [evidence](MINERALS-EVIDENCE-20261025.json) distinguish the completed host/original-helper checks from pending native execution. Saved post-calculation intake is not a fresh yield input.

Fresh mineral composition is documented in [WORLD-FRESH-MINERALS.md](WORLD-FRESH-MINERALS.md). It returns a bounded read-only proposal from fresh intake, actual convoys and pending support; native qualification remains UNRUN.

### Fresh technology-effect tile/intake binding (local candidate)

[WORLD-FRESH-TECHNOLOGY-YIELDS.md](WORLD-FRESH-TECHNOLOGY-YIELDS.md) connects the complete read-only technology-effects stage to original fungus leaves and stable/convoy nutrient proposals. Saved snapshot tile APIs remain unchanged. No cache/award/accounting commit or turn is added; native and publication remain held.

The separate checked [energy inefficiency component](ENERGY-INEFFICIENCY.md) returns numeric loss/diagnostic proposals from explicit facts. Full owned energy accounting, CI and native qualification remain pending.

[Plain energy allocation](ENERGY-ALLOCATION.md) preserves both original loss calculations and pre-commerce faction accumulation as explicit numeric proposals. World commerce/bonus/psych binding and all accounting commits remain pending; native qualification is UNRUN.


### Owned offline base accounting refresh (local candidate, native held)

`load_local_accounting` explicitly imports a new offline local session with its
own unit, resource and citizen definitions parsed from the same rules text.
Production-only imports do not gain accounting capability, and accounting imports
do not enable movement. Admission retains the existing edition, selected human,
active saved faction and rules validation. A failed import preserves the previous
session. The capability is not wired to player input.

`prepare_accounting` binds a selected owned base and complete resource context to
the session lifetime and revision. `commit_accounting` recomputes the result and
atomically updates the 23 declared base values in `local_accounting_fields.inc`.
Suspension, successful reload, another changing command or address reuse makes
an old plan unusable. Unsupported allocation and arithmetic failures reject
without a partial patch. An unchanged repeated refresh preserves the revision.

This refresh accepts only projection stage0. It updates second intake/accounting
fields, food sign normalization, ecological damage and happiness, while preserving
first intake fields used by the explicitly selected commerce phase. It does not
apply faction turn totals, projections, support actor flags, worker reassignment,
production completion, growth, research awards or turn progression. The inherited
stable worker domain accepts coherent existing human specialists with adjustment0,
and still rejects arrangements needing reassignment and automatic governors. An explicit
alien scratch policy remains required; the default rejects undefined original
paths, with no implicit zero-initialization policy selected by this session.

`encode_local_accounting_snapshot` is a separate buffer encoder. It preserves the
whole supported original container and combines the existing bounded movement /
production wire scope with the explicit base accounting fields. Legacy BASE
fields at modern offset0x98 and later shift by four bytes. All other decoded base
and faction data are sealed; every validation precedes the first output write.
Runtime RNG is returned separately from the unchanged footer tick. This supplies
neither a filesystem save protocol nor evidence that the original executable can
resume the encoded container. Changed binaries still require actual AmigaOS
execution and normal exit before deployment or any native qualification claim.


### Existing directed-human specialists (local candidate, native held)

Fresh intake now accepts the original stable arrangement when selected first21
worker tiles include the centre, all selected non-centre tiles are available,
and worker count plus specialist count equals population plus one. Specialist
count must be within0..population; a positive count requires adjustment0. This
preserves an existing arrangement and does not choose workers or turn governor
allocation into a successful fallback.

Private unchanged full original base_yield execution in both editions confirms
this path, including populations through127, centre-only arrangements, up to21
workers, and positive specialists above16. Every seeded global and base byte is
compared against the independent intake model; packed specialist types remain
unchanged. The owned full energy path separately validates and computes actual
specialist spending/obsolete replacement, and the existing session snapshot
persists its packed fields. The dedicated specialist transaction controls cover
fresh resources through income/happiness, stale-plan gates and full-container
reload against the independent wire models. Actual AmigaOS execution remains
unrun. Automatic worker reassignment, AI and complete turns remain unfinished.


### Original human worker selection core (local candidate, native held)

`human_worker_allocation` reconstructs the original directed-human branch with
no governor priority weights. It takes explicit current yields for the ordinal
21 tiles, unavailable mask, satellite contributions, population, requirement,
mineral maintenance, queue item, existing workers/specialists, packed specialist
types and actual prerequisite availability/spending of seven citizen rows.
It returns a selection proposal. It does not call fake source callbacks, infer
live authority from a save or mutate an owned world.

The core retains existing available workers, fills missing slots using the
original score and strict first-best ties, and preserves the original9999 worst
sentinel. If the original starvation condition reaches the exchange branch, it
attempts one swap and retains it only with the original population+1 food
margin. Missing useful tiles become specialists with the original prerequisite-
only fallback and packed first16 fields. Original project-weight thresholds
are edition specific: strictly below-39 for SMAC and below-70 for SMAX.

Population is checked within0..127, resource requirement1..99, satellite values
within0..population and initial counts within the population budget. Active
governor/AI priorities, damaged/overfull arrangements and specialist adjustment
need separate original binding/fixup contracts. All source signed32 intermediate
operations are checked. Failure and input/output overlap preserve the result.

Synthetic controls compare the raw core with an independent integer model on
host and independent68040. Private complete original base_yield observations
also compare selected masks, counts, packed types and intakes, plus the seeded
0x940000..0x9e0000 globals/base region, using real original farm/yield/technology
helpers. Those observations do not independently qualify the transient current
scratch page or a full original energy/psych/turn pipeline. No World/session
allocation commit, worker persistence extension, player input, full governor,
upkeep/AI/game or actual AmigaOS execution is supplied by this component.

### Human worker yield-call sequence (local qualification)

`human_worker_allocation_traced` returns the same selection proposal plus a
bounded ordered record of candidate/add/remove tile visits. Candidate visits
represent direct nutrient/mineral/energy calls; add/remove visits represent
resource channels 0 through 3, including the scratch reset. Rejected exchanges
retain the remove-new/add-old visits even though their intake change is reversed.
Both outputs remain unchanged on invalid facts, overflow or overlapping storage.
The original untraced API keeps its existing behavior.

The trace controls execute 2,424 synthetic cases on the host and independent
Linux m68040, including 162 rejected exchanges. Private complete original helper
runs also compare externally logged yield-function entry order and final scratch
words. Function order is observed directly; tile ordinals and add/remove direction
are checked against the independent selector model and final state, without
claiming that original stack arguments were observed. This boundary does not yet
bind world facts, execute yield leaves, change workers, persist assignments,
advance a turn, or qualify an AmigaOS executable. The complete workflow and
native execution remain unrun. Build the isolated native trace probe with
`build_human_allocation_trace_probe.sh`; run public controls with
`test_human_allocation_trace.py`.

### Manual human allocation from an imported world

`world_human_allocation` binds the original manual-human selector to the owned
world's actual first21 tile availability, fresh technology/yields, physical
facilities/projects, capped satellites, unit-mineral support, current production
item and prerequisite-qualified citizen bonuses. It accepts explicit worker
inspection/upkeep and transient scratch values. Active governor scoring, specialist
adjustment, malformed arrangements and mismatched editions are rejected.

Coherent retained arrangements preserve real governor bit31: the original exits
before consuming priority values. Missing-worker arrangements with bit31 remain
unsupported; no flags are cleared or priorities synthesized.

The proposal replays candidate and applied/removed tile visits in their original
order, including reversed exchanges and resource channel3 resets. It checks
unchanged numeric yields, signed overflow, the final intake, scratch values and
ocean-worker flags. It preflights all available tiles, so an overflowing tile
can conservatively reject a proposal even if selection would never visit it.
Work buffers use bounded private heap storage. All world data, tables and RNG
remain unchanged, and failure preserves the output. No callback supplies yields
or authority. This does not commit workers, refresh full base accounting, persist
assignments, execute a turn or expose player input. Native execution and full
workflow/CI remain unrun. Public wire-model controls are in
`test_world_human_allocation.py`; the isolated normal SDK build helper is
`build_world_human_allocation_probe.sh`.

### Owned worker and accounting transactions (local qualification)

Accounting-enabled offline sessions can prepare and commit a
`WorkerAccountingPlan`. Preparation allocates a private copy of the current
movement/order/accounting union, computes actual worker selection and fresh
first intake, then evaluates complete stage0 base accounting on that coherent
copy. Commit recomputes the private plan against the same session identity and
revision before applying one base patch. Failed calculations preserve the live
world and output; suspension, reload, address reuse and changes by another action
invalidate prior plans. An unchanged refresh keeps the revision unchanged.
Temporary wire/decoded-world/plan allocations have an explicit work budget;
called components retain their bounded internal scratch allocations.

`load_local_worker_actions` explicitly imports the combined base-game bounded
land-action and accounting capability from the same private rule tables. Existing
accounting-only imports still disable movement. Expansion movement remains
unsupported. The combined controls perform two bounded moves, three facility
orders, worker/accounting commits, export/reload and continued exhausted-movement
checks against an independently patched complete container. No turn refill is
performed. The frontend does not instantiate these capabilities.

`encode_local_workers_snapshot` separately encodes the existing movement/order/
accounting union plus worked mask, specialist count/adjustment, first intake,
unused intake and worker state bits. It seals all other decoded fields and all
state bits outside 0x8100; changed worker counts must remain coherent. Explicit
little-endian writes preserve opaque original bytes and the footer; runtime RNG
is returned separately. Frozen accounting/action codecs remain sealed to their
original field sets. These are buffer transactions, not a qualified filesystem
save, original-executable compatibility claim, full upkeep or playable game.
AmigaOS execution and complete final-head CI remain unrun.


Original unit production pricing is a separate checked dependency. Its explicit
facts distinguish stored design cost from the no-abilities component base cost,
and use the design owner's difficulty and FREEPROTO flag. Prototype history and
physical Skunkworks determine the surcharge; Skunkworks clears it only after the
original intermediate calculation. Expansion-only Brood Pit pricing applies to
predefined native designs; the base edition has no such discount. A producing
base raises zero-cost colony designs to one before adding the surcharge.

The pure API accepts current facts; it does not infer them from a saved world.
`production_mineral_requirement` multiplies an already-qualified nonnegative
unit/facility price and mineral factor, preserving original zero factors and
rejecting signed overflow. Neither API grants build eligibility, advances
minerals, creates a unit, completes a facility, refills movement or runs a turn.
29,076 independent controls passed on the host with address/undefined-behavior
checking and on independent big-endian 68040 Linux. Bounded executions of both
original complete unit pricing routines, including their actual component-cost
and bitmask helpers, matched 5,120 base and 4,579 expansion contexts. Original
executable bytes and all private reference material remain outside this tree.
Normal Amiga SDK builds are compilation evidence only; AmigaOS execution and
complete final-head CI remain unrun. No new frontend controls are exposed.


The current-world human production cost query binds the separate pricing
calculation to the owned save records. The producing base owner's pending
Industry and saved human mask determine the original human mineral factor;
planet size comes from the saved map settings. Unfinished custom prototypes use
the design owner's difficulty and FREEPROTO flag, the no-abilities component
base price, and the producing base's physical Skunkworks. Expansion predefined
native prices use the producing base's physical Brood Pit. Already prototyped
custom designs retain the original short circuit past unused component fields.

Both original editions and SC/SAV versions7..12 are supported within this human
pricing scope. Facility IDs, current rule costs and signed overflow are checked;
negative facility costs are unsupported. Zero original mineral factors remain
zero. AI cost factors and runtime edition conversion remain unsupported. Output
overlapping the world or input tables rejects, and failures preserve output,
world records, opaque bytes and runtime RNG. This query grants no build
eligibility and performs no accrual, spending, creation, completion or turn.

The exact original human-factor source and its controls are imported unchanged
from the frozen factor candidate. Independent host and big-endian controls cover
current-world prices and malformed/unused fields. Private bounded original
human-factor and unit-price routines are exercised sequentially with facts from
controlled worlds and the owner's three tutorials; complete original production
or save fixup is not executed. Normal Amiga builds do not qualify AmigaOS
execution. Full final-head CI and native frontend integration remain unrun.


The pre-production arithmetic prefix is a separate checked calculation. It
clears only the production-done state bit and updates the faction's current phase
mineral peak before checking negative accumulated food. That abort preserves
mineral stock. Otherwise nonnegative surplus accrues only without active drone
riots and outside Stockpile Energy, whose original item differs by edition.
Every signed intermediate is checked, including doubled surplus; failure leaves
the result unchanged instead of exposing an unsafe partial update.

The world adapter reads current base accounting and the current faction phase accumulator. It does
not certify that saved/current accounting is fresh and does not recompute it.
The query preserves all owned wire and decoded records and runtime RNG; output
aliases and edition/index errors reject. Its result is not a phase token, once
per turn guarantee, accrual commit, production reset, threshold/completion,
creation, queue or project effect. The frontend has no input binding for it.

Independent host and big-endian controls cover the arithmetic and current-world
binding. Private observations execute the original numeric prefix after its UI
constructor and stop before cleanup or the next production stage, using two
observation jumps outside the numeric window. Whole base bytes and all eight
faction records match the independent expected state. No original full
production or turn routine is executed. Normal Amiga SDK builds remain
compilation evidence; actual AmigaOS execution and complete final-head CI are
unrun. Explicit phase ownership and complete production effects are still
required before any future mutation or End Turn control.


The ordinary-facility settlement/install prefix is a checked pure dependency.
For IDs2..33 in the base edition and2..64 in the expansion, it requires absent
physical ownership of that facility and a nonnegative mineral requirement no
greater than the current stock. It pays that requirement, caps human excess
using the original surplus/retool limit, snapshots the remaining stock, updates
the original production flags and installs the physical bit. Other bitmap bytes
and state bits remain unchanged. Headquarters, satellites and projects are
outside this API. An already-present bit or unsupported input rejects without
changing the result.

This is a prefix, not full facility completion. Network Node8 still requires its
artifact-linked state and actor-order effects. Queue advancement/eligibility,
notifications/audio, choice/reset, the surrounding upkeep sequence and all
remaining effects must be implemented before a completion or turn commit. No
live World mutation, player control or runtime phase authority invokes this
calculation. Bounded original observations stop before those later effects and
compare every current-base byte with an independent model. The owned host and
big-endian results reproduce those byte patches. Amiga SDK builds remain
compilation evidence; native execution and full final-head CI are unrun.


The original outer production phase resets `best_mineral_output` to zero before
iterating that faction's bases. This field is a per-phase maximum, not a lifetime
record. The arithmetic prefix still computes max(supplied current accumulator,
current base output); its future phase owner must perform that reset once, along
with the other original faction-level initializations and effects. Earlier
“historical peak” wording described the supplied accumulator too broadly.

### Structural production queue iteration (local, unqualified on AmigaOS)

`queue_shift` reproduces one original structural iteration before either real
availability predicate. The saved `queue_size` counts waiting entries after the
current item: supported sizes0..9 address ten stored slots. Size0 is unchanged.
For positive size the old nonnegative unit ID's uint8 queue counter decrements
with wraparound, items1..size move left, and size decrements. Remaining tail slots
are preserved exactly. Old positive IDs outside0..511 and invalid sizes reject
without changing output; negative IDs remain opaque and are never negated.

This does not determine eligibility, retry unavailable candidates, invoke project
speech/drawing, complete production or mutate an owned session/World. No player
input invokes it. Actual original-code observations enter each queue routine and
stop before the first availability branch for nonempty queues; empty queues use
the actual normal return. Full current-base and all eight faction records match
independent expected bytes in330 contexts per edition. Owned results match those
observations on both host and independent68040 builds. Synthetic control suites
cover21,724 cases including empty queues, counter underflow and malformed input.
The normal SDK binary builds only; AmigaOS execution and full final-head CI remain
unrun. Full queue advancement still requires real candidate admission and all
source-backed completion effects.

### Original unit availability (explicit facts, local only)

`unit_availability` checks original unit admission from explicit qualified facts:
active flag, faction obsolescence mask, predefined prerequisite technology,
scenario colony-pod prohibition, coastal requirement for a sea chassis with a
base, and faction ownership of custom design IDs. Prerequisite normalization
retains faction0 rejection, None(-1), original ordinary technology range0..87,
row prerequisite sentinels and technology ownership. The raw checked API accepts
unit IDs0..511, factions0..7 and triads0..2; failure leaves output unchanged.

Original SMAC has no expansion-component filtering. Original SMAX additionally
rejects expansion armor/weapons/ability bits and predefined units14..20 whenever
its explicit runtime expansion setting is false. That setting is not guessed
from the executable/wire edition. Any bit in0x1f000000 rejects in that SMAX mode;
a later source's equality-based ability formulation is not copied.

Host and independent68040 suites pass31,520 cases. Complete unmodified original
routines plus actual coast helpers/original radius offsets match14,788 SMAC and
14,856 SMAX controlled contexts. The coast fixtures use an8x8 map, base(2,2),
all land or one adjacent water tile, with actual map globals; this is not a full
original World load or a general terrain qualification. An earlier additional
set tests the full routines with no base, bypassing the coastal branch.

The API has no owned World binding, authority, queue retry loop, production
commit, completion, actor creation, turn or frontend input. Normal Amiga SDK
builds are compilation only. Final-head CI and actual AmigaOS execution remain
unrun; do not expose this query as restored gameplay.

### Owned-world unit admission (read-only, local only)

`world_unit_availability` binds the checked original leaf to the owned saved
world's512 prototypes, active/obsolete flags, plan/component bytes, raw abilities,
scenario rules and technology owners. Parsed technology prerequisites and chassis
triads are used directly. Sea units with a base derive adjacent ocean access from
actual tile climate bytes and the eight original neighbors, with horizontal wrap,
flat-map bounds and vertical bounds. Base=-1 skips chassis/coast lookup as the
original global query does. Invalid chassis references conservatively reject a
base-present query, even if an earlier availability gate could have rejected it.

Runtime edition must match the loaded wire edition and definition table profile.
ExpansionEnabled remains explicit, including SMAX with expansion mode disabled;
original SMAC ignores that flag. Factions0..7 and cross-owner base queries retain
the original predicate semantics and grant no action authority. Aliases into the
World object, owned decoded arrays, original container, rules or definitions
reject before writes. Failure leaves the output unchanged. Full decoded/opaque
World bytes, definition tables and runtime RNG stay unchanged through the suites.

The corpus covers wire versions7..12, all512 designs/factions, global and base
queries, both explicit expansion states, invalid inputs, inactive/obsolete/custom
ownership, prerequisite sentinels, corrupt unused chassis and map edges under
flat/wrapped geometry. Private runs also include the owner's three tutorials.
The original routines/coast-helper differential evidence belongs to the unchanged
parent leaf; this binding is tested against an independent wire-based model and
is not execution of an original saved-world load or production turn.

No queue mutation/retry loop, unit creation, owning session/phase authority,
completion, save command, End Turn or frontend input is added. SDK compilation
and local host/independent68040 checks do not qualify AmigaOS execution or full
final-head CI, which remain unrun.

### Original world production queue retry proposal (read-only, local only)

`world_production_queue` composes the unchanged structural shift and actual World
unit/facility admission predicates into the complete bounded retry portion of
original `base_queue`. It copies the base queue and owner's512 unit counters,
shifts, checks the new item and repeats until admission or exhaustion. Every
removed nonnegative ID decrements its byte counter with original wraparound;
unused tail slots remain exact. Facility admission uses original queue_count0,
so waiting entries do not supply hypothetical prerequisite facilities. No
callback booleans or fake original predicates are used.

The proposal reports final queue/counters, iteration count, admission and whether
the original project-initiation notification is still required. It never invokes
or fakes audio/draw, installs a facility, creates a unit, commits queue state or
grants session/phase authority. For post-completion use the actual World must
already reflect completed installation/creation and its other effects; this
query does not fabricate that state. Original saved queue sentinel-1 is rejected,
as are faction0, invalid IDs/references and mismatched profiles. Errors preserve
the entire output. SMAC expansion-enabled profiles reject conservatively to match
the existing facility predicate contract.

Host/independent68040 controls include mixed unit/facility queues, all queue sizes,
exhaustion, repeated IDs and byte-counter underflow, project obligations, invalid
future candidates, malformed references, alias guards, original tutorials and
unchanged World/table/opaque bytes/RNG. Additional private actual-original queue
observations execute the real unit predicate for unit-only queues, stopping only
after admission before audio/draw; empty/exhausted paths return normally. This is
not a full original facility queue loop or full production completion proof.
Normal SDK compilation, local controls and bounded helper observations do not
qualify final-head CI, actual AmigaOS execution, a full turn or playable game.

### Offline Network Node artifact effect proposal (read-only, local only)

`node_artifacts` describes the original post-install Node8 effect: clear base
state bits0xc00 and set every plan12 artifact's order to0 in the actual tile
stack. Original code has no actor-faction ownership check here; foreign artifacts
are included. The mask includes already-order0 artifacts, while changed_orders
reports only orders that would change. Empty stacks still clear the base flags.

This requires a physically installed Node, an explicit owned offline runtime
policy and coherent reciprocal stacks/vehicle-present map bits. Offline mode is
not inferred from serialized flags or presented as an observation of a running
original game. Original DirectPlay stack_fix repair and veh_at error/logging/bit
rebuild paths remain unsupported. The proposal conservatively rejects corrupt
stacks anywhere in the world instead of repairing or executing those paths.

The result is bounded to32767 actors using a4096byte mask. Input/output aliases,
wrong edition, network runtime, absent Node and corrupt topology/bits reject
before any output write. Full owned/opaque World bytes and RNG remain unchanged.
Host/independent68040 tests cover198 worlds, shuffled stacks, empty/two-location
stacks, mixed factions, artifact/nonartifact designs and idempotent orders.

Private original observations enter the actual Node branch and run real veh_at
and stack_fix with explicit controlled offline globals and coherent8x6 fixtures.
A single observation stop precedes draw_tile; whole current base,64 actor slots,
512 designs and map bytes match independently expected changes in28 contexts per
edition. Owned proposals reproduce those exact byte effects on both architectures.
This is not the multiplayer repair branch, a complete original production action,
queue/audio/notification behavior, a turn or full-game qualification.

No live mutation, facility installation, queue commit, research award, player
input or phase authority is added. Normal SDK binaries compile only; actual
AmigaOS execution and final-head full CI remain unrun.

### Ordinary completion tail dependencies (local only)

`completion_tail` recovers two ordinary-facility tail dependencies without
pretending that their intervening UI/monitor/reset behavior is implemented.
Facilities13,14,31,32 increment the original clean-mineral modifier only when it
is nonzero and clear faction player_flags0x4000. Zero stays zero. The later mod's
optional forced increment is excluded. Signed overflow rejects before writes.

The second fragment reports the original base_reset requirement: an AI faction,
or a managing governor without an admitted queue, still needs the real reset.
The result does not invoke a reset or supply a fake return value. An unmanaged
human falls back to edition-correct Stockpile Energy(-38/-69) only when the
POST-notification current item still equals the completed facility and its
waiting queue is empty. A real user choice must be reflected before evaluation;
a cached item from before that choice is not sufficient. Governor management
requires both bits0x80000001. Ordinary IDs2..33/64 and queue sizes0..9 only.

Host/independent68040 suites cover164,174 cases including zero/negative/overflow
counters, partial governor flag combinations and post-choice item changes.
Private original fragments cover1,280 SMAC and2,520 SMAX contexts, comparing the
entire current base/selected faction and actual reset/fallback branch stops.
The eco fragment starts after applicable monitor/interlude processing. Reset
and redraw calls are stopped BEFORE execution; they are never stubbed. Those
two staged observations are not execution of a combined original production
function, its notifications, AI production choice or full turn.

There is no World binding/mutation, facility-announcement acknowledgment, queue
commit, original monitor/achievement execution, phase authority or frontend
input. Normal SDK compilation and local controls do not qualify actual AmigaOS
execution or final-head CI; both remain unrun.


### Original facility and Preserve monument state (local candidate)

`monument_effects` proposes event 4 or 12 in the original eight-record monument
wire table. It preserves all unrelated bytes, handles nonzero legacy markers,
copies the facility's base name with original 24-byte padding, and retains the
SMAX alien exclusion absent from SMAC. The result reports the original local
presentation-call obligation. It does not open/acknowledge a window, mutate the
World, encode a player save, complete production or advance a turn.

`test_monument_effects.py --out DIRECTORY` checks 2,064 cases against an independent
byte oracle, plus null/overlap guards. Use `--cc m68k-linux-gnu-g++ --m68k-linux`
for an independent big-endian runner. Private bounded original-helper comparisons
are separate from public synthetic controls. Amiga SDK builds are not AmigaOS
execution. This candidate has no published final-head CI or native qualification.


### Original monument presentation entry (local candidate)

`monument_presentation` checks original multiplayer, SMAX alien, preference,
scenario and final-score gates against an explicit local monument record. Manual
entry bypasses preference/state gates. It counts events 0–14 only and reports
whether the original count global was written and window initialization must be
attempted. The original initializer may fail; no visible window, modal response,
audio, acknowledgment, completion transaction or game turn is implied.

`test_monument_presentation.py --out DIRECTORY` checks 4,040 synthetic cases and
three null/overlap guards. Pass `--cc m68k-linux-gnu-g++ --m68k-linux` for the
independent big-endian runner. Private comparisons separately execute both
original entries to the real window initializer boundary, including a nested
milestone-update → original monument-entry comparison. No published final-head
CI or AmigaOS execution qualifies this candidate.


### Original facility announcement fragments (local candidate)

`facility_notice` proposes original outer/first-announcement routing, the staged
announced-bit write, voice-load and post-popup-fade obligations. Later-stage
warning/preferences are explicit snapshots; calling this query does not simulate
constructors, popups, user acknowledgment or audio. The original base-edition
foreign threshold33 and expansion threshold40 differ. Expansion facility40 reports
a separate Subspace-generator obligation whose interlude/generator path is not
implemented here. Input is limited to IDs2..33/2..41; unused expansion slots and
satellite/project production are outside scope. No World mutation, production
completion, phase, player-save persistence or turn is added.

`test_facility_notice.py --out DIRECTORY` checks 36,877 cases against an independent
byte oracle; `--cc m68k-linux-gnu-g++ --m68k-linux` selects the independent runner.
Private bounded original comparisons separately stage notification fragments and
stop before actual GUI/audio calls. These are not combined original popup runs.
Amiga SDK builds have no AmigaOS execution or published final-head CI yet.

Announcement wire correction: both editions start `facility_announced` at0x458;
legacy holds8 bytes, modern12. Legacy `clean_minerals_modifier` therefore starts
at0x460, modern0x464. The existing importer already implements this correctly.
An older private notification note claiming legacy announcement offset0x454 is
superseded; no imported data or frozen production source was changed.


### Original post-modal popup response (local candidate)

`popup_response` proposes the original offline effects of a **confirmed** popup
result. A checked disable-warning box takes precedence: clear the warning mask,
mirror warnings, and require actual preferences persistence. Otherwise result0
sets the deferred base-window request, result2 enables governor0x80000040, and
result3 requires real nerve-stapling execution. Unknown raw signed results retain
state. This function does not fabricate a modal result, call a fake callback,
change production, open a base window, save preferences, staple a base, mutate
the World or advance a turn. Network/PBEM behavior is outside its explicit owned
offline policy. Real rendering/input/authority and dependent actions remain
unimplemented and unqualified.

`test_popup_response.py --out DIRECTORY` checks39,207 cases against an independent
byte oracle. `--cc m68k-linux-gnu-g++ --m68k-linux` selects the independent runner.
Private original comparisons execute only the post-modal response fragments,
stopping before preferences-save, stapling and cleanup calls. They never execute
or stub the modal. Amiga SDK builds have no AmigaOS execution; no final-head
published CI or full production completion is claimed.


### Original local facility notice arguments (local candidate)

`notice_labels` returns ordinary local first/regular notification labels, queue
count substitution, warning mask and human/alien facility icon selection. Queue Q
precedes managing-governor G; managing requires both0x80000000 and0x1. It preserves
the existing numeric substitution without an admitted queue. Ordinary IDs2..33/
2..41 only, excluding special expansion Subspace40; queue_size0..9. The caller
must supply actual current local route/queue facts. No popup, audio, modal
response, World/global mutation, completion, phase or turn is performed.

`test_notice_labels.py --out DIRECTORY` checks10,766 controls against an independent
byte oracle. `--cc m68k-linux-gnu-g++ --m68k-linux` selects the independent runner.
Private original comparisons execute real literal/string/parse-number helpers,
stop before popb, and compare whole bases/all ten numeric slots. They do not
execute or replace a modal callback. SDK builds have no AmigaOS execution;
no final-head published CI or playable completion flow is claimed.


### Staged facility notice decisions (local candidate)

`facility_notice_entry`, `facility_notice_construction`, `facility_notice_voice`
and `facility_notice_fade` accept only the facts needed at their respective
original stages. Construction preserves the supplied current announcement vector,
including intervening changes. Audio preferences and the human mask are read at
voice selection; fade uses its own later preferences and has no human recheck.
The combined diagnostic remains a composition of these functions for regression
comparison. It must not be used to precompute future settings for a real window.

The stage functions are pure proposals, not delivery capabilities or an owning
notification controller. No constructor, renderer, modal response, audio playback,
World mutation, completion or turn is supplied. SMAX40 remains a separate
unsupported obligation. Local host sanitizer and independent Linux68040 controls
pass; SDK builds are compile-only. No CI, AmigaOS or player-facing qualification.

Run `python3 ports/alpha-centauri/native/test_notice_stages.py --out OUTPUT`;
use `--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian execution.


### Owned current monument storage (local candidate)

`OriginalWorld::current_monuments()` exposes a read-only span of a separately
owned current table. `game().monuments` continues to refer to the imported wire
baseline. Import copies every original byte, including unknown fields; failed
load preserves the old state, reload replaces both copies and clear frees both.
The memory budget includes the extra table. This supplies storage only: imports
do not award monuments or grant a completion/turn capability.

The five existing snapshot encoders reject changed current monument bytes until
an explicit qualified monument encoding scope exists. Their output and receipt
buffers, and the accounting/worker result buffers, cannot alias the new storage.
No public monument mutator, phase, notification, persistence extension, frontend
control or original executable save-compatibility claim is added.

`test_monument_storage.py` checks whole-container identity and alias/change
rejection across all five encoders. Controlled test-only storage corruption is
an unsupported-change test, not a simulated award or gameplay transaction.
Local host/independent Linux68040 execution and SDK compile-only receipts remain
separate from unrun AmigaOS validation. No publication or remote CI is claimed.


### Original social-upkeep dependency (local candidate)

`social_upkeep` proposes the original pending-to-active choice copy, three full
social effect calculations and paid-upheaval reset. Its explicit current base
modifiers remain authoritative; this operation does not recompute them from
SOCIAL bonuses. The third original calculation changes an unused argument,
not the quick-calculation flag. SMAX alone adds alien temple research.

`world_social_upkeep` binds current pending choices/base modifiers/ordered
bonuses from decoded faction records, project indices21/24/13/23 through actual
base owners, and the original temple landmark/visibility/owner condition across
the decoded map. Same-edition SOCIO rules are required. Output is disjoint from
all World allocations, including separate current monument bytes. Neither API
mutates World or authorizes a social, repair, production or turn phase.

Local host sanitizer and independent Linux68040 controls pass. Private complete
original social_upkeep executions include all three real social_calc calls,
real has_temple/whose_territory and normal returns; the entire selected faction
record is compared. Those bounded original helper observations are separate
from AmigaOS validation, which remains unrun. No remote CI or publication claim.

Run `test_social_upkeep.py` and `test_world_social_upkeep.py` with `--out OUTPUT`;
the latter accepts optional private `--original RULES SAVE` pairs. Both accept
`--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian execution.

### Original repair actor prerequisite (local qualification only)

`repair_vehicle` proposes the first-pass transition for one actor using explicit
current facts. It retains SMAC/SMAX differences for Battle Ogres, Fungal Towers
and air repair projects; timer decrement wraps as the original signed16 storage;
periodic flag clearing uses faction-turn plus actor index. Healing includes
terrain/territory/base/breed/repair facility and project/repair-bay/Nano Factory
conditions and the original damage floor. Xenoempathy Dome is project index8;
Pholus Mutagen is index16 and is not the fungus healing bonus here. Cargo means
`veh_cargo` capacity, not occupied transport slots. Base breed uses base ownership.
All unrelated vehicle fields are preserved. Invalid profile inputs and complete
input/output overlap reject before writes; foreign actors preserve their record
without inspecting unused design facts.

The API does not bind a World, inspect stacks, reset faction ODP, render, deliver
an event, advance a phase, refill player movement or authorize an end turn. Its
`draw_requested` describes the later predicate, not a rendering acknowledgment.
The caller must derive actual current facts and coherent stacks. SMAX FREEABIL
resolution belongs to the qualified ability dependency, not a raw-only assumption.

Local checks: 27,665 public cases (27,648 accepted,17 rejected), ASan/UBSan host
and independent static Linux68040 output identical. Private owner observations
execute the actual complete original actor first pass in8,192 contexts per edition,
including real territory/base_at/breed_mod/bitmask/veh_cargo/has_abil helpers, two
coherently linked actors, three controlled bases and one2x1tile. Every byte of both
actor records and the faction ODP reset agrees; all other bytes of the mapped
0x90000global region remain unchanged. One instrumented observation jump stops
before the second rendering loop. The original routine does not return normally
in these observations. These are bounded helper observations, not whole-game or
complete repair-phase execution. Raw ability paths are observed; FREEABIL grants
are not exercised here. Original owner inputs remain private.

Run `python3 ports/alpha-centauri/native/test_repair_vehicle.py --out OUTPUT`;
use `--cxx m68k-linux-gnu-g++ --runner qemu-m68k -cpu m68040` for independent
big-endian execution. `build_repair_vehicle_probe.sh` builds with the normal Amiga
SDK. Neither object compilation nor host/Linux execution qualifies AmigaOS. The
candidate has no remote CI, PR, native execution or player input wiring.

### Read-only owned-World repair binding (local qualification only)

`world_repair_vehicle` derives the repair proposal from decoded current actors,
designs, component tables, map tiles, bases, facilities, projects and technology
owners. Its explicit phase and local faction arguments are query context, not
turn authority; imported CurrentTurn only supplies the periodic flag arithmetic.
Foreign actors preserve their complete record without evaluating unused design
facts. Outputs cannot overlap the World, its owned buffers/records, or rule tables.

The friendly-territory predicate is exactly positive owner==phase for the original
`whose_territory(phase,x,y,null,0)` return test. The physical tile-base predicate
remains distinct from `base_at`: a set base bit requires an actual matching base
when healing reaches that lookup. Breed bonuses use the base OWNER and original
physical facilities9/30/31/32 plus project indices8/16/31/17. Non-native physical
repair facilities short-circuit the repair-project lookup as in the original.
Negative project locations mean absent; inspected positive out-of-world locations
reject. Cargo nonzero is equivalent to declared capacity nonzero even in native
morale-expanded cargo cases. No occupancy approximation is used.

When repair inspects a stack, links must be reciprocal, acyclic and coordinate
consistent, and include every colocated actor. Broken stacks reject without fixup.
Repair Bay0x200000 uses raw bits in SMAC and predefined designs. SMAX custom
missing bits resolve FREEABIL21 through design-owner technology and the actual
ability prerequisite. Raw-present bits retain their short-circuit. These helpers
remain scoped to repair; this does not generalize unrelated ability queries.

Local final host ASan/UBSan and independent Linux68040 checks pass293worlds,
22,974queries (21,150accepted/1,824rejected),1,446healing attempts and6,442guards,
including all3owner tutorials. Full decoded/opaque World state, current monument
storage, rules, RNG and input files remain unchanged. Final output agrees across
architectures. Additional private comparisons load384World fixtures reconstructed
from archived actual original first-pass observations (192per edition); complete
selected actor records match on both architectures. The irrelevant home-base
field is normalized to-1 in both fixture/expected to satisfy World import bounds.
Original helpers were not reexecuted for those binding comparisons. The archived
observations stop before rendering; they do not qualify a complete repair phase.

Run `python3 ports/alpha-centauri/native/test_world_repair_vehicle.py --out OUTPUT`;
add `--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian execution.
`build_world_repair_vehicle_probe.sh` uses the normal Amiga SDK. Fresh identical
builds remain unexecuted on AmigaOS. No remote CI, PR, deployment, World mutation,
faction ODP reset, event/render delivery, session/phase authority, movement refill,
player End Turn or persistence is provided. Owning phase admission and ordered
production-prefix/aggregate/debt dependencies still need implementation.

### Original pre-base production setup (local qualification only)

`world_production_prefix` proposes the complete original production setup before
its first base iteration. It resets the four phase aggregates, recomputes the five
technology-effect fields from actual rules/owners/bonuses, settles loans in
ascending creditor1..7order, clears player flag0x20, proposes the pre-base energy
scratch, and clears social support/psych/efficiency arrays plus the three unknown
counters, nutrient and labs totals. The selected decoded faction's other fields
survive; other factions only propose energy-credit changes. Legacy social arrays
start0x68c; modern0x690. Explicit faction/profile are query context, not authority.

`production_loans` retains original signed arithmetic: payment is the signed
minimum of current reserves and scheduled payment, with no modern zero clamp or
cap to principal. Negative reserves/payments/nonzero balances follow original
branches. Alive bits are preferences.FactionStatus[1], separate from human bits;
self and creditor0 are excluded; recipient sanctions must be exactly zero. War
bit0x10adds scheduled payment to debt without transferring credits. Every signed
intermediate is checked, and errors/aliases preserve output. No unchecked wrap
or nonnegative-domain patch is introduced.

Local ASan/UBSan host and independent Linux68040 checks agree:10,852loan contexts
(7,721accepted,4invalid,3,127overflow), and203Worlds/2,030queries
(1,565accepted,465rejected) with3,244guards, including3owner tutorials and empty
Worlds. All decoded/opaque World state, monuments, rules, RNG and input files stay
unchanged. The probe's explicit field encoding is test-only and checks every
other decoded field; no native structure dump or production save writer is added.

Private actual-original observations execute SMAC0x53b7a0 and SMAX0x526e70through
254accepted contexts each, including the real complete tech_effects helper.
One observation jump stops at the branch before base iteration (SMAC0x53b941,
SMAX0x527024); no fake callbacks. Complete selected-faction records, all8energy
balances and the energy scratch match; every other byte of mapped0x90000globals
stays unchanged. Both changed owned executables reproduce those same observations.
Overflow inputs are excluded from original execution and remain rejection tests.

Run `test_production_loans.py --out OUTPUT` and
`test_world_production_prefix.py --out OUTPUT`; add
`--cc m68k-linux-gnu-g++ --m68k-linux` for big-endian Linux execution. Normal Amiga
SDK scripts are `build_production_loans_probe.sh` and
`build_world_production_prefix_probe.sh`. Fresh matching builds remain unexecuted
on AmigaOS. No remote CI, PR, World/credits/cache mutation, base upkeep, phase
admission, event pump, player End Turn, discovery, AI or save claim. Post-base
maintenance discounts, interest, first-turn free advances and sanctions events
are outside this prefix. Real phase identity and ordered event/base execution
remain required before exposing any turn advancement.

### Phase-field persistence prerequisite (local qualification only)

`encode_phase_snapshot` extends the frozen worker/action/accounting buffer codec
with explicit social-upkeep, ODP and pre-base production faction fields. The
allowlist is in `phase_faction_fields.inc`: current4social choices, paid upheaval,
pending/active/temporary11effect arrays, credits/loan balances,5technology caches,
ODP deployed, phase aggregates and social/nutrient/labs accumulators. Player
flag0x20may clear; setting it or changing other player flags rejects. Pending
social choices, base social modifiers, loan schedules, diplomacy, tech owners,
prototypes, definitions, monuments and all metadata remain sealed. The previous
full map/actor storage and worker/action/accounting structural scope is retained;
serialization does not establish movement legality or phase authority.

The encoder uses a private owned World projection to retain the entire frozen
union validator. Only that local allocation is modified; caller state stays
unchanged. Additional allocation is explicitly bounded by `max_clone_bytes`.
Validation and allocation finish before caller output writes. Legacy faction
fields at/after0x464shift4bytes (social arrays0x68cvs modern0x690); earlier fields
including ODP0x364retain their offsets. All patches are explicit little-endian.
Complete original opaque bytes/footer survive; runtime RNG remains a separate
receipt. No native structure dump, filesystem protocol, atomic-file assumption,
player save UI or original executable compatibility is claimed.

Final host ASan/UBSan and independent Linux68040 tests agree across11Worlds,
395controls:107exports,288rejections and31continued exports after reload. They
include all3owner tutorials, versions7..12, empty Worlds, complete independent
Python byte comparisons, reload/idempotence, inherited union composition,
clear/set flag boundaries, output/receipt/storage/wrap aliases, short buffers,
clone exact-budget success and one-byte-short failure. Frozen workers encoding
still rejects the newly allowed faction deltas. The independent wire oracle is
an explicit offset list, not generated from the C++ allowlist. Historical flag
oracle failure is retained privately; corrected final host3/BE3 are authoritative.

Run `test_phase_snapshot.py --out OUTPUT`; add
`--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian execution.
`build_phase_snapshot.sh` builds with the normal Amiga SDK; two fresh identical
builds remain unexecuted on AmigaOS. No remote CI or PR is provided. This is a
persistence prerequisite only: it does not implement an owning phase controller,
commit social/repair/upkeep, admit a turn from a save, or enable player End Turn.
A future controlling engine must block exports while real events are pending.

Explicit decoded loader repairs are documented in [LOAD-REPAIRS.md](LOAD-REPAIRS.md).
They preserve raw import behavior and grant no turn authority or frontend action.

The complete original deficit-energy convoy cancellation proposal is documented
in [ENERGY-CONVOY-ORDERS.md](ENERGY-CONVOY-ORDERS.md). Its current-stage input
and immutable output do not establish turn authority or a player action.

Exact original load-record repairs can be retained in a snapshot buffer through
[INITIALIZED-SNAPSHOT.md](INITIALIZED-SNAPSHOT.md). This remains a structural
persistence boundary, with no new engine/player action or filesystem save API.

### Explicit repaired offline command resume

`LocalProductionSession::load_resumed_commands` derives the sole local human
from the saved MapWin owner and human mask, applies the verified original record
repairs, requires that owner to survive those repairs, then selects that owner
as CurrentFaction and clears the original PBEM-restart rule bit40 in a private
normalized command baseline. Every other rule bit is preserved. This is the original
offline resume selection; it does not refill movement or authorize upkeep.
Raw `load_local*` imports retain their previous saved-current-faction requirement.
The new import owns all accounting tables; its explicit base-actions option also
owns the existing bounded base-game movement tables. Expansion movement remains
unsupported. Existing private lifetime/revision command checks continue to apply.

The external input stays unchanged. `world()->original_bytes()` now refers to the
**repaired, owner-selected command baseline**, not the unmodified external source.
Existing worker/action snapshot encoding preserves this baseline and subsequent
supported commands; no filesystem save protocol or original executable acceptance
is claimed. Private repair/normalization and session loading must all succeed
before replacement; failed loads preserve the previous session and its revision.
The memory limit covers each operation plus its live private staging allocations,
excluding the old session, caller input and inherited validator scratch. The
private rule table/repair World are destroyed before command-session loading.

This remains backend integration only: no frontend controls, path/contour/cursor
initialization, real modal/audio/render event delivery, phase admission, end turn,
AI, combat or diplomacy. RuntimeMove zero, imported CurrentFaction and refreshed
accounting never confer phase authority. Native execution and remote CI are unrun.

```
python3 ports/alpha-centauri/native/test_resumed_commands.py --out /tmp/resumed-host
python3 ports/alpha-centauri/native/test_resumed_commands.py --out /tmp/resumed-be --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ ports/alpha-centauri/native/build_resumed_commands_probe.sh /tmp/resumed-native
```

### Owned original unit skip command

`prepare_skip_unit`/`commit_skip_unit` bind the original `veh_skip` assignment
to an existing session lifetime/revision and locally owned actor. They use the
session's own action or accounting unit tables and recovered original speed
calculation. Production-only imports have no such capability. A successful
changed commit writes only `moves_spent`, invalidates earlier plans, and reports
repaint/persistence obligations. Repeating it is a no-op; suspend/reload and
address reuse cannot revive old commands. Rendering is not performed here.

The original function assigns speed rather than `max(old,speed)`, so an old
larger spent byte can decrease to the computed maximum. It does not change
orders, working/skip flags, actor state, position, RNG or advance a turn. The
checked speed domain remains 0..255; original overflowing low-byte wraps are
explicitly rejected before mutation. Thinker's additional skipped-unit flags,
former working flags and monolith actions are not part of this original leaf.

Existing whole-world worker/action buffer encoding preserves this single-byte
command and the repaired resume baseline. The command is backend only, with no
player input, real renderer/audio delivery, readiness confirmation, End Turn,
upkeep, filesystem player save, native execution or full-game qualification.

```
python3 ports/alpha-centauri/native/test_local_unit_skip.py --out /tmp/skip-host
python3 ports/alpha-centauri/native/test_local_unit_skip.py --out /tmp/skip-be --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ ports/alpha-centauri/native/build_local_unit_skip_probe.sh /tmp/skip-native
```

### Original unit readiness and boarding inspection

`world_unit_readiness` implements the original `veh_ready` short circuits and
`veh_jail` boarding restriction. The const session binding uses its file-derived
human and privately parsed tables; production-only sessions cannot inspect this
capability. Both flag40 and flag400 together block readiness. Standing orders1,
2 or3 block it unless state200 or4000 permits evaluation. A ground unit ordered
aboard an air carrier is jailed unless its tile has a physical base with low
faction nibble1..7 or a physical airbase. Planet0 bases remain unsafe in the
original; no co-location, carrier-ownership or territory filter is invented.
Invalid carrier indices reject before access. Movement remaining is evaluated
only after these gates: readiness requires checked current speed > moves_spent.

The result explicitly marks whether speed was evaluated. The API is immutable;
errors preserve output, and outputs cannot alias world/session/owned tables.
It accepts only the inherited checked speed domain0..255 and parity-valid map
coordinates. A ready result is not a command-completion acknowledgement, genuine
readiness confirmation, End Turn permission, event delivery or phase authority.
No player input or preview control is added. Actual rendering/audio/modal loops,
AI/upkeep/turns, native execution and remote CI remain unqualified.

```
python3 ports/alpha-centauri/native/test_unit_readiness.py --out /tmp/ready-host
python3 ports/alpha-centauri/native/test_unit_readiness.py --out /tmp/ready-be --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ ports/alpha-centauri/native/build_unit_readiness_probe.sh /tmp/ready-native
```

### Original offline ready-unit search transaction

`world_ready_search` follows the offline branch of original
`Console_ready_search`: up to three passes over original readiness, nearest
unit by the original wrapped range metric, and current-unit preference on ties.
Reference/current indices and cursor coordinates are spatial facts only. A local
reference supplies its position; a foreign reference uses the supplied cursor.
When no candidate is found, state bit2 is cleared across **all** actors. The
first pass preserves current's bit2 when the reference is local, the second
clears all, and the third may then find current. Neither movement spent nor
orders, RNG or turn counters are reset. Online priority behavior is excluded.

`prepare_ready_search`/`commit_ready_search` bind that proposal to the owning
session's lifetime/revision and sole human. Commit re-evaluates the full result
before atomically clearing only the proposed state bits. Any unsafe reference
or unsupported readiness dependency aborts the whole proposal without partial
clears. Original callback-free offline search is implemented here; original
rendering, input/modal/audio and online Console paths are not substituted.
Returned selection is for a future UI caller; no Console/UI cursor is changed.

Whole-world buffer persistence retains these bit clears and all existing
supported commands. Reload and repeat-search controls preserve the resulting
bytes. No player input, End Turn, phase admission, command-completion signal,
real event delivery, native execution, filesystem save or playable-game claim.

```
python3 ports/alpha-centauri/native/test_ready_search.py --out /tmp/search-host
python3 ports/alpha-centauri/native/test_ready_search.py --out /tmp/search-be --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ ports/alpha-centauri/native/build_ready_search_probe.sh /tmp/search-native
```

Original opponent-specific native morale is available as a checked read-only
leaf and saved-world query. See [NATIVE-COMBAT-MORALE.md](NATIVE-COMBAT-MORALE.md)
for edition differences, absent version11 runtime rejection and remaining
battle/command/native gates.

The original riot-morale ordering is restored in this candidate; see
[RIOT-MORALE-CORRECTION.md](RIOT-MORALE-CORRECTION.md). Earlier source-reference
passes included an OpenSMACX gameplay change and do not qualify this correction.

Original basic combat-strength composition now connects qualified prototype,
psi, morale and physical-base dependencies. See [BASIC-COMBAT.md](BASIC-COMBAT.md)
for original/mod differences, supported domain and remaining real-battle gates.

Original read-only terrain defence and its saved-world binding: see [TERRAIN-DEFENSE.md](TERRAIN-DEFENSE.md). Native execution remains unrun; no attack/player command is added.

Original conventional base combat stage: [BASE-COMBAT.md](BASE-COMBAT.md). Read-only incoming-strength adjustment; final battle/damage/actions and native execution remain incomplete.

Original combat sensor stages: [SENSOR-COMBAT.md](SENSOR-COMBAT.md). Read-only actual World coverage and signed percentage adjustment; complete battle/player actions/native qualification remain open.

Original ground mobility/road/elevation stages: [GROUND-MODIFIERS.md](GROUND-MODIFIERS.md). Explicit incoming strengths; final battle/actions/native qualification remain open.

Late Pulse/Jammer/Tracking arithmetic: see [DEFENSE-ABILITIES.md](DEFENSE-ABILITIES.md); raw stage only, World/native/UI pending.

Owned late defense ability binding: see [WORLD-DEFENSE-ABILITIES.md](WORLD-DEFENSE-ABILITIES.md); read-only current facts, native/UI pending.

Original air/noncombat/In Port arithmetic: see [AIR-PORT-COMBAT.md](AIR-PORT-COMBAT.md); explicit conventional numeric stage only, World/native/UI pending.

Owned air/noncombat/In Port binding: see [WORLD-AIR-PORT-COMBAT.md](WORLD-AIR-PORT-COMBAT.md); current conventional facts only, native/UI pending.

Original artillery numeric modifiers: see [ARTILLERY-COMBAT.md](ARTILLERY-COMBAT.md); bit0x2 inspection only, World/native/UI pending.

Owned artillery modifiers: see [WORLD-ARTILLERY-COMBAT.md](WORLD-ARTILLERY-COMBAT.md); actual map/unit facts, numeric inspection only, native/UI pending.

Original psi ability phases: see [PSI-ABILITIES.md](PSI-ABILITIES.md); explicit early-offense/late-defense inputs, World/native/UI pending.

Owned psi ability phases: see [WORLD-PSI-ABILITIES.md](WORLD-PSI-ABILITIES.md); actual component/grant/missile facts, separate early/late strengths, native/UI pending.

Original airdrop/fanatic/faction/Planet combat phases: see [COMBAT-FACTIONS.md](COMBAT-FACTIONS.md); separate explicit numeric phase inputs, World/native/UI pending.

Owned airdrop/faction/Planet binding: see [WORLD-COMBAT-FACTIONS.md](WORLD-COMBAT-FACTIONS.md); actual current actor/faction/design-owner technology facts, explicit phase strengths, native/UI pending.

Original fungus attack phase and World binding: see [FUNGUS-COMBAT.md](FUNGUS-COMBAT.md); explicit incoming offense, numeric inspection only, native/UI pending.

Coherent conventional land battle numbers: see [LAND-COMBAT.md](LAND-COMBAT.md); complete-original numeric comparisons within the stated domain, no attack/native/UI qualification.

Original land retreat destination inspection: see [RETREAT-DESTINATION.md](RETREAT-DESTINATION.md); source-backed selection only, no relocation or native/UI qualification.

Owned conventional LAND engagement staging: see [LAND-ENGAGEMENT.md](LAND-ENGAGEMENT.md); current-frame damage/retreat through pending casualty or post-storage boundary, no player attack/commit/native qualification.

Checked vehicle-removal storage is documented in [VEHICLE-REMOVAL.md](VEHICLE-REMOVAL.md). It updates caller-owned dense arrays and explicit runtime references only; World/session commits, full casualty effects, native execution and player actions remain held. Synthetic controls: `python3 test_vehicle_removal.py --out /tmp/alpha-removal-host` and independent `--cc m68k-linux-gnu-g++ --m68k-linux`.

[WORLD-VEHICLE-REMOVAL.md](WORLD-VEHICLE-REMOVAL.md) documents current-frame ownership around checked removal. Caller/session commits, full casualty effects and variable-length persistence remain held. Controls: `python3 test_world_vehicle_removal.py --out /tmp/alpha-world-removal-host` and independent `--cc m68k-linux-gnu-g++ --m68k-linux`.

[WORLD-CASUALTY.md](WORLD-CASUALTY.md) documents owned ordinary/native casualty deletion and copied caller pools. Artifact/interlude and caller/session/native/full-game gates remain held.

[ARTIFACT-CAPTURE.md](ARTIFACT-CAPTURE.md) documents owned LAND artifact transfer and its captured caller pool, before the interlude gate. Legal battle/session/delivery/native/full-game gates remain held.

[LAND-RESOLUTION.md](LAND-RESOLUTION.md) documents the owned solitary conventional engagement/casualty/capture composition. Caller argument6 is explicit per branch; retreat has no RemovalRuntime claim. Full caller/event/session/persistence/native/gameplay gates remain held.
