/* SPDX-License-Identifier: GPL-3.0-or-later
 * Plain interface to the pure combat modules used as WP4 test oracles
 * (see wp4_oracles.cpp). All functions return 0 on success, non-zero when
 * the oracle rejects the input, -1 when the oracle is not built in.
 */
#pragma once
#include <stdint.h>

namespace wp4o {

bool have_prebattle();

struct Duel {
    int offense, defense, reactor_atk, reactor_def, damage_atk, damage_def;
};
struct DuelResult {
    int damage_atk, damage_def, rounds, attacker_hits, defender_hits, draws;
    bool attacker_won;
    uint32_t seed_after;
};
int rounds(const Duel& d, uint32_t seed, DuelResult* out);

int psi_factor(int strength, int rule_psi, bool attacking, bool tower, bool neural, bool dream, int* out);

struct Terrain {
    unsigned climate, val3, items, base_owner_nibble;
    int def_faction, def_unit, def_weapon_offense, def_triad;
    bool atk_present;
    int atk_unit, atk_weapon_offense, atk_triad, atk_faction;
    int pholus_owner;
};
int terrain(const Terrain& t, int* multiplier, int* label);

struct Basic {
    int unit_id, weapon_offense, armor_defense, plan, opposing_plan;
    int initial_morale, se_morale, se_morale_pending;
    bool at_base, opponent_native, dissociative, soporific, offense_mode;
    int factored_offense, factored_defense;
};
int basic(const Basic& b, int* offense, int* defense, int* morale_atk, int* morale_def);

int sensor(int defense, unsigned coverage, int percent, int* out);
int difficulty(int faction_atk, int faction_def, unsigned human_mask, int diff_atk, int diff_def,
    int offense, int* out);
int native_morale(int turn, int unit_id, int opponent, int atrocities, int tectonic,
    unsigned state, unsigned flags, unsigned fungus, int* out);

struct Artillery {
    int offense, defense, atk_triad, def_triad;
    unsigned atk_climate, def_climate, combat_type;
    int land_gun_percent, altitude_percent;
};
int artillery(const Artillery& a, int* offense, int* defense);

} // namespace wp4o
