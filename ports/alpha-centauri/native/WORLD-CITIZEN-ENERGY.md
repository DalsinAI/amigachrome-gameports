# Current world specialist facts

`world_citizen_energy` connects the checked original citizen definitions to the
current decoded technology-owner masks. It returns an explicit edition/faction
tag and seven `CitizenEnergyFacts` records for the complete energy component.
It does not read saved energy intake or cached specialist availability.

Both tables must be qualified from the same rules file. The citizen table's
edition tag must match the world wire profile: versions 7 through 10 use the
base profile, versions 11 and 12 the expansion profile. This is the existing
importer's wire-profile boundary, not a claim to perform original save fixups.
The function resolves both prerequisite and obsolete technologies through the
existing original `world_has_technology` helper. Faction zero has no technologies,
including None. None is available for factions 1 through 7. Disable and Future
Tech index 88 are unavailable. Ordinary technologies use the current owner bit,
with the original None/Disable prerequisite asymmetry. Signed bonuses are
copied unchanged, including values outside normal stock rules.

The output is transactional. Invalid table counts, edition, faction or technology
IDs reject without changing it. Output must be disjoint from both complete
tables, the World object, its raw container and every decoded allocation;
partial overlap also rejects before writes. The query allocates no heap memory.

No specialist selection or replacement, research award, cache/accounting write,
commerce/rank calculation, garrison ability binding, authority, persistence,
upkeep, turn, UI or AI operation is performed. The seven fact records can feed
the complete explicit energy component only after its other current facts are
also established. Original helper semantics were qualified in prior work; these
new composition controls compare actual decoded wire input with an independent
Python model, not execution of the full original energy/world pipeline.
