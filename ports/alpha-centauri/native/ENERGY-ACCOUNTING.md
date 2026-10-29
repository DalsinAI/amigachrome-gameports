# Fresh energy subtotal, original HQ loss and shortfall proposals

This component computes an energy proposal from current-stage facts. It does
not advance a turn, allocate workers, calculate commerce, generate research,
pay maintenance, or apply an owned-world mutation.

`base_energy_accounting` adds fresh intake, incoming energy convoys, commerce
and alien grid output, subtracts outgoing energy convoys, then applies the
original HQ-distance inefficiency formula. `world_energy_accounting` derives
nearest own physical HQ, physical Children's Creche, pending efficiency and
hypothetical efficiency statistics from the owned snapshot.

`world_fresh_energy` connects the actual actor-ordered convoy engine to the
stable directed-human worker intake engine, then to energy accounting and
shortfall-order proposals. Saved intake, surplus and inefficiency never serve
as fresh inputs. Commerce and alien-grid generation are explicit authoritative
prerequisites. The existing stable worker stage0, no-specialists, directed-human
restrictions remain; unsupported allocation branches reject instead of using a
saved result. It is a proposal, not the original mutable `base_compute` entry.

## Preserved original behavior

The source is Brendan Casey's GPL OpenSMACX, commit
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`, `src/base.cpp::black_market`
(original004EA1F0), `base_energy_costs` (004F4DC0), and
`src/map.cpp::vector_dist`/`base_who`.

- No HQ uses distance16. The original nearest-distance sentinel999 remains:
  an HQ whose vector distance is999 or more does not replace that sentinel.
  Thinker's later faction-base-count-dependent missing-HQ policy is not used.
- HQ distance0 and nonpositive available energy return before changing the9
  hypothetical efficiency statistics. Only explicit upkeep stage1 updates
  those statistics. Their terms are not clamped to the available energy.
- A physical Creche adds2 efficiency. A queued Creche/HQ supplies no effect.
- Energy shortfall cancels all the owner's plan10/order3/channel2 convoys on
  physically marked base tiles. The original routine is not limited to the
  failing base's own exports/home base. Convoys on foreign bases also qualify;
  exterior energy harvesters do not. Only a proposed order0 bitmap is returned.
- Signed32 intermediate overflow and invalid/aliased inputs reject without
  writing outputs. These checks establish a safe supported domain rather than
  reproducing undefined C++ signed overflow.

Gross/export/surplus composition is corroborated by Thinker `mod_base_energy`
(MIT commit`dabe62f2d1abba03de40f3dd3048494843f6a9af`). OpenSMACX does not
supply a complete `base_energy` allocation body. Full commerce ranking,
allocation sliders, specialist upgrading, facility multipliers, stockpile
energy timing, psych/drone processing, research and facility upkeep are not
implemented here. In particular, Thinker's later stockpile timing and no-HQ
policy are not silently treated as original behavior.

## Validation

`test_upkeep_base_energy.py --out NEW_DIRECTORY` builds the arithmetic leaf with
ASan/UBSan, checks an independent integer oracle, all aligned partial input/
output overlaps, and records the fixed binary corpus plus expected output.

`test_upkeep_world_energy.py --out NEW_DIRECTORY` builds connected queries with
ASan/UBSan. Its wire-derived oracle covers versions7..12, physical/current/
remote/missing HQs, physical Creches, wrap distance, pending efficiency, fresh
worker/convoy inputs, deliberately impossible saved energy totals, convoy
cancellation filters and all-world-storage alias rejection. World bytes,
decoded records, rules, transient inputs and RNG seed must remain unchanged.

`CXX=m68k-amigaos-g++ sh build_energy_probe.sh leaf|world OUTPUT` builds genuine
68040/FPU Amiga HUNK controls, without SDL or proprietary game assets.
Native AROS compatibility evidence is separate from AmigaOS3.2 qualification.
