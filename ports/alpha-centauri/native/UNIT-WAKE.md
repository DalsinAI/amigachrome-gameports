# Original unit wake command

This local candidate implements the original `veh_wake` order cancellation as a
checked world proposal and a revision-bound local session command. It provides
no player input, renderer callback, phase completion or end-turn authority.

The original signed order byte must be4..23 and state bit0x04000000 must be clear
before speed is evaluated. Spent movement becomes the low byte of speed minus
the road rate. Nonzero terraforming work loses the original contribution, with
a zero floor. Super Former uses mask1 (bit0): SMAC tests raw design flags; SMAX
also resolves eligible design-owner FREEABIL0 grants with original technology
prerequisites. The actor owner determines project ownership. Xenoempathy Dome
(project index8) doubles the rate for terraform10/11; Weather Paradigm (index2)
otherwise multiplies by3/2. These indices are not facility IDs.

The original alert exception uses auto-type11, state0x200 and no state4, setting
spent movement to zero. Every wake clears order and applies state mask0xf4ffbdff.
Other actor fields, world RNG and accounting remain unchanged. Crawling, orders
outside the signed range, and zero work preserve the original short circuits.
The inherited checked speed domain rejects values above255 before mutation;
SMAX Tower19 speed0 preserves the original subtraction wrap.

Prepare/commit uses private session identity, revision, local human ownership,
suspension and alias checks. Commit recomputes the proposal before writing any
field. Changed commands increment revision and report repaint/persistence
obligations. These reports do not claim that rendering or saving occurred.
Repeated wake is idempotent; reload and address reuse invalidate old commands.

Validation of this source:

- 864 synthetic worlds across save versions7..12: 798 accepted,30 speed overflow
  and36 foreign-owner rejections,18,084 session guards on each architecture.
- 24 additional SMAX Tower19 byte-underflow contexts,528 guards per architecture.
- Independent Python compares every exported container byte, reload, continued
  idempotent commands and RNG preservation. ASan/UBSan host and independent
  Linux68040 execution produce identical outputs.
- Private execution of complete original wake routines:120 contexts per edition,
  plus24 SMAX Tower contexts, normal returns and all786,432 mapped global bytes
  compared. Actual reached speed, morale, ability, contribution and SMAX
  technology helpers execute without fake callbacks or imports. Both owned
  architectures match all264 complete52-byte actor observations.
- All462 inherited unit-stat cases pass on each architecture.
- Two fresh normal Amiga SDK builds are byte-identical. AmigaOS execution is
  **UNRUN**. No remote CI or publication is claimed.

Source: OpenSMACX GPL3-or-later, Brendan Casey2013–2021,
commit2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, `contribution` and `veh_wake`,
checked privately against the owner's original SMAC/SMAX helpers. Owner media,
executable fragments and private differential corpora are not repository files.
