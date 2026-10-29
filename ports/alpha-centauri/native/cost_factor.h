/* SPDX-License-Identifier: GPL-3.0-or-later
 * Owned-context port of OpenSMACX base.cpp::cost_factor, original offset
 * 004E4430, Brendan Casey (C)2013-2021. Pin
 * 2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_COST_FACTOR_H
#define ALPHA_COST_FACTOR_H
#include <stdint.h>
namespace alpha {
enum class CostFactorError {ok,bad_state,bad_buffer,unsupported,overflow};
struct CostFactorFacts {
 uint32_t resource,multiplier,map_size,difficulty;
 int32_t industry_pending,growth_pending;
 bool human,leader_threat,leader_human,multiplayer_net;
 bool base_present,children_creche,golden_age;
};
/* Resource0 nutrient,1 mineral; other nonzero values preserve the original
 * legacy mineral-multiplier/unadjusted-industry branch. Map sizes0/1 apply
 * successive integer reductions after the rule multiplier. Industry uses the
 * original switch DEFAULT, including ratings below -7, without clamping.
 * Growth bonuses apply only with a base, then clamp -2..5. Rule multiplier
 * range1..100 follows read_basic_rules. AI dependency booleans are explicit
 * facts here; the connected adapter does not pretend to derive great_satan.
 * Invalid inputs/overflow/alias leave output unchanged. A valid result may0.
 */
CostFactorError cost_factor(const CostFactorFacts&,int32_t&output);
}
#endif
