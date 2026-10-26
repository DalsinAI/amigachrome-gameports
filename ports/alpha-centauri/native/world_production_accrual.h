/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ALPHA_WORLD_PRODUCTION_ACCRUAL_H
#define ALPHA_WORLD_PRODUCTION_ACCRUAL_H
#include "original_world.h"
#include "production_accrual.h"
namespace alpha {
/* Read-only binding of the actual CURRENT base accounting fields and faction
 * historical mineral peak to the original arithmetic prefix. Saved accounting
 * is not certified fresh here and no fields are recomputed. This is NOT a
 * production phase/turn authority, accrual commit, or completion API. It may
 * inspect faction0..7 just as the original prefix does. Exact edition required;
 * errors/aliases preserve output and every wire/decoded world field and RNG.
 * A future commit must qualify current phase facts and single execution.
 */
LogicError world_production_accrual(const OriginalWorld&,Variant,size_t base,
 ProductionAccrualResult& output);
}
#endif
