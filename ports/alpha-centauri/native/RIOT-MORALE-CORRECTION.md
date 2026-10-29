# Retain original riot morale ordering

Both owner originals clamp the morale adjustment to0..6 immediately after the
riot decrement, before adding the actor's raw rank. OpenSMACX deliberately
removed this early clamp as a gameplay fix. The earlier recovery imported that
change; this candidate restores the original ordering.

For a rank3 ordinary unit with SE morale-4, rule morale0, a rioting home base
and no Creche/immunity, both originals return morale3. The previous recovery
returned0. A rank6 unit in the same conditions remains elite in the originals;
this also changes the dependent elite movement-speed query. No state mutation
or player action is granted by either query.

The correction changes only the riot branch in world_effective_morale, its
adapted source reference and its independent raw-file oracle. Non-riot,
homeless, immunity, predefined psi, Probe and Planet paths retain their scope.
The dedicated new controls include ordinary custom, basic psi and SMAX Spore
profiles, signed division, Creche/Brood, immunity, home/no-home and rank0..255.

Final local checks:
-100 synthetic Worlds/900 actors:1800 morale+1800 speed queries, host ASan/UBSan
 and independent Linux68040/QEMU agree exactly. Saved bytes/RNG are unchanged.
-Complete unchanged original morale_veh+real bitmask helpers execute450 actor
 contexts per edition,900 calls each. All786432 mapped global bytes stay equal;
 both owned architectures match all1800 returned morale values exactly.
-New corpus does not requalify Probe/Planet original paths; inherited462 unit
 stats/ability controls pass on both architectures with the corrected original
 reference and independent oracle.
-Frozen parent actually reproduces260 differing morale and70 differing speed
 results across38 of the100 final worlds. Its source remains unchanged.

First focused controls correlated Brood with a profile that could not exercise
its branch; final fixtures decouple those facts and actual SMAC/SMAX outputs
now differ as expected. These preliminary passes remain private history.
ENOSPC interrupted the next runs before completion; final3 directories are
complete reruns after verified local storage recovery, not partial-run passes.

All final SDK builds are build-only. Actual AmigaOS and external final-head CI
are unrun. Previous PR93/derived hashes retain their historical scope, but do
not qualify this changed artifact or establish original riot morale fidelity.
No UI/turn/AI/fullcombat/game completion claim.
