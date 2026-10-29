/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef AC_ALPHA_MOVEMENT_SNAPSHOT_H
#define AC_ALPHA_MOVEMENT_SNAPSHOT_H
#include "original_world.h"
namespace alpha {
enum class SnapshotError {ok,bad_world,bad_buffer,unsupported_change,bad_reference,out_of_memory};
struct MovementSnapshotReceipt {size_t bytes;uint32_t runtime_seed;};
/* UNQUALIFIED buffer encoder, not a filesystem save operation or original
 * executable save-compatibility claim. Preserve the original supported plain
 * SC/SAV container and all opaque bytes, patching only owned tiles/vehicles
 * with explicit little-endian encoding. Reject changes to other decoded
 * records instead of silently losing them. Runtime RNG is returned separately:
 * the original footer tick is not the runtime RNG and is never repurposed.
 * No file creation/overwrite, SDK O_EXCL assumption or native struct dump.
 * Output/receipt stay unchanged on every error; buffers must be disjoint from
 * the world, its owned storage and each other. Not exposed to player input.
 */
SnapshotError encode_movement_snapshot(const OriginalWorld& world,void* output,
 size_t capacity,MovementSnapshotReceipt& receipt);
const char* snapshot_error_name(SnapshotError error);
}
#endif
