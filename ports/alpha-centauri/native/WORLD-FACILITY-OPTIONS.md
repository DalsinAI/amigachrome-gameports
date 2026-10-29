# Saved-world facility options

`world_facility_options` joins the original facility predicate to an imported
owned world. The caller supplies a successfully parsed matching technology and
facility table, explicit runtime edition/expansion enablement, faction1..7,
base index (or -1 for the original global predicate) and queue_count0..10.
Output lists every ordinal with zero-filled unused slots and the derived
adjacent-ocean fact. Failure preserves the previous output.

The binding reads the saved difficulty and scenario rules, faction definition
rule flags/filename, normalized technology ownership, physical/queued base
facilities and original SecretProjects base references. Built project ownership
comes from the referenced base's actual faction, not the query's faction.
Unbuilt(-1) and destroyed(-2) stay distinct. Invalid project references fail;
they are never treated as unbuilt. The existing qualified world_has_technology
rules supply disabled/prerequisite/ownership semantics, including Future Tech.

The edition is checked against raw data-version ordinal families; legacy-to-SMAX
loading/fixups are not invented. Runtime ExpansionEnabled remains an explicit
caller fact for a compatible expansion-format world. A base from a different
faction may be inspected, matching the original predicate. This API grants no
ownership, active-player or modal authority. Likewise queue_count is the original
caller's parameter, not an assumed interpretation of saved queue_size.

Coastal access reads the eight original adjacent positions from the owned map,
with horizontal cylinder wrapping or flat-map boundaries, valid parity/bounds,
and ocean altitude <0x60. The center tile is excluded: a base on an ocean tile
alone does not establish a water neighbour. No screenshot-based terrain guess
or guessed camera transform is used.

SMAX Caretaker detection compares the original faction filename to CARETAKE with
ASCII case folding. Non-ASCII filenames fail explicitly because this portable
boundary does not qualify Win32 CharUpperA locale behavior. Base-game predicates
do not inspect filenames. Leader sex and alien flags are not substituted for
this filename condition. The existing predicate preserves its documented
original Ascent ordering and Garden/Punishment asymmetry.

This remains a **read-only saved-state query**. It does not place or complete
production, debit minerals, award technology, change a turn, establish an active
session or add player UI. No world/raw-byte/RNG mutation is performed. The
mineral retool preview, current turn accounting and local order authority remain
separate requirements for a production command.

## Validation checkpoint

Host ASan/UBSan and independent raw-wire Python decoding pass 301 worlds and
18,753 complete option queries (16,575 accepted,2,178 rejected). This includes
298 synthetic worlds across versions7..12 and the three owner's original base
tutorials (315 additional queries); original files remain unchanged. Each world
also executes six invalid-argument preservation controls and one empty-world
preservation control. Wrong runtime profiles, base/expansion combinations,
invalid difficulty/project references and non-ASCII SMAX filenames are covered.

Public default corpus: 298 worlds,18,438 queries, output SHA256
`562e2ee183d7cf1bd893663c2bcd30bf6ae9aa3bcc16b36ad902460b0a45fdcb`.
Private corpus including original tutorials: output SHA256
`e1ebe4a4b9b1ee70e0ae9bdf43f0f14ccdb45cc6af26cec278201f9683025891`.
The program verifies unchanged owned original bytes and world RNG after every
query, plus exact output preservation on rejected calls. Individual binary
records use explicit little-endian transport, not native struct dumps or a new
save format. Public tests contain synthetic fixtures only.

Private original is_coast helpers were actually executed separately for each
edition with the synthetic maps and original radius-offset data: 280 base and
280 expansion cases match all boundary/wrapping/center-exclusion results. Both
complete bounded helpers run unchanged, without callback stubs, PE startup,
Windows imports or full-game execution. This is numeric source validation,
not execution of an original production menu or a loaded original campaign.
The inherited predicate has separately documented original SMAC and bounded
SMAX-suffix reference evidence.

The normal installed AmigaOS SDK builds the probe, SHA256
`c25d68c82f38fb94b7cad5928fa60055c989793ef97b818606e11a3a80626cf1`.
Actual AmigaOS execution is UNRUN; independent final-head big-endian CI is
pending at this committed checkpoint. User-owned watch preview is untouched;
no native qualification, merge, visible UI change or restored-game claim follows
from this build. The first host pass predates extra empty-world/profile guards;
the final host-2 corpus is authoritative for this candidate.
