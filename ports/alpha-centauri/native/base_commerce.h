/* SPDX-License-Identifier: GPL-3.0-or-later
 * Trade formula: pinned Thinker default mod_base_energy policy, MIT,
 * dabe62f2d1abba03de40f3dd3048494843f6a9af. Complete original base_energy
 * is absent from OpenSMACX. See BASE-COMMERCE.md and THINKER-LICENSE.txt.
 */
#ifndef ALPHA_BASE_COMMERCE_H
#define ALPHA_BASE_COMMERCE_H
#include <stdint.h>
namespace alpha {
enum class CommerceError {ok,bad_state,bad_buffer,overflow,unsupported};
struct CommercePartnerFacts {
 int32_t fresh_our_intake,fresh_their_intake,technology_count,our_bonus,their_bonus;
 bool global_trade_pact,pact,our_governor,their_governor;
};
struct CommercePartnerResult {int32_t base_value,import_value,export_value;};
/* One already eligible treaty partner, no world mutation. Bonus -1 yields
 * zero pre-governor trade; lower custom bonuses are unsupported. Complete
 * original signed intermediate overflow rejects with unchanged output. */
CommerceError commerce_partner(const CommercePartnerFacts&,CommercePartnerResult&);
}
#endif
