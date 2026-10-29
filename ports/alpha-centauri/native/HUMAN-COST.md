# Original human nutrient and mineral factors

`original_human_costs` computes the original HUMAN faction factors from explicit
rule multipliers, map-size setting, pending social Growth/Industry, Children's
Creche and Golden Age facts. It is a pure query for both original editions.
It does not infer whether a faction is human, bind saved-world/global settings,
price a specific production item, allocate workers, advance growth or produce it.
AI difficulty/ranking/great-satan adjustments remain outside its scope.

The original zero result on tiny multipliers/small maps is retained. Industry
-7 through -3 uses 130%; values below -7 follow the original default half-factor
branch, as do values above4. Growth bonuses are added before the -2..5 clamp.
The input domain is rule multipliers1..100, map setting0..6 and signed32 social
values whose Growth bonus addition is representable. Invalid input or overflow
leaves output untouched. Bounds keep all later products representable.

This is deliberately separate from `calculate_human_cost_factors` in the earlier
colony testbed, which documents its patched minimum-one result. Neither function
is silently substituted into a full original game loop.

Host ASan/UBSan and independent integer Python pass9841cases:9828accepted,
8invalid,5overflow. Privately executing both unchanged original cost-factor
helpers plus their actual bitmask helpers matches every accepted nutrient and
mineral pair, including both base bonus flags. No callback stubs, AI path,
PE startup or full game are executed. No original instructions/assets ship.

`build_original_human_cost.sh OUTPUT` builds the normal Amiga SDK probe.
The probe consumes six explicit little-endian signed32 inputs per record
(nutrient multiplier, mineral multiplier, map size, Growth, Industry, flags
bit0Creche/bit1GoldenAge) and emits status/nutrient/mineral as three LE words.
Failure retains sentinel0x31415926 in both outputs. This probe format is test
transport, not an original save format. Actual AmigaOS execution and independent
big-endian final-head CI remain required before merge. No player controls added.
