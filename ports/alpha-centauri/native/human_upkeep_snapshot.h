/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_HUMAN_UPKEEP_SNAPSHOT_H
#define ALPHA_HUMAN_UPKEEP_SNAPSHOT_H
#include "local_actions_snapshot.h"
namespace alpha {
/* Internal base-game stage transport, NOT commit authority. Adds population,
 * stored food, worker mask, specialist count/types, economy/psych/labs totals
 * and faction credit/research ledgers to the existing qualified movement/order/
 * facility snapshot union. Seals all other decoded fields and opaque bytes.
 * Pending, incompletely resolved worker assignments may be transported inside
 * a transaction checkpoint; this grants no turn completion or public save UI.
 * Output and receipt remain unchanged on failure; all buffers disjoint.
 */
LocalSnapshotError encode_human_upkeep_snapshot(const OriginalWorld&,void*,size_t,LocalSnapshotReceipt&);
}
#endif
