/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_ACTIONS_SNAPSHOT_H
#define ALPHA_LOCAL_ACTIONS_SNAPSHOT_H
#include "original_world.h"
namespace alpha {
enum class LocalSnapshotError {ok,bad_world,bad_buffer,unsupported_change,bad_reference,out_of_memory};
struct LocalSnapshotReceipt {size_t bytes;uint32_t runtime_seed;};
/* UNQUALIFIED buffer-only union of the movement and production record scopes.
 * Preserve complete supported original container/opaque bytes; explicitly LE
 * patch tiles/vehicles, base current minerals/queue0, faction units_queue.
 * Seal all other decoded fields and validate actor references/whole stacks
 * before writes. Serialization grants no order authority or legality.
 * Runtime RNG receipt is separate; footer tick remains original. No filesystem,
 * atomic creation, player save UI or original-executable compatibility claim.
 * Output/receipt unchanged on errors; all buffers disjoint from world storage.
 */
LocalSnapshotError encode_local_actions_snapshot(const OriginalWorld&,void*,size_t,LocalSnapshotReceipt&);
const char* local_snapshot_error_name(LocalSnapshotError);
}
#endif
