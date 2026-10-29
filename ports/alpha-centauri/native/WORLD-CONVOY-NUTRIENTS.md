# Owned convoy-derived fresh nutrients

`world_convoy_flows` derives contributions from the owned decoded actor array
in original actor order. A supply unit is the original design plan 10 with
order 3. The actor faction must match the queried base's owner.

An actor homed to this base, on a physical base tile whose owner nibble is
below eight, adds one outgoing unit in its resource channel. The original
helper does not require that tile to belong to the querying faction. Outside
such a base tile, channels zero through two harvest the corresponding fresh
tile resource. Channel three harvests zero and resets the transient terraform
reduction counter. A supply actor with a different nonnegative home, at this
base's coordinates, adds one incoming unit. Its home reference is checked by
the owned importer and query; this operation does not dereference that base.

`world_convoy_nutrients` passes these flows and the resulting scratch into the
existing stable human intake and nutrient proposal. It uses actual rule
requirements, physical facilities, population, pending growth and stored food.
Saved intake is never fed back as fresh production. The returned proposals
leave actor state, base/faction accounting, original bytes and runtime RNG
unchanged. They do not advance session revision or invalidate a pending order.

This is a query backend, without player input, accounting commits, full mineral
support, pacifism, social projections, worker assignment, upkeep or turns.
The earlier explicit-convoy API remains available for callers that own those
transient facts; no existing API or frozen parent production file changes.

The stage supplies the edition/expansion profile, current base, normal worker
inspection, control-upkeep flag and scratch explicitly. It does not infer
original runtime authority from a save. Fungus harvesting rejects until fresh
technology-cache integration is available. Malformed resource channels reject
before indexing an array. Signed arithmetic rejects overflow. The shared tile
leaf computes all three resources, so overflow in an unused channel also
rejects conservatively. All inputs and output must be disjoint; failure leaves
the output untouched.

The synthetic controls compare every output word to an independent Python wire
model, exercise complete actor stack metadata and preserve every input/decoded
record. Private reference checks execute the owner's unchanged complete support,
base-yield and nutrient helpers in sequence, with actual crop/mine/energy code,
and compare 128 contexts to both host and independent Linux 68040 results.
Private session checks commit two real facility orders, query before/after,
export/reload the complete container, then continue with a no-op order. Exact
container bytes are compared to a separate Python patch oracle. Queued Creche
does not acquire a physical facility's growth bonus.

Local host and Linux 68040 evidence is additional to final-head CI and AmigaOS
execution. This local integration has no published CI or native qualification.
No proprietary files or original executable bytes are included in the source.

Fresh technology-effects integration supersedes the earlier selected/harvested-fungus refusal; see [WORLD-FRESH-TECHNOLOGY-YIELDS.md](WORLD-FRESH-TECHNOLOGY-YIELDS.md). All other stable-domain and mutation exclusions remain.
