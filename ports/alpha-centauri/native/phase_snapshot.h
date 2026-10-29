/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_PHASE_SNAPSHOT_H
#define ALPHA_PHASE_SNAPSHOT_H
#include "local_workers_snapshot.h"
namespace alpha {
/* Buffer-only extension of frozen workers snapshot. Explicit social-upkeep,
 * ODP and pre-base production faction fields in phase_faction_fields.inc plus
 * player flag0x20 CLEAR ONLY; all other faction fields remain sealed. The
 * previous full map/actor, order and worker/accounting structural scope stays.
 * Uses a private owned World projection to retain every inherited validator;
 * its additional allocation is bounded by max_clone_bytes (caller World and
 * output storage excluded). Current monuments and all metadata stay sealed.
 * No phase admission, actor refill command, gameplay mutation, engine state,
 * filesystem/atomic commit/player save or original executable compatibility.
 * Runtime RNG is returned separately; footer bytes remain original. All
 * failures preserve caller buffers/receipt and caller World.
 */
LocalSnapshotError encode_phase_snapshot(const OriginalWorld&,void*,size_t,
 LocalSnapshotReceipt&,size_t max_clone_bytes=64u*1024u*1024u);
}
#endif
