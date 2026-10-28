# Owned research eligibility query

`world_research_options` reads a loaded `OriginalWorld`, qualified technology
`Rules`, an explicit runtime game variant, and faction slot 0–7. It returns all
89 original slots with known state, mechanical availability, both missing
prerequisite bits, repeatability and a diagnostic reason. It also copies the
saved current research ID, accumulated research, cost and Transcendent Thought
count. These accounting fields are snapshots; costs are not recalculated.

It never chooses, awards or advances research, changes the world, consumes RNG,
executes original callbacks, or offers a fake next turn. Unsupported saved
research IDs remain visible and are marked unsupported rather than remapped.

## Recovered mechanics and boundaries

The source is pinned OpenSMACX `technology.cpp::has_tech` and `tech_avail`
(2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, GPL-3.0-or-later), checked against
pinned Thinker `tech.cpp` (dabe62f2d1abba03de40f3dd3048494843f6a9af, MIT).
Those eligibility bodies agree; none of Thinker's revised costs, technology
balance bonuses, AI scoring or selection patches are used. The implementation
calls the existing qualified `world_has_technology` rather than adding another
interpretation of owned technology state.

* Known technologies are unavailable for research. Ownership comes from the
  original saved `TechOwners` bitset, not the faction's trade-source history.
* Both prerequisite technologies must be known. `None` succeeds for faction
  slots 1–7; `Disable` fails. Planet (slot0) cannot research.
* Slots78–86 are excluded in base-game mode even when a modified rules file
  enables them. IDs retain their original slot semantics.
* Slot88, Transcendent Thought, is deliberately never considered known by the
  recovered `has_tech`; it remains repeatable once its prerequisites are met.
  Its accumulated repeat count remains the saved faction field.
* Slot87 is ordinarily disabled by its prerequisite rows, not by a newly
  invented hardcoded ban. A custom enabled slot follows the original lookup.
* The original `has_tech` asymmetry is preserved: a `None,Disable` row can be
  considered owned when its ownership bit is set, but cannot itself be
  researched. Consequently it can satisfy another technology's prerequisite.
* The runtime variant is explicit. Save `data_version` describes serialized
  layout and is not the original unsaved `ExpansionEnabled` flag. Callers must
  supply their selected game/rules profile. Both legacy and current wire layouts
  are tested under both explicit modes; no implicit component/ID remapping occurs.

**This is `tech_avail`, not the final human research menu.** Original
`tech_pick` additionally filters choices using faction/research history
(including a modulo-three gate), and applies category/selection behaviour.
Blind research, AI rankings, selection history, trade/steal eligibility, network
synchronization, cost calculation and discovery side effects remain separate.
No claim is made that every mechanically eligible candidate would appear in a
particular original research-choice dialog. Alive/human status is not a new gate:
this query mirrors `tech_avail` for the requested saved faction slot.

Diagnostic reasons are an owned UI contract, not recovered engine return codes.
All input references are checked before output replacement. Invalid world,
faction, variant, table count or prerequisite references fail transactionally.

## Verification and present status

`test_research_options.py` generates legacy/current saved-state contexts,
ownership patterns and explicit base/expansion modes. Every resulting candidate
matches test-only retained OpenSMACX function bodies with owned-data adapters,
and an independent Python reference reads the original wire ownership/accounting
fields directly. Tests check output reasons, both prerequisite bits, repeatable
state, accounting snapshots, seven invalid transactional cases per world, and
unchanged original wire bytes/runtime RNG.

Optional `--rules-data VARIANT RULES` applies original rules to synthetic owned
states. `--original VARIANT RULES SAVE` checks actual original saves. Original
assets remain private. The supplied control build script uses the normal SDK,
with no overlays or fake library/Exec stubs.

This workstream is **host-only** while the user's watch preview owns the lab.
A successful normal-SDK build is not native execution proof. Native query/UI
qualification and actual independent big-endian CI remain required before a
qualified merge/deployment. The query is not yet wired into the browser or a
working research turn.
