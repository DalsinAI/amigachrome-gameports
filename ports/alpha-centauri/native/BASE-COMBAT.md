# Conventional base combat stage

`base_combat_adjustment` restores the normal-combat (`combat_type0`), non-psi,
physical-base stage of original `battle_compute`. Incoming strengths must already
include preceding faction/terrain calculations. The result still requires later
sensor/ability and other battle stages. It is not a final battle calculation.

The attacker's triad selects Perimeter/Citizens, Naval Yard/Maritime or Aerospace.
Cloudbase supplies the air defence only in SMAX. Their multiplier is4 instead
of2; Tachyon adds2. Blink suppresses the ordinary base-percent branch and caps
the multiplier at2. Without an attacker, Tachyon does not increase the final
multiplier. Infantry-versus-base requires faction>0, land chassis speed1 and
(custom prototype>=64 OR nonnegative raw weapon and prototype!=15). Division
occurs after the signed multiply, toward zero. Every original int32 intermediate
is checked before an output write. Applied flags describe numeric branches,
not additions to the original four-entry presentation ledger.

`world_base_combat_adjustment` resolves a real defender tile/base, physical
facilities and actual project ownership. Citizens uses the defender actor's
faction; Maritime/Cloudbase use the physical base owner. These can differ in
inspection contexts. Effective Blink uses the actual prototype owner and actual
prerequisite technology in SMAX; SMAC is raw-only. The incoming strengths remain
explicit arguments, so the caller cannot mistake this stage for a qualified
complete battle. No authority is derived from the imported world.

Host ASan/UBSan and independent Linux68040 pass44,034 raw controls each:
30,498 accepted,12 invalid,13,524 checked-overflow rejections,101,604 guards.
World controls pass57 synthetic worlds,20,520 queries and969 guards per
architecture, including all9 chassis, mixed actor/base/project owners, effective
Blink grants with known/unknown prerequisites, absence, negative strengths and
overflow. Sources/World RNG remain unchanged; all owned storage/table aliases
reject before writes. Three extra grant controls extend the initial54 worlds.

Private tests execute COMPLETE unchanged original `battle_compute` at SMAC
516390 and SMAX501da0, returning normally in972 contexts per edition (864
facility/Blink/Creche/percentage contexts plus108 project contexts). These
controlled numerical contexts put the defender on a sea tile to bypass sensor
lookup dependencies. Land-on-sea numerical inspection is not a legal-attack
claim. Both owned architectures' actual stage outputs, composed with independent
basic/terrain inputs, match all56 recorded bytes per original call: final
strengths, modifier counts/values and morale caches. Original basic/morale,
prototype, ability, bitmask, base lookup, Strings_get and reached CRT routines
execute without patches, fake callbacks, imports or PE startup.

The original harness compares851,280 unchanged global bytes in SMAC and851,272
in SMAX. Named battle/cache/string ranges are permitted to change. Numeric
outputs are compared; label string buffers are not. A synthetic bounded label
table supplies real Strings_get/CRT inputs. Metadata was clarified after the
run to distinguish the851,968-byte mapped region from unchanged-byte counts;
execution/binary/output bytes did not change.

Normal SDK binaries build reproducibly but AmigaOS/native execution and remote
final-head CI remain UNRUN. No command/input/UI, strength application, legal
attack, damage/RNG/casualties, AI, turn or full-game claim is added. The watch
remains exclusively user-owned.
