# Original basic combat strength composition

This read-only stage composes original prototype strengths, faction psi factors,
corrected human/opponent-specific native morale and physical-base effects into
get_basic_offense/get_basic_defense results. It does not run battle_compute,
choose a legal attack, damage or kill units, draw RNG, or execute a player turn.

Important original behavior retained:
-Base game has only the Creche branch. SMAX can add Brood on top of Creche;
 the later source else-if rewrite is not used.
-Attack Creche uses active SE morale clamp-4..4; defense Creche uses-4..0.
 Brood-only uses-4..4. Values<=-2 are raised by one before subtraction.
-Pending morale2..3 and a native opponent add in-base bonuses. Original fifth
 offense argument is explicit offense_mode; its bonuses/gas/clamp are preserved.
 No claim that this argument grants or determines a legal artillery duel.
-SMAX Soporific/Dissociative use actual qualified design-owner FREEABIL grants
 and original technology prerequisites. Base ignores these expansion effects.
-Original HQ has no added defense morale. Tests deliberately include its bit.
-Artifact defense and unarmoured Probe versus non-Probe defense return1 after
 morale calculation; unnecessary psi defense arithmetic is not executed.
-Offense mode false may report morale below0 or above6 exactly as original;
 the final product is checked at each original signed multiplication boundary.

World binding requires real actor IDs, edition-correct component tables and
technology facts. Stored map base-bit lookup finds the first matching physical
base; a stale set bit without a base rejects instead of pretending to perform
original rebuild/log/UI callbacks. Original missing opposing-actor unsafe reads
are excluded. Existing checked component-profile restrictions are retained;
unsupported base ability bits reject rather than expanding the old speed/morale
binding's domain. Version11 native versus human still needs absent tectonic
runtime and fails closed as documented in native_combat_morale.h.

The query reports the intermediate offense/defense morale values. It does not
install original global cache state. Original offense_mode true writes cache1,
then defense overwrites cache1 while cache0 stays unchanged; that ordered cache
lifecycle remains an obligation of a future real battle context.

Final local validation:
-48398 raw contexts:24304 accepted,14 invalid,24080 checked overflow;120890
 alias/output guards. Host ASan/UBSan and independent Linux68040 agree.
-54 synthetic Worlds/34992 paired queries, including raw and FREEABIL gas/wave,
 technology ownership, Probe/Artifact/native, Creche/Brood/HQ and all8 explicit
 mode combinations. Both architectures agree;594 guards include stale map bits,
 null-profile/technology, source aliases and unchanged original bytes/runtime RNG.
-Complete unchanged original basic strength functions plus all real reached
 helper routines return normally for3744 SMAC+4032 SMAX contexts (15552 top-level
 calls). Both owned architectures match all31104 strength/morale observations.
 All786432 mapped global bytes compare, including exactly the original ordered
 battle-morale cache writes. No patched instructions, callbacks or imports.
 These are explicit paired synthetic World contexts, not original battle/turn
 startup, unsupported stale-map execution or full legal-combat qualification.

Early World controls included unsupported base high ability bits and correctly
rejected them; the fixture was corrected without broadening production input
validation. A later oracle omitted Probe improvements after granting T0; that
independent-model error was corrected and the original/host/BE controls rerun.
The final fixtures also decouple custom/predefined profiles and plans. Earlier
partial/preliminary directories are retained, not final-head qualification.
The SDK raw probe initially warned about a directly type-punned alias guard;
its uintptr_t observation helper now retains the same guard without suppressing
warnings. Final exact SDK artifacts are build-only; native and remote CI UNRUN.

No player controls, saved-state mutation, filesystem save, terrain battle
modifiers, damage/RNG/kills, AI, diplomacy or playable-game claim is added.
