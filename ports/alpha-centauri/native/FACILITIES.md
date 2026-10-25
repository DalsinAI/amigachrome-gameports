# Original facility and project definitions

`read_facility_definitions` reads the original ordinal tables into caller-owned
storage:71base rows (slot0unused,72slots) and133expansion rows (134slots).
Repeatable items start34/65 and secret projects39/70 respectively. It retains
names/effects, signed32 raw costs/maintenance, prerequisite IDs and project AI
priorities in file order fight,power,tech,wealth,growth. This is not build
eligibility, item pricing, queue mutation, completion, upkeep or a turn engine.
Duplicate/empty display names do not change ordinal IDs.

The actual original loaders resolve the declared free-technology column, then
unconditionally overwrite its effective value with Disable(-2). The later
OpenSMACX enhancement enabling that column is deliberately not imported. Both
values are retained separately for transparent data interpretation; no automatic
free facilities are awarded by this parser. Faction-granted facilities remain
a separate rule path.

Facility descriptions use literal semicolons. The original loader consumes one
effect token and ignores remaining annotation columns. Project priorities still
follow that comma token. A scoped literal-token reader preserves these facts;
existing technologies/units keep their previously qualified comment behavior.
Unknown technology keys fail with an owned error instead of an original GUI
popup. Fixed row counts, bounded strings/lines and signed32 numeric parsing are
explicit safety boundaries. Required project priorities must be present.

No allocation or original global/string-table addresses are used. The caller
must provide qualified technology definitions and inspect output only on
Error::ok; failure leaves slot_count0 with partial fields unqualified. The output
object is about48KiB: use caller heap/static storage, not a tiny platform stack.
Costs and maintenance are raw signed values, not proof a negative-cost mod item
can be safely built.

Final host ASan/UBSan plus independent Python compare every output byte in
254synthetic controls (30accepted,224rejected), covering fixed counts, row
truncation, both editions, literal descriptions, ignored tails, configured versus
effective free tech, string/numeric bounds and invalid prerequisites. Two private
owner inputs import all204rows exactly and remain unchanged. Existing technology
and157unit-definition regressions pass. Full original loader execution is NOT
claimed; pinned GPL source and original loader/text-item instructions were
inspected to distinguish original behavior from enhancements.

`build_facility_probe.sh OUTPUT` builds with the normal Amiga SDK. The probe
accepts profile and rules path; output is explicit LE test transport: four32bit
header words then name128/effect192/ten32bit values for each slot. It is not an
original save format. Actual AmigaOS and final independent big-endian CI remain
required before merge. No original data is committed and no player UI is added.
