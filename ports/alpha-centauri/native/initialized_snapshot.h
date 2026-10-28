/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_INITIALIZED_SNAPSHOT_H
#define ALPHA_INITIALIZED_SNAPSHOT_H
#include "phase_snapshot.h"
#include "rules_text.h"
namespace alpha {
/* Buffer extension of frozen phase codec for original load-record repairs.
 * Alive byte1 and revealed-map200 must exactly equal the repair recomputation
 * from this immutable baseline and qualified same-input89-row Rules. All other
 * preference/faction fields retain the inherited seals/phase allowlist. Full
 * inherited map/actor/order/accounting structural scope is unchanged.
 * Two simultaneous private World projections are bounded in total by limit;
 * output/caller storage and inherited validation scratch are excluded.
 * No turn authority, filesystem/player save protocol, actor/order action,
 * event acknowledgment or original-executable compatibility is established.
 * Failure preserves output/receipt/World/Rules. RNG separate/footer unchanged.
 */
LocalSnapshotError encode_initialized_snapshot(const OriginalWorld&,const Rules&,
 void*,size_t,LocalSnapshotReceipt&,size_t limit=64u*1024u*1024u);
}
#endif
