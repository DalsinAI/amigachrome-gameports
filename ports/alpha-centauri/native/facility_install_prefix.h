/* SPDX-License-Identifier: GPL-3.0-or-later
 * Ordinary-facility settlement/install prefix independently adapted from
 * Thinker base.cpp, MIT notice in THINKER-LICENSE.txt,
 * pin dabe62f2d1abba03de40f3dd3048494843f6a9af; original instructions checked.
 */
#ifndef ALPHA_FACILITY_INSTALL_PREFIX_H
#define ALPHA_FACILITY_INSTALL_PREFIX_H
#include "game_logic.h"
namespace alpha {
struct FacilityInstallPrefixInput {
 Variant variant;int32_t facility_id,minerals_accumulated,mineral_requirement;
 int32_t mineral_surplus,retool_exemption;bool human;uint32_t state_flags;
 uint8_t facilities[12];
};
struct FacilityInstallPrefixResult {
 uint32_t state_flags;int32_t minerals_accumulated,minerals_accumulated_2;
 uint8_t facilities[12];bool network_node_tail_required;
};
/* Pure PREFIX after availability/threshold checks, NOT a complete facility
 * completion, eligibility, live World mutation or production/turn authority.
 * Ordinary IDs2..33(SMAC)/2..64(SMAX); excludes HQ1, satellites and projects.
 * Requires physical bit absent and nonnegative requirement<=stock. Human excess
 * after payment is capped to max(0,min(retool_exemption,mineral_surplus)); AI
 * excess is retained. Snapshot remaining minerals into the retool baseline,
 * set production-done0x800000, clear bit31, and set the physical facility bit.
 * Retool exemption0..1000. Other flags/bitmap bytes remain exact; failure
 * preserves output. Network Node8 STILL NEEDS artifact state/actor effects.
 * Queue advance, choice/reset, announcements/audio and all other completion
 * effects remain required outside this prefix; no player control invokes it.
 */
LogicError facility_install_prefix(const FacilityInstallPrefixInput&,FacilityInstallPrefixResult&);
}
#endif
