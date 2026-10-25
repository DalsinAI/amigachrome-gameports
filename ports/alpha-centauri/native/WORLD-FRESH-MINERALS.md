# Fresh owned mineral proposal

`world_fresh_minerals` combines the convoy-derived food/intake proposal, pending unit support and the checked original mineral/ecology calculation. It derives mineral intake and signed raw ecology from fresh tile queries, transfers from actual convoy actors, support from saved unit designs and pending SUPPORT, and facility/project/map/faction facts from the owned world. Saved mineral intake is not fed back as fresh income.

The function returns a proposal and its explicit calculation inputs. It preserves all world, wire, rule and runtime RNG storage. Output aliases reject before writes, and all errors preserve the caller's output. This does not mutate accounting or actors, complete production, allocate workers, advance upkeep or turns, grant session authority, or add a player control/save action.

## Supported domain

All inherited stable-worker, convoy and support restrictions remain in force: a stable human worked mask, explicit worker/runtime facts, no specialist/governor allocation, and the qualified pending-support subset. This candidate still rejects fungus harvesting/selected fungus; the separate fresh technology-effects composition is not included. Only matching wire/component editions are accepted: SMAC versions 7–10 and SMAX versions 11–12. Original expansion legacy-save fixups are not implemented here.

SMAX version 12 has eight little-endian tectonic counters in its preserved wire snapshot. Version 11 requires an explicit runtime counter; absence is an unsupported stage, not an assumed zero. Explicit negative counters reject. Base SMAC has no tectonic contribution and rejects a nonzero explicit counter. The global Voice project test deliberately preserves the original `location != -1` behavior, including a destroyed-project sentinel, instead of guessing faction ownership. Other project bonuses resolve actual project base ownership.

## Validation

The public independent Python model decodes synthetic wire containers and compares all 131 explicit output words, including support totals, raw mineral facts and final proposal. Host address/undefined sanitizers and an independent static Linux 68040 run each executed 293 controls: 209 accepted and 84 rejected. Both produce SHA256 `882c681eadb957a9c04f0518ff05e4a299033ac7191d3ae59524fa879cf22575`. Complete owned world/rules/wire bytes and RNG are unchanged; aliases and invalid edition/runtime stages are checked within each probe.

Private bounded execution of both unchanged original mineral helpers, with genuine facility/bitmask helpers, compared the six original base outputs for all 209 accepted fresh input sets (128 SMAC, 81 SMAX). This checks the numeric helper using derived facts; it is not complete original support/yield/mineral or upkeep execution. Original assets and executable bytes remain private.

The inherited 49,747 mineral controls and 558 support contexts passed locally. All 101 inherited workflow entries are retained, with two new composition commands. Final-head CI remains pending until actual logs are inspected.

Two fresh normal Amiga SDK builds are byte-identical at SHA256 `3836a7e6f3fe5ebd342e9bc79d109af7bdd8ca6f588e327fa2af3705f98414c0`. Compilation is not AmigaOS execution: native qualification remains UNRUN and the candidate must remain held.
