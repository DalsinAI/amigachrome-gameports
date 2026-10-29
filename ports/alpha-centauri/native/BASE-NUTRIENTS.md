# Original nutrient accounting from fresh intake

`base_nutrients` calculates the original nutrient accounting from explicit facts.
Growth scratch starts at pending social growth, gains two for physical Children's
Creche facility 2 and two for the actual Golden Age flag. Incoming convoy food
is added to intake; consumption is population times the citizen requirement,
plus outgoing convoy food. Surplus is intake minus consumption. Nonnegative
surplus clears negative stored food, while a deficit changes only stored zero
to -1. It does not add surplus to storage or perform population growth.

Only upkeep stage 1 adds surplus clamped to 0..99 to the faction nutrient total.
Other explicit stage values leave that total unchanged. No live authority or
upkeep execution is implied by supplying this scalar. Population is limited to
0..127 and the requirement must be nonnegative. Each original intermediate is
checked for signed32 overflow, including growth bonuses, multiplication, convoy
addition, surplus subtraction and the faction total. Overflow rejects rather
than relying on the original machine's arithmetic wrap or C++ undefined behavior.
Inputs and output must be disjoint; failed calls preserve output.

`world_fresh_nutrients` first computes the stable directed-human fresh intake,
then derives population, pending growth, physical Creche, Golden Age, stored food
and faction total from the owned snapshot. It reads the actual BasicRules
citizen requirement. It never takes the saved base intake as an input. Incoming
and outgoing convoy amounts remain explicit transient facts; there is no convoy
engine or inference from stale totals. Its result contains the complete fresh
intake proposal and the subsequent nutrient accounting proposal.

The composed query uses the same explicit worker/upkeep stage for both calls.
The existing worker inspection supports stage zero only, so its faction total
is preserved. It does not invent a stage-one runtime to enable the aggregate
update. Unsupported governors, assignments, specialists and selected fungus
retain the parent query's explicit rejections. Every world, rule and stage
storage range must be disjoint from the composed output. The world, original
container, decoded records, runtime RNG and all inputs remain unchanged.

Private evidence calls the complete unchanged nutrient helper and real bitmask
helper in each owner executable for 2,624 contexts per edition. All bytes of the
selected base and faction records, plus growth scratch, match an independent
integer model. This does not claim all other globals were comprehensively
compared. There are no patched instructions, replacement callbacks, imports,
PE startup or full-game execution. The safe accepted domain is compared directly
with both actual originals; rejected overflow observations remain documented.

Public controls contain only synthetic scalar facts, rules and complete version
7–12 worlds. They test physical versus queued facilities, stored-food boundaries,
growth overflow, partial aliases, adjacent buffers, explicit stage behavior and
whole-input preservation. Private world controls add unchanged owner tutorials
with their original rules. Run `test_base_nutrients.py --out NEW_DIRECTORY` and
`test_world_fresh_nutrients.py --out NEW_DIRECTORY`; add
`--cc m68k-linux-gnu-g++ --m68k-linux` for independent big-endian execution.

The matching build scripts use the normal Amiga SDK. Actual AmigaOS execution
remains unrun. No world mutation, worker allocation, population growth, production
completion, upkeep, turn, AI, filesystem persistence or frontend control is added.
