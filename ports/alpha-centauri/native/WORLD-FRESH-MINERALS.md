# Fresh owned mineral proposal

`world_fresh_minerals` combines the convoy-derived food/intake proposal, pending unit support and the checked original mineral/ecology calculation. It derives mineral intake and signed raw ecology from fresh tile queries, transfers from actual convoy actors, support from saved unit designs and pending SUPPORT, and facility/project/map/faction facts from the owned world. Saved mineral intake is not fed back as fresh income.

The function returns a proposal and its explicit calculation inputs. It preserves all world, wire, rule and runtime RNG storage. Output aliases reject before writes, and all errors preserve the caller's output. This does not mutate accounting or actors, complete production, allocate workers, advance upkeep or turns, grant session authority, or add a player control/save action.

## Supported domain

All inherited stable-worker, convoy and support restrictions remain in force: a stable human worked mask, explicit worker/runtime facts, no specialist/governor allocation, and the qualified pending-support subset. This integration uses fresh technology-effects reconstruction for selected and harvested fungus. Saved fungus caches are not authoritative, and no cache is committed. The legacy snapshot-only tile query remains available unchanged. Only matching wire/component editions are accepted: SMAC versions 7–10 and SMAX versions 11–12. Original expansion legacy-save fixups are not implemented here.

SMAX version 12 has eight little-endian tectonic counters in its preserved wire snapshot. Version 11 requires an explicit runtime counter; absence is an unsupported stage, not an assumed zero. Explicit negative counters reject. Base SMAC has no tectonic contribution and rejects a nonzero explicit counter. The global Voice project test deliberately preserves the original `location != -1` behavior, including a destroyed-project sentinel, instead of guessing faction ownership. Other project bonuses resolve actual project base ownership.

## Validation

The public independent Python model decodes synthetic wire containers and compares all 131 explicit output words, including support totals, raw mineral facts and final proposal. Host address/undefined sanitizers and an independent static Linux 68040 run each executed 421 controls: 337 accepted and 84 rejected. Both produce SHA256 `8020367034f8fa4fcd465ba603f465620a2efa884b5a8b1711ab67b75a007478`. Complete owned world/rules/wire bytes and RNG are unchanged; aliases and invalid edition/runtime stages are checked within each probe.

Private bounded execution of both unchanged original mineral helpers, with genuine facility/bitmask helpers, compared the six original base outputs for all 337 accepted fresh input sets (192 SMAC, 145 SMAX). This checks the numeric helper using derived facts; it is not complete original support/yield/mineral or upkeep execution. Original assets and executable bytes remain private.

The inherited 49,747 mineral controls and 558 support contexts passed locally. The combined workflow retains all 111 entries in the union of the mineral and fresh technology branches. Final-head CI remains pending until actual logs are inspected.

Two fresh normal Amiga SDK builds are byte-identical at SHA256 `e280cdbb6481cbb98446f643bbf0215c7f9b99823d0ac8d18c8060f01d3a6dd9`. Compilation is not AmigaOS execution: native qualification remains UNRUN and the candidate must remain held.

The new cases vary Planet ratings as well as technology flags and faction fungus bonuses, while poisoning saved fungus caches. The first comparison exposed an inherited Python convoy-model assumption of neutral Planet; the model now decodes the actual pending Planet/economy/base-count facts. Production C++ was unchanged, both initial failures are retained privately, and the complete final runs pass. The frozen non-fungus mineral candidate has its own earlier hashes and is not relabelled as this integration.

A private composition also executed 48 coherent worlds with two real facility orders each, readonly food/mineral queries, exact complete buffer export/reload and same-item continuation on host and independent 68040. Both outputs match `0e2590c783408f7b4e5e45fee0935fbb81cb22fa0c3b3d266b956d1ef0e0af02`. The returned queue ID changes with the real order; all other controlled non-orbital proposal fields remain stable. Two fresh SDK session builds match `06be210b24f79383ae048f88a6faffec90c75fb93586f68eefde617936b2d27c`; actual AmigaOS execution is UNRUN. This is buffer persistence, not filesystem atomicity or a player save command.
