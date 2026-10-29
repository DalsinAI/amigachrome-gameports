# Current-world happiness proposal

`world_psych` binds the complete five-phase `base_psych` dependency to an owned
original world. It derives population, specialists, assimilation, difficulty,
human status, pending efficiency/talent/police and the selected economics model,
physical facilities, original project ownership, first NODRONE bonus and
WORMPOLICE. Pacifism is freshly counted from actual actors; the garrison retains
validated original stack order and edition-correct double-police grants.

`WorldPsychContext` explicitly selects the offline validated snapshot profile,
upkeep projection stage0/1 and newly allocated psych spending. These are caller
facts, not authority inferred from a save or fabricated original runtime globals.
Saved psych totals are not fresh spending. Damaged stacks and unsupported facts
reject; this layer never repairs them or substitutes zero counters.

The quick_root implementation follows OpenSMACX general.cpp, Brendan Casey,
GPL3, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
The map factor is `quick_root(width/2*height)` from recomputed dimensions. Both
original map-init paths assign that value; saved cache bytes cannot override it.
The82 social slots start40 bytes before the8x9 psych matrix: the last two saved
queue entries, eight social-support entries, then72 psych entries. Stage1
proposes changes only to those72 entries; the preceding ten remain intact.
This uses explicit decoded fields and never casts native layouts or wire bytes.

The proposal and all source objects/allocations remain unchanged on errors.
The result contains five diagnostic fields and100 original happiness/social
fields. Signed overflow rejects instead of relying on machine wrapping. The
caller owns same-edition qualified technology, unit and basic-rule tables.
All input allocations are protected against output aliasing. The temporary
2048-entry garrison uses heap storage; allocator failure is reported, but no
injected allocator test is claimed.

Local validation ran259 worlds on host ASan/UBSan and independent Linux68040:
818 accepted base/stage queries,166 unsupported/malformed rejections,
32 overflow rejections and4291 guards. This includes192 public synthetic
worlds,64 synthetic worlds with private owner rule tables and the three original
owner tutorials. All24 tutorial base/stage queries were accepted. Every output
byte matched independent wire extraction, the existing independent garrison
and pacifism models, and the independent five-phase integer model.
Combined output SHA256:
`c7af10cb577146ce5ae39d9de28729e4dd41c384e73a57a9831f3f5c6b7cb81f`.

Private unmodified original quick_root helpers each executed73459 calls,
including all0..65535 inputs and square boundaries through the supported maximum
map tile count8388608. Both freshly compiled owned architectures produced the
same results, SHA256
`3394ff6d5b65d1fa23e4f0bff5032f80698b18053deeef5bffb87bab693f2393`.
No original map-init or new full world-to-original psych composition execution
is claimed. The full original psych, ability and support evidence belongs to
its separately frozen dependencies; it does not qualify a changed binary.

Two normal SDK builds were byte-identical, SHA256
`36181a4d554be7662ba7da042639a194f82c3e873a227ad48c110e41d9912cc1`.
They were compiled only. AmigaOS execution and CI are UNRUN. This local component
has no PR/publication, frontend controls, session/accounting commit, persistence,
upkeep execution, turns, AI or complete-game claim. The user-owned watch and
source-export permission gate remain in force.
