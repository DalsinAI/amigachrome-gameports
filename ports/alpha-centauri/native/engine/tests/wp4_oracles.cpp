/* SPDX-License-Identifier: GPL-3.0-or-later
 * Differential oracles for the WP4 combat tests (built by test_engine_wp4.py).
 * Wraps the existing pure combat modules of the parent directory, which are
 * not part of the engine and are not changed: combat_rounds/combat_crt (round
 * loop and CRT stream) and the recovered original battle maths of the
 * prebattle prefix work (combat_prototype, terrain_defense, basic_combat,
 * sensor_combat, combat_difficulty, native_combat_morale, artillery_combat).
 * Kept in its own translation unit: those modules live in namespace alpha and
 * share names (Rules, Variant, ...) with alpha::engine.
 */
#include "wp4_oracles.h"

#include "combat_crt.h"
#ifdef WP4_PREBATTLE_ORACLES
#include "artillery_combat.h"
#include "basic_combat.h"
#include "combat_difficulty.h"
#include "combat_prototype.h"
#include "native_combat_morale.h"
#include "sensor_combat.h"
#include "terrain_defense.h"
#endif

namespace wp4o {

bool have_prebattle() {
#ifdef WP4_PREBATTLE_ORACLES
    return true;
#else
    return false;
#endif
}

int rounds(const Duel& d, uint32_t seed, DuelResult* out) {
    alpha::CombatDuel in = {d.offense, d.defense, d.reactor_atk, d.reactor_def, d.damage_atk, d.damage_def};
    alpha::CombatRandomState rng = {seed};
    alpha::CombatResult r = {};
    alpha::CombatError e = alpha::resolve_seeded_combat_rounds(in, rng, r);
    if (e != alpha::CombatError::ok) {
        return (int)e;
    }
    out->damage_atk = r.attacker_damage;
    out->damage_def = r.defender_damage;
    out->rounds = (int)r.rounds;
    out->attacker_hits = (int)r.attacker_hits;
    out->defender_hits = (int)r.defender_hits;
    out->draws = (int)r.random_draws;
    out->attacker_won = r.attacker_won;
    out->seed_after = rng.state;
    return 0;
}

#ifdef WP4_PREBATTLE_ORACLES
using alpha::LogicError;
using alpha::Variant;
static const Variant SMAX = Variant::alien_crossfire;

int psi_factor(int strength, int rule_psi, bool attacking, bool tower, bool neural, bool dream, int* out) {
    int32_t v = 0;
    LogicError e = alpha::combat_psi_factor(SMAX, strength, rule_psi, attacking, tower, neural, dream, v);
    if (e != LogicError::ok) {
        return 1;
    }
    *out = v;
    return 0;
}

int terrain(const Terrain& t, int* multiplier, int* label) {
    alpha::TerrainDefenseContext c = {};
    c.altitude_byte = t.climate;
    c.roughness_byte = t.val3;
    c.items = t.items;
    c.owner_nibble = t.base_owner_nibble;
    c.defender_faction = t.def_faction;
    c.defender_prototype = t.def_unit;
    c.defender_weapon = t.def_weapon_offense;
    c.defender_triad = t.def_triad;
    c.attacker_present = t.atk_present;
    c.attacker_prototype = t.atk_unit;
    c.attacker_weapon = t.atk_weapon_offense;
    c.attacker_triad = t.atk_triad;
    c.attacker_faction = t.atk_faction;
    c.pholus_owner = t.pholus_owner;
    alpha::TerrainDefense r = {};
    if (alpha::terrain_defense(c, r) != LogicError::ok) {
        return 1;
    }
    *multiplier = (int)r.multiplier;
    *label = (int)r.label;
    return 0;
}

int basic(const Basic& b, int* offense, int* defense, int* morale_atk, int* morale_def) {
    alpha::BasicCombatContext c = {};
    c.prototype = b.unit_id;
    c.weapon_offense = b.weapon_offense;
    c.armor_defense = b.armor_defense;
    c.plan = b.plan;
    c.opposing_plan = b.opposing_plan;
    c.initial_morale = b.initial_morale;
    c.active_se_morale = b.se_morale;
    c.pending_se_morale = b.se_morale_pending;
    c.at_base = b.at_base;
    c.creche = false;
    c.brood = false;
    c.opponent_native = b.opponent_native;
    c.dissociative = b.dissociative;
    c.soporific = b.soporific;
    c.offense_mode = b.offense_mode;
    c.factored_offense = b.factored_offense;
    c.factored_defense = b.factored_defense;
    alpha::BasicCombatStrengths r = {};
    if (alpha::basic_combat_strengths(SMAX, c, r) != LogicError::ok) {
        return 1;
    }
    *offense = r.offense;
    *defense = r.defense;
    *morale_atk = r.offense_morale;
    *morale_def = r.defense_morale;
    return 0;
}

int sensor(int defense, unsigned coverage, int percent, int* out) {
    alpha::SensorCombatAdjustment r = {};
    if (alpha::sensor_combat_adjustment(SMAX, defense, coverage, percent, r) != LogicError::ok) {
        return 1;
    }
    *out = r.defense;
    return 0;
}

int difficulty(int faction_atk, int faction_def, unsigned human_mask, int diff_atk, int diff_def,
int offense, int* out) {
    alpha::CombatDifficultyFacts f = {};
    f.observation = alpha::BattleEntryObservation::supplied;
    f.edition = SMAX;
    f.attacker_faction = (uint32_t)faction_atk;
    f.defender_faction = (uint32_t)faction_def;
    f.human_status = human_mask;
    f.attacker_difficulty = diff_atk;
    f.defender_difficulty = diff_def;
    f.offense = offense;
    alpha::CombatDifficultyResult r = {};
    if (alpha::inspect_combat_difficulty(f, r) != alpha::BattleEntryError::ok) {
        return 1;
    }
    *out = r.offense;
    return 0;
}

int native_morale(int turn, int unit_id, int opponent, int atrocities, int tectonic,
unsigned state, unsigned flags, unsigned fungus, int* out) {
    alpha::NativeCombatMoraleContext c = {turn, unit_id, opponent, atrocities, tectonic, state, flags, fungus};
    uint32_t v = 0;
    if (alpha::native_combat_morale(SMAX, c, v) != LogicError::ok) {
        return 1;
    }
    *out = (int)v;
    return 0;
}

int artillery(const Artillery& a, int* offense, int* defense) {
    alpha::ArtilleryContext c = {};
    c.offense = a.offense;
    c.defense = a.defense;
    c.attacker_triad = (uint32_t)a.atk_triad;
    c.defender_triad = (uint32_t)a.def_triad;
    c.attacker_altitude_byte = a.atk_climate;
    c.defender_altitude_byte = a.def_climate;
    c.battle_flags = a.combat_type;
    c.land_gun_percent = a.land_gun_percent;
    c.altitude_percent = a.altitude_percent;
    alpha::ArtilleryStrengths r = {};
    if (alpha::artillery_combat(c, r) != LogicError::ok) {
        return 1;
    }
    *offense = r.offense;
    *defense = r.defense;
    return 0;
}
#else
int psi_factor(int, int, bool, bool, bool, bool, int*) { return -1; }
int terrain(const Terrain&, int*, int*) { return -1; }
int basic(const Basic&, int*, int*, int*, int*) { return -1; }
int sensor(int, unsigned, int, int*) { return -1; }
int difficulty(int, int, unsigned, int, int, int, int*) { return -1; }
int native_morale(int, int, int, int, int, unsigned, unsigned, unsigned, int*) { return -1; }
int artillery(const Artillery&, int*, int*) { return -1; }
#endif

} // namespace wp4o
