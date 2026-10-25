# Normal human research candidate pool — draft validation

`world_human_research_pool` combines the owned original `tech_avail` query with
the deterministic candidate-membership part of normal
`tech_pick(faction, 0, -1, ...)`. It reports ordered original technology IDs,
their original categories and a per-ID offered bit. It does not pick a technology,
show a fake popup, consume RNG, update research history or award discoveries.

This branch depends on PR86's `research_options` API. PR86 and the separate cost
PR89 are preserved unchanged. The pool is published as a stacked draft targeting the PR86 branch. It must
remain held until its dependency and actual AmigaOS qualification pass; no
deployment or native qualification is claimed.

## Recovered pool mechanics

The implementation follows the original directed-human candidate decision,
checked privately against instruction-backed original decision execution.
Category comparison is also described by GPL OpenSMACX
`technology.cpp::tech_category` (Brendan Casey, pin
2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c). Pinned Thinker
 dabe62f2d1abba03de40f3dd3048494843f6a9af `mod_tech_pick` delegates the choice
dialog to the original engine; its revised-cost postprocessing is not included.
No proprietary decompiled code or original binary artifacts are distributed.

1. Normal candidate eligibility is original `tech_avail`: known technologies,
   both prerequisites, disabled rows and explicit base/expansion exclusions are
   handled by the qualified owned query. Transcendent Thought88 is repeatable,
   not considered permanently known. Flags such as Secrets are not new bans.
2. Candidates are scanned by original category: growth, technology, wealth,
   power. Within each category original IDs0–88 are scanned in increasing order.
   Rules priorities retain input order power/tech/wealth/growth. Ties prefer
   growth, then technology, wealth and power. Negative priorities remain signed.
3. Once **any** technology has already been offered, a normal candidate is
   withheld when `(ID - saved_tech_ranking + faction_ID) % 3 == 0`.
   The source uses `tech_ranking`, not `earned_techs_saved`, accumulated points,
   Transcendent Thought count or a guessed total of owned technologies.
4. The first offered technology bypasses that rotation gate globally, not once
   per category. This guarantees a fallback offer when eligible candidates exist.
   A category mismatch does not increment the offer count.
5. Original slots0–6 bypass the rotation exclusion when saved `tech_ranking==0`.
   This is an ID/ranking exception, not an invented general rule for every
   prerequisite-free technology. Eligibility still applies to those slots.
6. Future technology88 otherwise receives the normal rotation rule. If it is the
   only eligible technology, the first-offer fallback keeps it available. Its
   displayed repeat number belongs to UI/accounting, not pool membership.

The source applies the rotation condition before category matching. The owned
loop preserves that order and checks original signed32 `ID-ranking` then
`+faction` arithmetic. Unsafe original overflow fails transactionally rather than
silently wrapping. Every successful result is committed at the end; no partial
pool is returned after a malformed context or overflow.

## Explicit paths and unsupported modes

The runtime variant is supplied explicitly to the world wrapper, not guessed
from serialized layout version. The query uses saved faction human status,
`CurrentTurn` and `GameRules` blind-research bit0x200. For normal calls, turn0,
nonhuman or blind-research paths report
`automatic_selection_required` with no fabricated candidate choice. The original
engine delegates those paths to `tech_ai`; that AI/blind choice is not emulated.
Negative turns are outside the supported saved-game context and are rejected.

The pure pool helper takes explicit owned eligibility results from the same
faction/profile plus a typed human context. The world wrapper establishes that
relationship. Callers must inspect the returned path: an automatic path is not a
claim that the faction lacks research possibilities.

Trade/steal choices (`other_faction>=0`), nonzero force-dialog flag, tutorial
requesters, network waits, valuation/random highlighting, exact rendered popup
order/layout, user choice and discovery effects are not implemented. The ordered
IDs describe the deterministic internal pool, not a reproduced complete GUI.
Scenario no-tech-advances gating belongs to the caller/upkeep path; no additional
unsupported policy is silently inserted into original `tech_pick` semantics.

## Controls and present status

Host ASan/UBSan controls cover all categories, signed/tied priorities, varied
eligibility/ranking/faction contexts, starting slots, first-offer fallback,
repeatable-only pools, explicit automatic paths and transactional invalid/overflow
inputs. Independent raw-wire world controls cover legacy/current layouts,
original base/expansion rules and three original base tutorials. Expansion
saved-state contexts are synthetic; no original expansion save is claimed.

Privately, 184 original directed-decision contexts produced 1,693 offers matching
our ordered pools exactly. The reference executes the unchanged decision block
and original `tech_avail`/`tech_category` helpers with owned accept/reject
observation continuations. It does not execute the GUI, RNG/valuation after
acceptance, automatic-selection entry or the full game. The supported original
SMAX executable is exercised with both runtime variant flags; a separate original
base executable GUI differential is not claimed. All original helper artifacts
and original media remain private.

Public CI runs 115 generated raw contexts plus two invalid snapshot controls,
and 64 owned worlds / 512 faction queries, on host sanitizers and independent
big-endian 68040. Final-head CI status must be checked before review completion.
Normal SDK raw/world probes build. Actual AmigaOS execution is **unrun**;
independent big-endian execution is pending the draft CI. The user's watch preview owns the lab;
this workstream has made no lab reads, writes, inputs, starts, stops or restores.
No browser/gameplay wiring or native qualification/merge is implied.
