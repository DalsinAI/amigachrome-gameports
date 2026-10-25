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
