# Manual research choice: pure field transition (draft, native unrun)

`manual_research_choice` implements only the original manual research-change
post-chooser field transition. It accepts an explicitly supplied confirmation
stage and chooser result, returning the resulting target/labs/cached-cost state
and a disposition. It neither creates a chooser nor invokes synchronisation.
There is no world mutator, frontend control, eligibility query or discovery.

This operation differs from original `tech_selection`, which assigns the normal
picker result without imposing the manual-change labs penalty. The manual
caller `Console_set_tech` first requests confirmation when labs are nonzero and
the old target is nonnegative. After confirmation and a real picker return:

| Outcome | Target | Accumulated labs | Cached cost |
| --- | --- | --- | --- |
| Confirmation declined | unchanged | unchanged | unchanged |
| Negative chooser result | unchanged | unchanged | unchanged |
| Chosen target equals prior target | unchanged | unchanged | unchanged |
| Prior target negative, chosen target valid | chosen target | unchanged | unchanged |
| Existing nonnegative target changes | chosen target | old labs /2 | unchanged |

Division truncates toward zero, including negative saved labs and INT32_MIN.
Original `synch_research` follows every picker return, including cancel. The
result reports that unresolved requirement; no network or synchronisation
callback executes. Declining the earlier confirmation does not request that
synchronisation boundary.

The input can alias `output.state`. Invalid stage values and positive choices
above88 leave output unchanged. Original post-choice machine code accepts any
nonnegative result; limiting this API to original technology IDs0..88 is an
explicit safe supported-domain guard. All negative results retain original
cancel semantics. A declined confirmation ignores the unused choice value.

## Inputs and omitted integration

The caller must establish a genuine picker result and confirmation provenance.
The pure function does not prove local-player authority, faction, variant,
offline mode, directed research, pool membership or a fresh world revision.
Those caller/GUI boundaries are not yet fully qualified, so this addition does
not guess an owned-world transaction. In particular, AI/blind/turn-zero paths
must not be converted into an invented player offer or fake choice.

No ownership, technology ranking, discovery count, earned advances, base labs,
RNG or other world fields are changed. No original world or wire snapshot is
written. Eligibility and offered-pool membership, stale-request detection,
synchronisation, versioned world mutation/persistence and gameplay/UI exposure
remain separate integration gates. Existing research eligibility/pool/cost
work is not a dependency of this standalone arithmetic transition.

Cached cost remains unchanged; Thinker's revised-cost post-picker recalculation
is a mod policy. Original `tech_research` resets labs and invalidates/recomputes
cached cost only through its discovery/accounting call flow, which is not
implemented here. This addition must not be used to simulate discovery/upkeep.

## Original evidence and public controls

The implementation was written independently from instruction-backed original
field semantics. Private controls execute exactly41 unchanged original bytes
from the **post-chooser** manual-change block0x51508a..0x5150b2 in the owner's
supported original SMAX executable, with an externally supplied choice. They
stop before `synch_research`; the chooser, confirmation GUI, full caller,
AI/network, discovery and whole game are not executed. No proprietary code,
executable bytes, original assets or private helper artifacts are distributed.
Pinned Thinker dabe62f2d1abba03de40f3dd3048494843f6a9af
`tech.cpp::mod_tech_selection/mod_tech_pick/tech_research` supplies public call-flow
context; its patched policies are not silently substituted for original rules.

Public tests generate2,486 cases covering signed extrema, same/changed/unset
states, cancellation, both confirmation stages, invalid IDs/stages, output
aliasing and transactional failures. An independent arithmetic model compares
every result; truncated input is rejected by the probe. Private expanded host
controls include4,552 supported original post-choice contexts, whose target,
labs and cost exactly match actual original slice outputs. A further504 original
contexts use unsupported positive IDs and are excluded from that comparison.

```sh
python3 ports/alpha-centauri/native/test_research_choice.py --out /tmp/research-choice-host
python3 ports/alpha-centauri/native/test_research_choice.py --out /tmp/research-choice-m68k \
  --cc m68k-linux-gnu-g++ --m68k-linux
CXX=m68k-amigaos-g++ sh ports/alpha-centauri/native/build_research_choice_probe.sh /tmp/research-choice-probe
```

Normal SDK build passes. Host sanitizers pass7,038 combined public/private
contexts. Independent big-endian CI is required for the draft. Actual AmigaOS
execution is **UNRUN**, no lab access occurred, and no merge is authorised by
this evidence. See `RESEARCH-CHOICE-EVIDENCE-20261025.json` for exact hashes/scope.
