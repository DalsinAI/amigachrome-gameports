# Saved-design speed abilities

The owned-world speed query now resolves faction-granted speed abilities before calling the qualified saved-speed arithmetic. Land movement pricing uses the same resolver. `world_speed_ability_flags` preserves raw design bits and adds only the grants queried by original `speed_proto`: Slow and Antigrav for all triads, plus Air Superiority and expansion Fuel Nanocells for aircraft. This is not a general ability API.

The owner is **prototype ID divided by 64**, even when the vehicle belongs to another faction. Original predefined designs 0..63 never receive faction grants. A raw bit bypasses its own prerequisite lookup. A FREEABIL bonus requires that design owner's prerequisite technology, using actual owned technology-owner masks and original None/Disable semantics. Transcendent Thought 88 is not treated as a normally owned prerequisite. Unrelated Deep Radar and Nethack bonuses cannot answer a speed-bit query.

The implementation adapts GPL OpenSMACX `veh.cpp::has_abil`, `speed_proto` and `speed` at pin 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c (Brendan Casey, 2013–2021). The source reference binds real owned metadata/technology bytes, not canned callbacks. Portable outputs are independently checked from raw serialized fixtures in Python. No proprietary decompile or executable bytes are included.

Malformed FREEABIL count/ability indexes and missing technology context fail before replacing output; original undefined indexing is not reproduced. Needed technology facts are resolved even when morale is skipped. When no unresolved relevant bonus exists, skipping morale retains the existing lazy behaviour and does not require an otherwise unused technology context. The original byte-speed overflow rejection is retained.

## Validation and current gate

Host ASan/UBSan passes 462 synthetic contexts: 444 accepted and 18 rejected. Controls cover both component profiles, all speed grant combinations, all three triads, design-owner versus vehicle-owner technology, direct raw bits, duplicate grants, None/Disable and disabled/future prerequisites, malformed metadata, partial-grant-then-error transactionality, existing morale/damage/projects and byte-speed overflow. Newly queried synthetic faction-bonus counts are explicitly initialized; the old fixtures' previously opaque poison is not valid bonus metadata.

All 248 land movement-price regression contexts pass. The three actual original base tutorials retain their original independent accounting digests and files unchanged. The public synthetic controls contain no owner assets; expansion saved-state contexts are synthetic.

Privately executed unchanged SMAX `has_abil` and `has_tech` helpers match 8,788 speed-ability queries across 444 accepted controlled worlds, with actual fixture metadata and prerequisite masks. This compares the four ability bits, not the complete original speed/morale routine or an original base-executable helper. No PE startup, imports or full game is executed.

Normal private SDK probes compile. Independent big-endian CI and actual AmigaOS execution remain pending. The user owns the current watch preview, so no lab writes/inputs/start/stop operations occurred. This remains draft until native qualification. Historical native evidence for the earlier stats artifact does not qualify this changed binary.

This change does not mutate worlds, consume RNG, expose legal movement, relax clear-land action gates, advance turns or implement combat/AI. Existing runtime component-profile checks remain; no silent base/SMAX ID remapping is added.
