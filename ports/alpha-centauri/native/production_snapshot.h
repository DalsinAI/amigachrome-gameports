/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_PRODUCTION_SNAPSHOT_H
#define ALPHA_PRODUCTION_SNAPSHOT_H
#include "original_world.h"
namespace alpha {
enum class ProductionSnapshotError {ok,bad_world,bad_buffer,unsupported_change};
struct ProductionSnapshotReceipt {size_t bytes;uint32_t runtime_seed;};
/* UNQUALIFIED buffer-only production snapshot boundary. Preserves the complete
 * original supported container and all opaque fields. Permits only current
 * base minerals/queue item0 and faction units_queue[512]; rejects every other
 * decoded change. Serialization does not establish action authority/legality.
 * Runtime RNG is returned separately; footer tick is unchanged. No filesystem,
 * atomic SDK file creation, player save UI or original-executable save claim.
 * Output/receipt unchanged on error; disjoint buffers required. */
ProductionSnapshotError encode_production_snapshot(const OriginalWorld&,void*,size_t,ProductionSnapshotReceipt&);
const char* production_snapshot_error_name(ProductionSnapshotError);
}
#endif
