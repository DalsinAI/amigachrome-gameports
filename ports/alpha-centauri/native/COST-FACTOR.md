# Owned-world original resource cost factors

`cost_factor.cpp` ports OpenSMACX `base.cpp::cost_factor`, Brendan Casey
(C)2013–2021, GPL-3.0-or-later, pinned at
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`. The recovered routine is marked
complete at original offset **004E4430**. `upstream_cost_factor_reference.h`
retains its exact body for test comparison with explicit global-name bindings.
Neither the test nor the port executes the original game executable.

## Connected API

```cpp
int32_t factor;
CostFactorError error = world_base_cost_factor(
    world, basic_rules, base_id, 1, factor); // resource1 = minerals
```

`world_cost_factor(world, basic_rules, faction, resource, base, factor)` also
supports the original optional-base query: base **-1** omits base-local growth
bonuses. A supplied base must exist and belong to the faction. The connected
adapter handles the complete **human** path using owned current stage facts:

- Human identity from the actual faction-status bits.
- Pending industry/growth from the actual faction records.
- Nutrient/mineral multipliers from the qualified original `BasicRules`.
- Planet-size category from **map.world_settings[0]**, original map metadata
  **+0xa30** (`MapSizePlanet`), rather than infer it from map dimensions.
- Physical Children's Creche facility bit 2 and Golden Age state bit 0x4 when
  an actual base is supplied. Queued facilities do not grant a bonus.

No cost factor, threat result or growth/industry adjustment is supplied by the
world caller. The adapter is read-only and rejects aliases into all owned
world/rule storage, preserving the output on any error.

The original AI branch also depends on `great_satan(FactionRankings[7], false)`,
leader human identity and live network-game state. The pure fact leaf preserves
that complete branch and tests every dependency combination. The connected
adapter returns **unsupported for AI** because the original threat/climactic
battle engine has not been connected. It does not replace that engine with an
arbitrary caller boolean. No claim of AI cost-factor integration is made.

## Preserved source behavior

The human starting factor is 10; AI starts from the original difficulty table
13/12/11/10/8/7 minus its leader adjustments. The rule multiplier divides by
10 first, then tiny/small planet categories 0/1 multiply by 8/9 and divide by
10. These are successive integer operations, not one combined fraction.

Resource **0** applies pending growth plus physical Creche/Golden Age bonuses,
then clamps growth to -2 through +5 and performs the original upward rounding.
Resource **1** applies the complete original industry switch. In particular,
ratings **-7 through -3** use 130%; **-2** 120%; **-1** 110%; 0 100%; +1 90%;
+2 80%; +3 70%; +4 60%; every other rating uses the original default half-factor
branch. This includes ratings below -7; they are deliberately not clamped.
Legacy other nonzero resource IDs use the mineral rule multiplier but bypass
industry, exactly as the source does.

The qualified multiplier domain is 1–100, matching `read_basic_rules`.
Unrelated AI difficulty is not read on the human path. Growth addition that
would overflow a signed 32-bit original intermediate rejects transactionally
instead of invoking undefined behavior. A low multiplier on a tiny planet can
legitimately produce **factor0**. Callers must either support that original
case or explicitly reject it; substituting 1 or 10 would change the rule.

## Original-world result and validation

With the genuine currently staged `alpha.txt` and unchanged promotional
Boreholes scenario, human bases **9 and 10 both calculate mineral factor8 and
nutrient factor8**. Their pending industry/growth are 0, normal multipliers
are 10, and the saved planet-size category is 0. The faction2 no-base query
also returns 8 for both resources. Other factions return unsupported from
this connected human adapter.

```sh
python3 ports/alpha-centauri/native/test_cost_factor.py \
  --out /absolute/new/evidence-directory \
  --original /private/Borehole.SC --original-rules /private/alpha.txt

CXX=/absolute/toolchain/bin/m68k-amigaos-g++ \
  sh ports/alpha-centauri/native/build_cost_factor_probe.sh \
  /absolute/cost-factor-probe
```

The host test passes **40,350 pure cases** against both the exact pinned source
body and an independent integer oracle, plus **49 imported-world cases**
against an independent byte-level oracle. Tests cover all industry branches,
growth bounds and base bonuses, tiny/small categories, multiplier rounding,
AI dependency combinations, legacy resource branches, invalid facts, output
aliasing, human optional-base queries, and owned-state preservation. Host
ASan/UBSan and strict compiler warnings are enabled. The control also builds
as a genuine 68040/FPU Amiga HUNK.

That HUNK also executed in a separate FS-UAE 3.2.35 / AROS 20261023 copy with
the documented disposable libnix FPU40 patch. Its full 40,350-case leaf output
and genuine Boreholes report matched the host outputs byte for byte; both
guest commands returned **0**. Exact binary/input/output hashes and report
sizes are in `COST-FACTOR-EVIDENCE-20261026.json`.

These checks qualify the source port and its tested owned-world calculation
path. **Original-game binary parity and AmigaOS 3.2.3 execution remain unrun.**
This is not a production completion, base growth or End Turn implementation.
