# WP0b: rules tables

`engine/rules.cpp` + `engine/rules.h` fill the rules tables of the bound
`EngineState` from the original text files. The code is Thinker Mod's reader
(`src/config.cpp`: `text_open`, `text_get`, `text_item*`, `tech_name`,
`chas_name`/`weap_name`/`arm_name`, `read_basic_rules`, `read_tech`,
`read_faction`, `read_factions`, `read_units`, `read_rules`;
`src/game.cpp`: `labels_init`, `label_get`; `src/strings.cpp`: `stoi`,
`btoi`, `purge_spaces`, `kill_lf`, `strtrail`, ...) at
`dabe62f2d1abba03de40f3dd3048494843f6a9af`, ported line for line.

```cpp
std::unique_ptr<EngineState> s(new EngineState());
engine_reset(*s);
engine_bind(s.get());
if (engine_load_rules("DH1:SMAC", 1)) {          // 1 = alphax.txt, 0 = alpha.txt
    printf("%s\n", engine_rules_error());
}
```

## What is loaded

| Source | Tables |
| --- | --- |
| `labels.txt` `#labels` | `TextLabels`, `label_get(i)` |
| `#TECHNOLOGY` | `Tech`, `*TechValidCount`, `*TechCommerceCount` |
| `#RULES`, `#WORLDBUILDER` | `Rules`, `WorldBuilder` (and the `conf` values Thinker derives) |
| `#TERRAIN` | `Terraform`, `Order[4..23]` |
| `#RESOURCEINFO` | `ResInfo` |
| `#TIMECONTROLS` | `TimeControl` |
| `#CHASSIS`, `#REACTORS`, `#WEAPONS`, `#DEFENSES`, `#ABILITIES` | `Chassis`, `Reactor`, `Weapon`, `Armor`, `Ability` |
| `#MORALE`, `#DEFENSEMODES`, `#OFFENSEMODES` | `Morale`, `DefenseMode`, `OffenseMode` |
| `#UNITS` | `Units[0..n)` (components, name, tech, icon; `make_proto` (WP5) derives cost/plan/carry) |
| `#FACILITIES` | `Facility[1..133]` |
| `#ORDERS`, `#COMPASS`, `#PLANS`, `#TRIAD`, `#RESOURCES`, `#ENERGY` | `Order`, `Compass`, `PlansShortName`/`PlansFullName`, `TriadName`, `ResName`, `Energy` |
| `#CITIZENS`, `#SOCIO`, `#DIFF` | `Citizen`, `SocialParam`/`SocialField`, `Difficulty` |
| `#BONUSNAMES`, `#FACTIONS`, `#NEWFACTIONS`, `#CUSTOMFACTIONS` + faction files | `BonusName`, `faction_pool`, `MFactions[1..7]` (first eight rows of each faction file) |
| `#MANDATE`, `#MOOD`, `#REPUTE`, `#MIGHT`, `#PROPOSALS`, `#NATURAL` | `Mandate`, `Mood`, `Repute`, `Might`, `Proposal`, `Natural` |

The record tables live in the bound `EngineState`. The plain string tables
(`TextLabels`, `Compass`, `PlansShortName`, `PlansFullName`, `TriadName`,
`Difficulty`, `Mood`, `Repute`) and the strings every `char*` field points to
are process-wide: `Strings_put` interns text in a pool that is never freed, so
pointers stay valid across reloads and state copies, and reloading the same
files does not grow it.

`engine_load_rules_ex(dir, expansion, 0)` is Thinker's `read_rules(0)`: it
skips `#UNITS` and the faction files (used when loading a saved game, which
carries its own prototypes and factions).

## Deviations (`Fix:` in the code)

- Files are opened from the given directory; each name is tried as given,
  then lower case, then upper case (no `filefind`, no ini file;
  `prefs_fac_load` sets `search_key = filename` like the original without
  "Prefs Format" 12).
- CR of CRLF line ends is removed together with LF.
- Parser errors (`BADTECHKEY`, `BADCHASKEY`, `BADWEAPKEY`, `BADARMKEY`,
  `DUPLICATETECH`) raise the popup through the event sink and make the load
  return 1 with `engine_rules_error()` set, instead of `exit_fail()`. A missing
  file or section also returns 1 and names it. A missing faction file shows
  `PLAYERFILE` and continues like Thinker; `engine_rules_missing_factions()`
  counts them.
- Every string field starts as `""` (never null), and every tech reference as
  `TECH_Disable`, before reading.
- `text_get` resets the item cursor also at end of file; `purge_spaces` and
  `noun_item` do not read before/after an empty string; the
  `IMPUNITY`/`PENALTY` bonus loop stops at `MaxBonusNum` inside the loop.
- `ResInfo` rows and the faction ideology fields are addressed by name rather
  than by pointer arithmetic over struct members.
- `read_units` stores chassis/weapon/armor/abilities/reactor and
  `UNIT_ACTIVE` before calling `make_proto`, so the table is usable while
  `make_proto` is still a WP5 placeholder.
- No user-interface side effects: `load_faction_art` and the edit-map
  `BaseButton_set_bubble_text` calls are removed; the DEBUG-only magtube speed
  listing is removed.
- `parse_string` (an executable routine in Thinker) is implemented for the
  tokens rules text uses: `$STRn`, `$WORDn`, `$NUMn`, `$NUMBERn`, `$$`.
- Base game (`alpha.txt`, `expansion = 0`): Thinker only reads `alphax.txt`.
  The base file has 21 weapons, 10 armors, 24 abilities, 71 facilities,
  29 bonus names, no `#NEWFACTIONS` and no subspace generator rows. Its rows
  are placed into the Alien Crossfire slots the engine enums use
  (`rules_weapon_slot`, `rules_facility_slot`): weapons 0..11 stay, 12..20
  move to `WPN_STRING_DISRUPTOR`..`WPN_ALIEN_ARTIFACT`; facilities 1..33 stay,
  34..71 move to `FAC_SKY_HYDRO_LAB`..`FAC_ASCENT_TO_TRANSCENDENCE`. Slots
  without a row keep an empty name and `TECH_Disable`. `#FACTIONS` fills
  `MFactions` (Thinker's `smac_only` path). Labels are read as they are.

## Tests

```sh
python3 test_engine_wp0b.py --out /tmp/wp0b-host
python3 test_engine_wp0b.py --cc m68k-linux-gnu-g++ --m68k-linux --out /tmp/wp0b-m68k
python3 test_engine_wp0b.py --out DIR --original /path/to/game   # owner data, not committed
```

Synthetic base and Alien Crossfire directories (also with CRLF line ends) are
built from the existing generators (`test_faction`, `test_units`,
`test_facility_definitions`, `test_resources`, `basic_fixture.json`) and every
loaded value is compared with the existing Python references
(`test_social.reference`, `test_faction.reference`, `test_units.reference`,
the basic fixture's expected values) or with the generated values. Unknown
keys, a duplicate technology, a missing labels file and missing sections must
be rejected.
