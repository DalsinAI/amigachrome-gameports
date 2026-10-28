# Fresh technology effects for resource production

`world_technology_effects` adapts the complete recovered OpenSMACX
`technology.cpp::tech_effects`, original offset `005BAE60`, at
`2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c`. It recomputes fungus resources and
commerce bonuses from actual technology ownership, parsed technology flags,
faction bonus entries and pending economy. Saved cache fields are ignored.
The fourth fungus slot remains zero, as in the source. Negative resource
totals clamp after all faction bonuses; commerce receives the three original
economy-threshold additions. Signed intermediate overflow rejects.

This is a read-only proposal, with output aliases into the world, original
wire, decoded records or rules rejected without writes. It does not award
technologies, refresh every technology-related engine cache or advance a turn.

`world_fresh_tile_yields` computes these effects and replaces the fungus fields
in the existing tile context before using the same resource leaves. The older
`world_tile_yields` saved-snapshot query retains its existing behavior. Fresh
worked-tile intake and exterior convoy harvests use the new path on fungus.
They still require valid directed-human worker assignments and the existing
inspection-stage guards; this does not bypass an allocator or upkeep gate.

Validation uses an independent raw-wire/Python integer model: six original
container versions, every faction, all four technology bonus flags, disabled
technologies, negative faction bonuses, pending-economy boundaries, corrupt
bonus counts, stale cache poisoning and overflow. The host sanitizer run
passed 980 queries over 98 worlds, including untouched original Boreholes.
The genuine 68040/FPU HUNK batch ran the same corpus in FS-UAE/AROS with RC0;
all 23,520 result bytes matched the independent oracle. Exact hashes are in
`TECHNOLOGY-EFFECTS-EVIDENCE-20261026.json`. The batch probe's standard-library
manifest scanner additionally needs the genuine AROS math libraries; the
calculation itself contains no floating-point operations.

The intake and convoy suites include successful fungus cases with deliberately
negative saved cache values and late technology-effect overflow rejection.
Existing saved tile-query controls remain unchanged. No comparison against an
unchanged original executable or AmigaOS 3.2 runtime has been performed here.
