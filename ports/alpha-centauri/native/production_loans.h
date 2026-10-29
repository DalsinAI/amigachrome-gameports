/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original production_phase debt arithmetic; source differences retained from
 * pinned OpenSMACX/Thinker references, attribution in original_faction.h.
 */
#ifndef ALPHA_PRODUCTION_LOANS_H
#define ALPHA_PRODUCTION_LOANS_H
#include "game_logic.h"
namespace alpha {
struct ProductionLoanFacts {
 int32_t faction;uint32_t alive_mask;
 int32_t credits[8],balance[8],payment[8];uint32_t diplomacy[8];int32_t sanctions[8];
};
struct ProductionLoanResult {int32_t credits[8],balance[8],energy_before_bases;};
/* Pure original ascending-creditor1..7 settlement. Exact signed minimum, no
 * modern zero clamp or cap to remaining principal. Nonzero negative balances
 * and signed payment/reserve values retain original branches. War bit0x10
 * increases balance without payment; recipient sanctions must be exactly zero.
 * Every original signed intermediate is checked. Error/alias leaves output
 * unchanged. No diplomacy action, World mutation, phase authority or upkeep.
 */
LogicError production_loans(const ProductionLoanFacts&,ProductionLoanResult&);
}
#endif
