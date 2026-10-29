/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_WORKERS_SNAPSHOT_H
#define ALPHA_LOCAL_WORKERS_SNAPSHOT_H
#include "local_accounting_snapshot.h"
namespace alpha {
/* Buffer-only union of movement, facility orders, stage0 accounting and explicit
 * worker/first-intake fields. Every other decoded field is sealed. Full original
 * bytes/opaque regions/footer retained, runtime RNG returned separately.
 * No filesystem atomicity/player save/original-executable compatibility claim.
 * Failed validation preserves output/receipt; input/output storage disjoint. */
LocalSnapshotError encode_local_workers_snapshot(const OriginalWorld&,void*,size_t,LocalSnapshotReceipt&);
const char* workers_snapshot_error_name(LocalSnapshotError);
}
#endif
