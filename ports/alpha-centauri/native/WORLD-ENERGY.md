# Fresh current-world income proposal

`world_energy` connects fresh tiles and satellites, actual convoy actors and
mineral support, original commerce ranks with fresh technology bonuses, owner
citizen definitions, physical facilities/projects, headquarters distance and the
complete five-phase happiness calculation. It returns resource, commerce and
income/social proposals. It never writes accounting, actors, faction caches,
RNG, production queues or original bytes.

The resource provider currently requires a valid directed-human worked mask,
with no specialists or governor reassignment and no selected unavailable tile.
Those cases reject and still need a genuine worker-allocation pipeline. The
complete numeric energy component supports specialists, but this current fresh
world provider does not qualify positive-specialist full-world execution.
No automatic allocation, support side effects or turn execution is supplied.

The stage is explicit: source edition, expansion-enabled branch, worker/upkeep
inspection context, transient scratch, projection stage0/1, offline validated
garrison, original current-first-energy commerce profile and alien scratch
policy. Stage1 only proposes diagnostic/faction accumulators. A save does not
establish runtime authority. Physical project IDs retain original index meaning.

Fresh energy uses freshly calculated intake channel2 and actual convoy channel2.
Stockpile uses freshly calculated mineral surplus, not saved accounting. The
current first energy field remains the original input to commerce ranking and
valuation; this layer explicitly preserves that phase instead of recomputing
all bases' rank inputs. All eight technology-commerce bonuses are recomputed.
Two inefficiency calls retain separate diagnostic accumulation, and the faction
surplus accumulator uses the first pre-commerce surplus. Headquarters scans
same-faction physical HQs, uses the original wrapped vector metric and its999
scan sentinel, and falls back to16 exactly when the sentinel remains.

Happiness fact extraction is shared internally with `world_psych`. Its public
interface still accepts explicitly fresh spending; `base_energy` assigns its
newly calculated spending before happiness. No placeholder spending is consumed
as a runtime fact. The temporary ordered garrison lives in separate heap storage
through the complete numeric call. No self-referential result pointers escape.

Alien paths that consume uninitialized original stack scratch reject under
`reject_uninitialized`. `initialize_zero` is an explicit deterministic port fix,
not an original-semantics claim or an engine default. The caller must select it.
All buffers are sealed against aliasing with every input and world allocation;
all errors preserve outputs. Heap failure is reported; no injected allocator
failure test is claimed.

Local host ASan/UBSan and independent Linux68040 validation each ran259 worlds,
1036 base/stage/policy queries:910 accepted,32 requiring allocation,68 invalid or
unsupported and26 overflowing. All21756 guards passed on each architecture.
The corpus contains192 public synthetic worlds,64 synthetic worlds with private
owner rule tables and the first base of all three owner tutorials (12 accepted
stage/policy queries). All278 returned scalar fields matched independent wire,
resource, convoy, commerce, citizen, integer-income and happiness models.
The saved second intake and mineral-surplus fields were deliberately poisoned
in the synthetic worlds; fresh results still came from resources. Coverage
includes672 nonzero-commerce queries,262 local HQs,648 no-HQ fallbacks,
32 positive fresh stockpile conversions and30 accepted alien queries.
Combined output SHA256:
`3f8bfe65a3aa9a4e31c950e8dc83b55cd25f378133fb0a53eaf991c59b971839`.

The exact internal HQ scan separately passed128 world controls on both
architectures: own/foreign or absent HQ, flat/wrapped maps and vector distances.
SHA256 `45f0b1c9dfdcb1ba880cef91959a31afa549a24ba444ed80629ab079e6721afc`.
The shared psych extraction reran259 worlds on both architectures and retained
all100 happiness/social fields and diagnostics exactly. Its output remains
`c7af10cb577146ce5ae39d9de28729e4dd41c384e73a57a9831f3f5c6b7cb81f`.
The unchanged complete energy component reran1568 controls on both architectures,
retaining `7c1ce3ea59ac2c47c14bb15f6f596c501bfd40fb6f5a8d2bb884d53af632c5f6`.

No new complete original executable world-to-energy comparison was run. Original
bounded-helper receipts belong to separately frozen dependencies and are not
native qualification of these changed probes. Normal SDK energy builds were
byte-identical:
`7b5440744e9dba35004d22de3e706248646231df4e5e17c56da52501a4912164`.
Refactored psych probe builds are separately recorded. AmigaOS execution, full
workflow/CI, publication, owning-session commit, accounting/specialist/faction
persistence, worker allocation, production completion, turns, AI, UI and full
game playability remain unfinished. The user-owned watch remains exclusive.
