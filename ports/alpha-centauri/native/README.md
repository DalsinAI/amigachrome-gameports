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
