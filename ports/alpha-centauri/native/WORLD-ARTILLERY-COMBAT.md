# Owned artillery modifier binding

`world_artillery_combat` resolves actual loaded actors/prototypes and chassis
triads, actual current map climate bytes and current land-gun/altitude rules.
Both actual component profiles and coordinates validate before output. Explicit
battle flags must select the supported bit0x2 artillery domain in0..31;0x18
skips modifiers. Bit0x1 remains a separate earlier bombardment flag. Incoming
strengths remain explicit: this is a numeric inspection, not a legal artillery
order/range decision, active battle, inferred normal/psi mode or live authority.

The immutable scalar stage applies land-versus-sea gun bonuses, excluding
altitude even for zero gun rule, or the higher side's actual level-difference
bonus for other pairs. No actor/footer seed is used as RNG. This binding and its
scalar stage write no World, cache, label, UI or action state. Every World and
actual input-table output alias rejects; technology is not a stage input.

Host ASan/UBSan and actual independent Linux68040 each pass54 synthetic Worlds,
78,732 queries and1,080 guards. All9chassis/3triads, every map altitude level and
lower climate bits, six explicit modes2/3/6/10/18/26, zero/positive/negative rules,
signed strength extremes and invalid modes/components/aliases are covered.
Both architectures' output SHA256 is
f14c706609230579814ed63ed44cb47a90c47cc3ba605b967809fa8101b0f14c.

Private tests run the SAME actual binaries on all3 owner-original tutorials
(Explore, Build, conquer), another2,628 queries/60guards per architecture,
with independently decoded Python expected facts. Original scenario and rules
files remain byte-identical; owned runtime RNG stays unchanged. Private output
SHA256 is799b1f1701f4839c13502f5f8c5b50d84483a7648e08871619cdb33b72c64171.
These are owned World inspections on original data, not execution of original
game orders, attack legality or whole battle_compute. Parent2,430 complete
original paired-baseline battle observations retain their distinct scope.

Two normal SDK builds reproduce byte-identically. BUILD ONLY; actual AmigaOS,
remote final-head CI, input/UI, damage/casualty/RNG application, AI, upkeep/turn
and full-game playability remain unqualified. The human watch is untouched.
