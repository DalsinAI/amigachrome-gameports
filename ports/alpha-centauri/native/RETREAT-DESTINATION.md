# Original land retreat destination selection

`world_land_retreat_destination` reads the current owned World, unit definitions,
an ordinary nonmissile LAND defender, incoming direction, explicit network fact,
and explicit supply-pod coefficients. It returns the selected direction and
coordinate, plus an inspection-only mask of eligible neighbors. It does not
move a unit, authorize a retreat, run combat, or consume any random stream.

The source enumerates eight staggered-map directions in order. It applies the
original move-ZOC check, LAND terrain, actual pod placement, fungus exception,
and occupied-tile treaty rule before scoring. The fungus project is Pholus
Mutagen (project index 16). SMAC permits predefined negative-weapon designs;
SMAX additionally permits predefined design 15. Custom designs do not receive
that native exception. Entry treaties use the stored tile owner's opinion of
the defender faction. Source direction ties retain the first candidate.

The decoded map importer currently normalizes the raw flat-map word using
nonzero truth while these original routines test bit 0. This candidate refuses
noncanonical raw words outside 0/1; it does not alter frozen map/ZOC parents.
Whole-World stack validation refuses source repair/UI routes. Unsupported
triads, missiles, malformed topology, invalid definitions, and overlapping
outputs, or selected weapon offense outside signed-byte range fail without changing the output or World.

Validation: 38 synthetic Worlds and 5,472 queries pass host ASan/UBSan and actual
independent 68040 execution with identical output. There are 4,032 accepted and
1,440 rejected queries, including 1,280 selected destinations. All eight
eligibility-mask bits occur. Nineteen guard checks per World cover invalid
arguments/definitions, aliasing, unchanged output, and unchanged original bytes
and runtime movement seed. Separate private bounded executions compare the
selected direction for every accepted query (2,016 per edition) against the
original eight-direction enumeration with its
real ZOC, veh_at and goody_at helpers, stopping before relocation. They supply
controlled World/frame facts; they do not execute original startup or battle.

The focused predefined design-15 fungus controls produce no selected
destinations for SMAC and six for SMAX, matching the original routines.

The normal Amiga SDK builds this probe; AmigaOS execution, frontend input,
remote final-head CI, retreat health/countdown progression, legal attack,
stack relocation, events, combat RNG ownership/persistence, casualties, AI and
turns remain unqualified. The existing inherited workflow commands are retained;
unchanged inherited suites have not been rerun wholesale for this leaf addition.
