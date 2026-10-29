# Checked proposer record stages

These are independently implemented, read-only candidate transformations at two
closed record-write boundaries of the original proposer. They are not allocation,
a complete proposal, a discovery award, or permission to mutate a World.

`prepare_proposer_record` joins a genuine **pre-retirement** record with the genuine
**post-retirement construction** result. The old active bit is captured before the
original retirement call. If it was clear, preparation adds flag0x40. Preparation
always adds0x80 and adds4 when the full signed group argument is-2 or the observed
signed runtime difficulty is below2. It preserves every other flag. A nonnegative
full group argument stores its low byte in group_id (wire0x2b); only the exact full
value10 also stores plan3 (wire0x2a). For example266 stores group10 but does not
change the plan. Negative arguments otherwise preserve the constructed group.
The explicit difficulty is runtime input data, not proof of an active controller.

`store_proposer_automatic_name` requires the qualified, terminated256-byte logical
name. It reproduces strncpy32 followed by NUL at31 and clearing flag2; names of
31 or more characters are safely truncated to31. Bytes after an earlier terminator
are zero-filled. Other record fields are unchanged. An unterminated256-byte input
is rejected transactionally. This API does not invent, select or resolve a name.
SMAX's missing live collision-prefix provenance is still held by its naming API;
this mechanical store API does not satisfy that obligation.

The World wrappers bind actual prior custom id64..511 and check edition/version.
They return a separate candidate, reject output overlap with all World raw and
decoded ranges and other inputs, and leave all owner storage unchanged. Constructed
record/name provenance and session authority remain obligations for a later owning
transaction. Passing a different World to later phases does not manufacture those
facts.

## Original order and remaining dependencies

The original captures inactive state, retires the chosen active slot where required,
constructs against the resulting state, prepares flags/group, resolves its actual
automatic name, stores it, calls prune, and then remaps owned production queues.
In particular, construction availability cannot be computed against a stale
pre-retirement active record. The new0x80/0x40 flags and final name must already
exist when pruning runs. Production remapping tests group_id0x2b, not plan0x2a.
The final last-created cursor update also follows those stages. Existing qualified
history, retirement, construction, naming and restricted human-pruning proposals
remain separate; their whole composition is not qualified by this change.

Source observations are base propose_proto0x597390 and SMAX0x580860, specifically
preparation0x597afd..0x597b46 /0x580f77..0x580fc3 and automatic-store
0x597c3d..0x597c61 /0x581049..0x58106f. No proprietary code, bytes, tables or
original assets are included here. Layout facts use the existing attributed
OriginalPrototype schema. Thinker wrappers' ai_plan argument name does not change
the actual group-id byte write or special plan3 write observed in both originals.

## Validation and limits

`test_proposer_record_stages.py --out OUTPUT` runs728 raw cases and112 owned-world
cases on ASan/UBSan host; `--m68k-linux --cc COMPILER` actually executes the same
cases on independent big-endian68040/QEMU. Signed extrema, negative modes, full
argument10 versus266, byte wraps, old active state, unrelated flags, empty/short/
31/32/141/255-byte names, unterminated rejection, output aliases and unchanged
whole World/input checks are included. Wrong variant/id failures preserve output.
Raw output SHA2561542557fa99e7fd6416d521cff7a631a498e43c7b08124b993b33c4c88c5f0b9;
World output47b8dff77d619aafdb5fcae9b66272a16a7a7b39c9d0eb4e37e36c17e4a9aff6.
`build_proposer_record_stages_probe.sh OUTPUT` builds with the normal private SDK.
Actual AmigaOS execution is UNRUN; no lab/UI/player controls are exposed.

Private controls execute104 original contexts in each edition at each of the two
unchanged instruction fragments, with the real complete254-byte strncpy helper.
Every720896-byte declared global region is compared after each stage, along with
all256 name-input bytes. The driver adds an explicitly recorded observer jump at
each boundary before the unresolved following stages. It does not execute the full
proposer, naming, retirement, pruning, production, discovery, loader or PE startup.
Synthetic record/name/difficulty inputs are declared observations, not claimed live
owner provenance or authority. The initial diagnostic driver clobbered its entry
slot register during input copying and failed; that receipt remains private. The
corrected driver restores the real entry register and both complete sequences pass.
