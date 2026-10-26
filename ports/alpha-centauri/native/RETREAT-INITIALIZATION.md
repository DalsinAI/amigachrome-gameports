# Owned retreat initialization

Read-only ordinary nonmissile land/land initialization derives both current
speed(...,1) values, defender health/order/state, actual tile bits, validated
linked-stack count, and edition-correct effective Comm Jammer/Dissociative.
SMAC Jammer blocks retreat; SMAX alone allows the defender Dissociative exception.
FREEABIL grants use actual design-owner technology, never actor faction guesses.
Only the once-initialized threshold is returned: ceil(current defender health/2).
No countdown, destination selection, retreat relocation or battle action exists.

Conservative scope: inherited speed requires reactor1..4 and edition-valid
ability bits. Air/missile paths, malformed same-coordinate stacks and mismatched
technology tables are refused. Physical base tile bit requires exactly one matching decoded base; stale
or duplicate base records are refused. Original base_at enters mandatory repair/UI
on stale bits, which this read-only stage cannot silently replace. Negative signed plan bytes retain their
original byte interpretation for the artifact comparison.

Public controls cover32 synthetic Worlds, all actor pairs, flags0 and1, health,
speed differences, Jammer/Dissociative raw and technology-granted conditions,
orders/state, bunker/airbase/base bits, paired stacks, and disconnected-stack
refusal. Original full stack_check(mode1,-1,-1,-1) has529 unchanged normal returns
per edition with all851968 mapped globals unchanged; this is the count-all path,
not other stack modes. The corrected current binding additionally matches2650
actual original initialization paths:544SMAC+2106SMAX. Full original base_at
normal returns supply genuine base presence, followed by the unchanged425/407byte
caller prefix and actual speed/morale/ability/technology/stack/range helpers.
All851968 mapped global bytes remain unchanged. One observer jump stops before
the next battle phase; no helper bytes or callbacks are replaced. This qualifies
the scoped initializer, not complete battle execution or action authority.

All owning World/rules/table storage aliases reject before output writes. Saved
facts do not supply active battle authority. No CRT state is read or consumed,
no unit/world/stack mutation, UI, turn or full-game completion is claimed.
Normal Amiga SDK builds only; native and remote final-head CI remain UNRUN.
