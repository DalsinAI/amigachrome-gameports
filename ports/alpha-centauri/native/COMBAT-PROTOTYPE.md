# Original combat prototype strengths

This candidate restores the original `offense_proto`, `armor_proto` and
`psi_factor` arithmetic with checked typed inputs, plus read-only bindings to
actual saved-world actors, prototypes, faction rules and project ownership.
It does not resolve a battle: morale, social/terrain modifiers, attack legality,
combat RNG, damage, kills, AI and player command execution remain separate work.

Prototype queries require an actual opposing prototype. They deliberately omit
the originals' unchecked negative-opponent reads. Returned conventional and psi
values have the original eightfold in-battle scale. Raw signed weapon/armor byte
facts and byte missile/mode facts are checked; caller tables must match the
edition and psi ratios must be1..1000. Errors preserve output, and output storage
cannot alias the source tables, prototypes or world allocations.

Recovered details retained:

- Probe weapon mode11 against opposing plan11 returns16 for either rating.
- Psi offense uses the opponent's triad; psi defense uses the queried unit's
  own triad. Differing land/sea/air ratios are preserved.
- SMAX prototype15 has the original Spore Launcher exceptions. The same numeric
  slot in SMAC does not acquire those expansion rules. Bombardment, signed
  offense/armor and Spore conditions are evaluated in the original order.
- Missile strength compares the signed raw offense byte with99 before applying
  the3/2 multiplier to its magnitude. This preserves negative-byte edge cases
  rather than a later source rewrite's comparison after absolute value.
- Faction psi percentage and project ownership use the actor's faction, not the
  prototype design owner. Neural Amplifier is project index9; Dream Twister19.
- SMAX Fungal Tower19 defense applies a second50% bonus after Neural Amplifier,
  with separate integer rounding. SMAC has no tower bonus. Attack uses Dream
  Twister and ignores the tower flag. Division truncates toward zero.
- Original overflowing signed32-bit intermediate products/additions are outside
  the checked API domain and are rejected before output; no clamping or claimed
  wrapped-overflow equivalence is supplied.

Validation of these leaves:

- 23,520 public contexts on ASan/UBSan host and independent Linux68040, identical
  output:23,496 accepted/24 rejected prototype contexts;23,130 accepted psi
  factors,2 invalid variants and388 arithmetic overflow rejections.47,454 guards
  per architecture include output preservation and table/prototype aliases.
- Actual unchanged original helpers:11,556 SMAC and11,552 SMAX contexts,
  69,324 normal-return calls. Each context compares all786,432 mapped global
  bytes unchanged. Both owned architectures match all69,324 returned values.
  This executes complete closed helpers, without callback/import stubs, binary
  patches or PE/game startup. It is not execution of full battle resolution.
- Saved-world bindings:72 public worlds and10,848 paired queries across save
  versions7..12, plus the owner's three original tutorials/292 queries. Both
  architectures match an independent Python model, with525 guards each. Source
  containers and owned RNG seed remain unchanged.
- Two fresh normal Amiga SDK builds for each probe are byte-identical. AmigaOS
  execution and remote CI are **UNRUN**; no player controls are exposed.

Source: OpenSMACX GPL3-or-later, Brendan Casey2013–2021,
commit2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c, checked privately against both
original editions. Original instruction behavior supersedes later mod fixes.
Owner media, executable fragments and differential inputs remain private.
