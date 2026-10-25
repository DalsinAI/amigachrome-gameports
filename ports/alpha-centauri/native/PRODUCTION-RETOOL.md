# Original production-switch preview

`production_retool` implements original `base_making` classification and
`base_lose_minerals` arithmetic from explicit context. The saved-world wrapper
`world_production_retool` previews an explicit proposed queue item using the
base's actual facilities, prior-production ID and `minerals_accumulated_2`, plus
the saved human-faction mask. Current accumulated minerals and saved calculated
mineral intake are not substituted for that original retool baseline.

This is a read-only preview, not build eligibility, a queue write, actual mineral
spending, production completion, player order, worker allocation, upkeep or turn.
A classified item is not certified buildable. The wrapper rejects unsupported
legacy-to-expansion contexts rather than inventing original fixup callbacks.

Original base-game repeatable facilities start34 and projects39; expansion
starts65 and70. Actual Skunkworks bit10 forces category mode when strictness is
positive. The later OpenSMACX FREEPROTO override is a patch absent from the
original helpers and is not added here. An already-completed ordinary facility
in the proposed queue returns the original -1 classification before strictness
handling. Strictness0/1/2/3 preserves original branches; original file parsing
clamped strictness to2, whereas this explicit API and the existing separately
source-qualified BasicRules reader also accept the documented3 option. This is
not a claim that the unchanged original text loader accepted3.

Rules are strictness0..3, penalty0..100%, exemption0..1000. A queued INT_MIN
rejects unsafe original negation. Signed32 overflow in the original active
penalty product rejects transactionally; safe inactive/zero-penalty paths remain
accepted. Result is replaced only after complete success. No native struct dump.

Host ASan/UBSan and independent Python pass93339raw cases:91966accepted,
9invalid,1364overflow. Both unchanged original executables' classification/loss
and actual bitmask helpers match every accepted output triple:45992base and
45974expansion contexts. No callback stubs, queue mutation, PE startup or full
game execution. The owned binding passes744synthetic contexts across versions7
through12 and21previews on the three original owner tutorials, files unchanged.

`build_production_retool.sh NEW_DIR` builds both probes with the normal Amiga SDK.
The raw probe reads44byte explicit LE test records (profile, strictness, penalty,
exemption, human, previous item, proposed item, mineral baseline, three facility
words) and emits status/previous-class/queued-class/loss as four LE words. This
is test transport, not an original save format. The world probe accepts profile,
game, strictness, penalty, exemption, proposed item and reads saved base facts.
Actual AmigaOS and final-head independent big-endian CI remain required before
merge. No player controls are exposed while those gates remain pending.
