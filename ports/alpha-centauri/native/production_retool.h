/* SPDX-License-Identifier: GPL-3.0-or-later
 * Independently adapted from Brendan Casey's OpenSMACX base.cpp::base_making
 * and base_lose_minerals, C2013-2021, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 * Original instruction facts correct edition IDs, actual Skunkworks-only
 * override and signed32 intermediate domain; no FREEPROTO patch added.
 */
#ifndef ALPHA_PRODUCTION_RETOOL_H
#define ALPHA_PRODUCTION_RETOOL_H
#include "game_logic.h"
namespace alpha {
struct ProductionRetoolInput {
 Variant variant;
 int32_t strictness,penalty_percent,exempt_minerals;
 bool human;
 int32_t previous_item,queued_item,minerals_accumulated_2;
 uint8_t facilities[12];
};
struct ProductionRetoolResult {int32_t previous_class,queued_class,mineral_loss;};
/* Pure original retool classification and loss query. Not build eligibility,
 * production completion, a queue mutation or full base upkeep. Explicit original
 * facilities/pending queue/prior-production/mineral-baseline facts. Facility
 * bytes are indexed bits, not a native packed struct. Output transactional.
 * Rules0..3/0..100/0..1000; queued INT_MIN rejects unsafe original negation.
 * Checked original signed32 arithmetic rejects overflow instead of wrapping.
 */
LogicError production_retool(const ProductionRetoolInput&,ProductionRetoolResult&);
}
#endif
