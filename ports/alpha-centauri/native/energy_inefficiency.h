/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original black_market arithmetic: OpenSMACX base.cpp, Brendan Casey,
 * GPL-3.0-or-later, pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c.
 */
#ifndef ALPHA_ENERGY_INEFFICIENCY_H
#define ALPHA_ENERGY_INEFFICIENCY_H
#include <stdint.h>
namespace alpha {
enum class InefficiencyError {ok,bad_state,bad_buffer,overflow};
struct InefficiencyFacts {
 int32_t energy,pending_efficiency,hq_distance;
 uint32_t creche,upkeep_stage;
 int32_t counters[9];
};
struct InefficiencyResult {int32_t loss,counters[9];};
/* Explicit effective HQ distance after the original scan:0..998, no HQ16.
 * Return loss and nine proposed diagnostic counters. No world/cache/accounting
 * writes or upkeep/turn authority. Stages0/1 only; physical selected-base
 * Creche is explicit. Every executed signed32 intermediate must fit;
 * original wrapped arithmetic is outside this checked domain. Failure leaves
 * output unchanged. Input/output storage must be disjoint.
 */
InefficiencyError energy_inefficiency(const InefficiencyFacts&,InefficiencyResult&);
}
#endif
