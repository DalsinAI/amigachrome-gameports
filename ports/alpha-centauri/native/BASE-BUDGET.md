# Fresh Base Budget Projection

`world_base_budget` connects fresh support, worked tiles, convoys, nutrient and
mineral accounting, energy loss, the current energy sliders and specialist /
facility / project effects to an income-and-maintenance projection. It credits
the allocation's **economy** output before calling the original facility
maintenance routine. Raw energy surplus never substitutes for economy income.

The resource and maintenance components retain their existing recovered-source
attribution. Energy distribution is the explicitly documented pinned Thinker
default restoration policy in `ENERGY-ALLOCATION.md`, rather than a claim that
the missing original energy-allocation body has been recovered.

The caller supplies the current running pre-base credit ledger, maintenance
total and player flags, together with fresh commerce and the explicit resource
stage. Maintenance uses the actual current-world mineral cost factor. Turns0
and1 skip maintenance, matching the pinned base-upkeep policy. Custom rules
that yield a zero cost factor are outside this adapter's qualified domain.

The complete proposal and all projected support states are committed together
only after every stage succeeds. Overflow in post-income credits or later
maintenance leaves both outputs unchanged. Input/output overlaps, insufficient
actor capacity and tail guards are checked. No world, credit balance, facility,
specialist, convoy order, runtime seed or dirty flag is modified.

This is an inspection forecast. The authoritative turn engine still needs to
resolve production, growth, workers, riots and trade before applying income.
Stockpile production is excluded; psych is an allocation output, not a fresh
drone/riot determination. An explicit trade input is not trade generation.
The existing supported directed-human stage0 worker restrictions remain.

## Native Interface

The candidate world's existing F popup now also shows fresh economy, research,
psych, facility upkeep and the economy-minus-maintenance balance before trade.
It uses actual current human energy settings, qualified rules, live owned world
and current reserves. The ordinary build keeps its existing read-only entry.
The forecast does not add income, scrap facilities or advance the turn.

## Validation

```
python3 ports/alpha-centauri/native/test_world_base_budget.py --out /new/budget-host
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_world_base_budget_probe.sh /new/BudgetProbe
```

The strict ASan/UBSan host corpus covers56 imported worlds,48 accepted, against
independent wire/integer resource, allocation, original cost-factor and facility
maintenance oracles. It checks rounding carries, low reserves and scrapping,
early-turn maintenance bypass, poisoned saved resource caches, different
sliders, world versions, unsupported stages, late overflow and unchanged
outputs on every rejection. The complete ordered binary output SHA256 is
`46a41ee3b8a1d0d4d75bdb8af174ef981e0700f54eb643aea729f828491ec256`.
The existing72 connected maintenance controls pass after exposing optional
ledger arguments in their independent Python model. Native execution is
recorded separately under `evidence/alpha-budget-20261027`.
