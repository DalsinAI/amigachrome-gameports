/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_FACILITY_PRODUCTION_SNAPSHOT_H
#define ALPHA_FACILITY_PRODUCTION_SNAPSHOT_H
#include "local_actions_snapshot.h"
namespace alpha {
/* Restoration-stage persistence union: retains existing live movement/orders,
 * adds Creche2/Recycling3 built bits, production flags/mineral baseline and
 * nondecreasing best-mineral-output. All other decoded fields remain sealed.
 * Complete opaque original container retained. Serialization grants no stage
 * authority and is not an original Windows save-compatibility certification.
 */
LocalSnapshotError encode_facility_production_snapshot(const OriginalWorld&,void*,size_t,LocalSnapshotReceipt&);
}
#endif
