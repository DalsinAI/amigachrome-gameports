# Fresh original pacifism count

`world_pacifism` obtains the current counter from actual decoded actors and map
facts. It does not read an assumed or saved pacifism total. A unit must belong
to the selected base's faction, have that base as home, have unsigned plan < 10
and nonzero signed weapon offense. Negative psi offense counts as combat.
Unhomed, foreign, zero-offense and noncombat-plan units do not contribute.

A qualifying unit counts once if either condition holds:

- It is outside a valid base and the original territorial comparison does not
  return its own faction.
- Its chassis is air and its plan is not Air Superiority (plan 4).

The original valid-base test is the map base bit together with val2's low
nibble < 8. This can exempt an ordinary combat unit in any such base, including
foreign bases. A base bit with an invalid low nibble does not itself exempt it.
Air Superiority exempts only the aircraft arm of the condition; an aircraft
outside its own territory may still count. Overlapping reasons count once.
Clean Reactor affects mineral support, not this count. Pending POLICE and drone
immunity affect later happiness/flag handling, not counter membership.

With null base-output pointer and ignore-communications false, original
whose_territory returns the querying faction only for a positive tile owner
matching that faction. Foreign communication/game flags can switch a foreign
answer between -1 and that foreign owner, but cannot make it the querying
faction. The query uses this exact comparison equivalence without invoking
callbacks or inventing runtime flags. Planet faction zero never matches an
owned-territory result. All nonpositive signed owners remain unowned. A reached
positive owner > 7 rejects instead of accessing undefined diplomacy memory.
That restriction is not applied when the original valid-base short circuit
makes territory irrelevant.

The result contains the count plus home-combat/away/air/overlap diagnostics.
All actual unit-component dependencies are checked; irrelevant reactor and
armor fields are not required. There is no heap allocation. Errors and overlap
with the complete definitions, World object, raw container or any decoded
allocation preserve all output and inputs. The world's records, wire snapshot
and runtime RNG remain unchanged. The fixed 2,048-unit domain bounds counters.

This is a pacifism dependency proposal, not full base_support or upkeep. It
neither clears/assigns actor support/pacifism flags, spends minerals, refreshes
faction social projections, commits happiness/accounting, establishes authority,
executes a turn/AI, writes a save nor exposes a player control. Full original
support observations qualify only their explicitly documented counter domain;
other source writes are not qualified by this component.
