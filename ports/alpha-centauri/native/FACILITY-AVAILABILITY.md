# Original facility availability predicate

`facility_available` is a pure predicate over a successfully parsed, matching
facility table and explicit caller-owned facts. It distinguishes SMAC ordinals
(72 slots, repeatable34, projects39..71) from SMAX (134 slots, repeatable65,
projects70..133). Runtime expansion enablement is separate from file version.
It does not validate unit production, install a queue, charge minerals, complete
production, advance turns or provide player/session authority. No UI is added.

Inputs include normalized original technology availability/ownership facts,
physical facility bits, the first 0..10 production queue entries, difficulty,
scenario rules, faction rule flags, project ownership/state, and an optional
base. `caretaker` means the original case-insensitive faction filename check;
it is not inferred from alien flags or leader sex. `coastal` means the original
adjacent-ocean predicate, excluding the center tile. A map screenshot or the
center tile alone is insufficient. No world binding guesses these facts.

Projects distinguish unbuilt(-1), destroyed(-2) and a built faction owner0..7.
Physical/queued facility queries exclude repeatable orbital facilities, as in
the original helpers. The absent-base path preserves the original early return.
The availability check rejects malformed metadata, boolean technology facts,
ordinals, queue bounds and project states without replacing the output.
Caller-owned inputs are const; the predicate does not allocate memory.

Original quirks are retained: Ascent checks Voice state != -1 and scenario
victory enablement before technology/ownership/Caretaker checks. Destroyed Voice
therefore satisfies that first condition. The later OpenSMACX Caretaker-Ascent
fix is excluded. Punishment Sphere checks Paradise Garden; the Garden's later
reciprocal exclusion is excluded. Redundant project rules and base/expansion
satellite ordinals remain edition-specific. This is original behavior, not a
mod's revised production policy.

The GPL-3-or-later implementation adapts the factual predicate structure from
OpenSMACX 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, base.cpp::facility_avail,
checked against each original executable's instructions. Owner executable bytes,
original tables and assets are not included.

## Validation checkpoint

`test_facility_availability.py --out NEW_DIR` compares 27,946 controls against
an independent Python set-based oracle: 27,902 accepted contexts,44 invalid
rejections. Coverage includes every ordinal, prerequisite/dependency changes,
built and queued facilities, project ownership/destruction, coastal constraints,
difficulty, scenario flags, absent bases, SMAX runtime enablement and Caretakers.
Host ASan/UBSan passes. Output SHA256:
`d3ec1c2937eb99885da20ba4202d9434cc4b7c4a20e0c1fbfff6dbf5c388bb24`.

A private bounded reference actually executes the **complete original SMAC
facility_avail**, plus actual has_fac, bitmask, has_project and is_coast helpers.
All 10,976 accepted SMAC contexts match (43,904 result bytes, SHA256
`9e8f8600034c72498821ad65a19a19fe331a6e486d8d24976e0f5a5ea77ced14`).
Bases, queues, project indices, normalized technology records and land/ocean
maps are explicit controlled fixtures. No callback stubs, PE startup, Windows
imports or full production/game execution occur. The SMAX full original routine
has a Windows character-case import and has **not** been executed by this
reference; its branches are source/instruction checked and covered by portable
controls. This evidence does not qualify either original GUI production menu.

The normal installed AmigaOS SDK builds the standalone probe, SHA256
`4855e983cea2d3b7ea5f010d9335440a1e626bf0c23a6ef02f5c1f53a626aac2`.
Actual AmigaOS execution is UNRUN; independent big-endian CI is pending at this
checkpoint. The existing user-owned watch preview is untouched. No merge or
native/player availability claim is made. Test transport is explicit little
endian fixed256-byte records and8-byte status/result rows, not an original save
or portable in-memory struct dump.
