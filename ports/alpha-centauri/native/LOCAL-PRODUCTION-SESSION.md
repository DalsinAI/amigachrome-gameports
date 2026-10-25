# Owned offline local facility orders

`LocalProductionSession` is an explicit new offline planning controller. It owns
its imported world and parsed technology, basic/world and facility rules. No
mutable world/rules handle is exposed. Both the saved map owner and saved active
turn faction must equal the caller-selected human, whose saved human-mask bit
must be set. These are import constraints; the controller separately owns its
live planning/suspension state. It does not pretend to restore the original
network, AI, modal or turn-processing runtime by inventing zero globals.

Loading is transactional: bad rules/world/edition, inactive/nonhuman control,
insufficient memory or unqualified world dependencies leave the previous world,
rules, phase and revision intact. Explicit runtime edition and expansion flag
remain separate from wire version. Original text-loader strictness0..2 is
retained here; the standalone retool API's optional strictness3 does not change
that original loader policy. The selected rules are copied into owned tables;
changing the caller's buffers after load cannot alter an issued order.

Facility orders are limited to the selected human's base, an unlocked base state
and exactly one production queue slot (`queue_size==0`). Multiple/empty slots,
unit production choices and unsupported item IDs remain outside this API. The
original facility predicate uses queue_count0 for this first-item replacement;
it does not fabricate queue shifting/compaction. Availability, all actor/ordinal
bounds and checked original retool arithmetic are resolved before any write.

`prepare_facility` issues a private plan containing a preview and the issuing
session identity/revision. Commit revalidates against the same owned world.
Copied, unissued, cross-session, suspended, reloaded, closed and address-reused
session plans cannot authorize a different state. A process-lifetime identity
counter and monotonically checked revision are controller bookkeeping, not
world RNG, serialized game fields or guessed original network authority. One
owning engine thread is required. Unrelated modal handling must suspend this
controller and invalidate outstanding confirmations; no GUI is wired here.

For a changed facility target the atomic commit performs the observed original
local `base_change` core effects: queue_items[0] becomes the negative facility
ordinal; an old unit item's faction units_queue byte decrements modulo256; and
minerals_accumulated becomes minerals_accumulated_2 minus the qualified original
retool loss. The baseline and production_id_last remain unchanged. Retooling is
not repeatedly subtracted from current minerals. A same-item plan is an
idempotent no-op, even if the already-selected target has since become
unavailable. Cancel means discarding a plan, without a commit.

No new unit item is offered, so this boundary does not increment a new unit
counter or claim unit-production eligibility. The original general core's
counter increment and 99999 sentinel are documented separately in private
reference evidence; neither broadens this facility-only API.

A changed commit returns its new revision plus repaint/persistence requirements.
No-op receipts request neither. The receipt is an owned engine event, not a fake
call to original draw_radius or remote project notifications. No construction
completion, resource yield/upkeep, advance turn, AI, diplomacy or campaign
completion is implemented. Existing frontends do not instantiate this class.

**Persistence is still pending.** Live records are inspected through
`world()->base/faction/...`; the owned original bytes and wire spans remain the
import baseline. A movement-only snapshot encoder rejects these changed base
and faction records. This class offers no player save/filesystem commit, and
its in-memory order tests do not claim original-executable save compatibility.

## Evidence checkpoint

Host ASan/UBSan and independent raw-wire Python modeling pass153 contexts:
5,280 initial order attempts,904 accepted plans (858 changed orders and46
no-ops),4,376 rejected plans, and855 additional consecutive different-order
commits. Each first attempt resets to its fixture; these are controlled tests,
not a claim of1713 played campaign turns. The consecutive checks prove live
queue continuation, one-time old unit-counter removal, unchanged mineral
baseline/last-production category and re-evaluation from that baseline.

The public default is150 synthetic contexts across wire versions7..12:
4,992 initial attempts,892 accepted plans,4,100 rejected plans,845 consecutive
order checks. Output SHA256:
`135991bfb0559704367f780cc635a499655005c45401edfda97e0861070d9207`.
Three owner tutorials add288 attempts,12 initial changed orders and10 further
consecutive orders; files remain unchanged. Full private output SHA256:
`deeddeb6d294da39ce1b486c933a7f683a0ee879dca1c2828abda474abc0b12a`.

Within-process snapshots check every base/faction/vehicle/tile/prototype/faction
definition, the complete game view, original bytes and RNG. Only the two intended
base fields and one old unit-counter byte may differ on a changed first order;
the second facility order may change only its two base fields. These native
object-representation comparisons are within one process, not save encoding or
cross-endian struct dumps. Test output uses explicit LE integers.

Additional controls cover failed loads preserving a live session; private token
copy/replay, cross-session misuse, suspend/resume, reload/close/address reuse;
empty sessions/maps; argument bounds; changing caller-owned save/rules buffers;
nonlocal humans and mismatched active factions; queued/locked bases; unsupported
references and original arithmetic overflow. Imported strictness3 is tested in
a project-to-project case that distinguishes the original clamp-to2 behavior.

Before implementation, actual private original `base_change` cores were
observed for6699 base and6696 expansion contexts. Complete base records and all
512 unit-queue bytes match independent expected effects. Execution stops before
draw_radius via one instrumented boundary jump; original no-op epilogues and real
base_making/bitmask dependencies execute. Rendering/foreign notifications are
neither executed nor replaced with callback stubs. No PE startup or full game is
run; no original executable bytes/tables/assets are shipped. Original state
semantics also correspond to Thinker's MIT mod_base_change (pin
 dabe62f2d1abba03de40f3dd3048494843f6a9af), with original numeric behavior retained
through the separately qualified GPL retool and availability components.

The normal installed AmigaOS SDK builds the final sequence-control probe:
`ba7ce871dd39a79d9043031a428c436e96c23d9edf5c6f97c8e66873d0e7d215`.
Actual AmigaOS execution is UNRUN; independent final-head CI is pending at this
committed checkpoint. The user-owned watch preview is untouched. Early controls
precede lifecycle/active-faction/sequence coverage; host-final-7 is authoritative.
An initial driver syntax error and a later test-driver indentation warning were
corrected before this final pass, with private history retained.
