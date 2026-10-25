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
