/* SPDX-License-Identifier: GPL-3.0-or-later
 * OpenSMACX (C) 2013-2021 Brendan Casey, base.cpp::black_market004EA1F0,
 * pin2f7f4e24faddbaa5b97ee76d5e3dccfd855e4a8c. Gross/surplus composition
 * also follows Thinker mod_base_energy pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 */
#ifndef ALPHA_BASE_ENERGY_H
#define ALPHA_BASE_ENERGY_H
#include <stdint.h>
namespace alpha {
enum class EnergyError {ok,bad_state,bad_buffer,overflow};
struct EnergyFacts {
 int32_t fresh_intake,convoy_to,commerce,energy_grid,convoy_from;
 int32_t headquarters_distance,pending_efficiency,upkeep_stage;
 bool physical_creche;
 int32_t social_efficiency_before[9];
};
struct EnergyResult {
 int32_t intake,consumption,inefficiency,surplus;
 int32_t social_efficiency_after[9];
};
/* Pre-allocation energy accounting only. Fresh values must come from current
 * engines, never saved intake/surplus. No-HQ is distance-1 -> original16;
 * original physical-HQ sentinel999 is retained (distances>=999 mean missing).
 * Physical HQ at distance0 returns before updating efficiency statistics.
 * Only upkeep_stage1 updates the9 original hypothetical-efficiency totals;
 * those statistics intentionally are not clamped to energy. All signed original
 * intermediates must fit32bits. Normal nonnegative income/export domain.
 * Invalid/overflow/aliased inputs preserve output. No world mutation, economy/
 * labs/psych allocation, commerce generation, upkeep or turn advance implied.
 */
EnergyError base_energy_accounting(const EnergyFacts&,EnergyResult&);
}
#endif
