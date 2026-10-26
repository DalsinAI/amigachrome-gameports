/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_LOCAL_ACCOUNTING_SNAPSHOT_H
#define ALPHA_LOCAL_ACCOUNTING_SNAPSHOT_H
#include "local_actions_snapshot.h"
namespace alpha {
/* Buffer-only union of bounded movement/orders and explicit upkeep0 base
 * accounting outputs. Full original bytes/opaque regions are preserved.
 * All other decoded fields, including faction totals, are sealed. No file
 * save protocol, original-executable compatibility or turn completion claim.
 * Failed validation leaves output/receipt unchanged; buffers must be disjoint. */
LocalSnapshotError encode_local_accounting_snapshot(const OriginalWorld&,void*,size_t,LocalSnapshotReceipt&);
const char* accounting_snapshot_error_name(LocalSnapshotError);
}
#endif
