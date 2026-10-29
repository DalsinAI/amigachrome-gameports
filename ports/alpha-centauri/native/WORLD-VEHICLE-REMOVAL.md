# Owned current-world removal frame

`stage_vehicle_removal` captures the current decoded World, then applies the
frozen checked removal storage leaf to the private frame. Success replaces the
whole output plan; rejection preserves the caller World, supplied runtime facts
and any existing output. Current `vehicle_count` and decoded `VehCount` decrease
together. Runtime cursor/lift references and a copy of the removed record belong
to the resulting plan. Further removals can be staged into another plan.

Capture copies current actors, tiles, factions, bases, prototypes, faction
definitions, preferences and other decoded metadata. Current monument/history
bytes receive separate ownership. All borrowed wire spans are rebased into an
exact owned copy of the original file; they remain immutable baseline. Use
`frame()->vehicle/tile/faction/...` for current records. Low-level wire readers
on `frame()->game()` still read preserved baseline slices, not staged actors.
No variable-length file encoder or original-executable reload claim is made.

Memory budgets include the plan and current arrays. The checked leaf validates
stack topology before changes. Input/output aliases, missing or unsupported
runtime observations, invalid IDs, inconsistent current counts and low memory
reject without replacing the prior result. Self-replacement using the output's
own frame as input is held; stage from that frame into a different plan instead.

Synthetic controls deliberately change current damage/state, a tile contour,
faction energy, prototype/faction-definition text, base flags, monument bytes
and an independently supplied history buffer without altering the original
container. They verify current-state capture, logical count consistency,
second removal, output replacement, low-memory failure, caller/runtime
preservation and survival after clearing the source and overwriting its bytes.
Save versions7–12 and counts1/2/9/2048 are covered on host and independent68040.

This is owned staging. It does not establish legal casualty/attack authority,
commit to a running session, execute `kill`/`battle_kill` rewards or caller tails,
initialize/persist combat CRT state, save to disk, acknowledge events, run AI or
advance a turn. Offline/log-disabled/invisible-window runtime facts are supplied
explicitly within the source-qualified storage profile. Native execution and
frontend integration remain unrun.
