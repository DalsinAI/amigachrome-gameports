/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP4 combat, part 3: engine_attack() for the UI, shoot() and capturing an
 * undefended base. New code (no Thinker counterpart: the executable did this
 * in order_veh, the map window and shoot()); it calls the Thinker-derived
 * battle functions in combat.cpp/combat_fight.cpp, which are MIT licensed
 * (Thinker Mod authors, see engine/THINKER-LICENSE).
 */
#include "combat.h"

namespace alpha {
namespace engine {

/*
Engine: shoot() is only in the executable. It is called by planet_busting()
once for every faction that would be hit. A human attacker giving orders
interactively is asked through break_treaty() (BREAKINGPACT/BREAKINGTREATY/
BREAKINGTRUCE/BEGINVENDETTA); a non-zero return calls the strike off. When the
strike goes ahead against a faction that is not at war with the attacker, it
is an act of aggression.
*/
int __cdecl shoot(int faction_id, int faction_id_tgt, int UNUSED(x), int UNUSED(y)) {
    if (faction_id_tgt <= 0 || faction_id_tgt >= MaxPlayerNum || faction_id_tgt == faction_id) {
        return 0;
    }
    if (is_human(faction_id) && (*VehAttackFlags & ATTACK_INTERACTIVE)
    && break_treaty(faction_id, faction_id_tgt, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
        return 1;
    }
    if (!at_war(faction_id, faction_id_tgt)) {
        act_of_aggression(faction_id, faction_id_tgt);
    }
    return 0;
}

const char* battle_outcome_name(int outcome) {
    switch (outcome) {
    case BATTLE_INVALID: return "invalid";
    case BATTLE_NO_RULES: return "no_rules";
    case BATTLE_NO_TARGET: return "no_target";
    case BATTLE_CANCELLED: return "cancelled";
    case BATTLE_ATTACKER_WON: return "attacker_won";
    case BATTLE_DEFENDER_WON: return "defender_won";
    case BATTLE_DEFENDER_RETREATED: return "defender_retreated";
    case BATTLE_BOTH_SURVIVED: return "both_survived";
    case BATTLE_ARTILLERY: return "artillery";
    case BATTLE_MARINE_DETACH: return "marine_detach";
    case BATTLE_PLANET_BUSTER: return "planet_buster";
    case BATTLE_BASE_CAPTURED: return "base_captured";
    default: return "unknown";
    }
}

/* Forwards everything to the installed sink and follows vehicle and base ids
 * while vehicles and bases are removed (ids above a removed one shift down). */
class IdTrackingSink : public EventSink {
    public:
    EventSink* next;
    const BattleResult* record;
    int veh_ids[2]; // attacker, chosen defender (-2 until it is chosen)
    int base_id;
    bool base_killed = false;
    IdTrackingSink(EventSink* n, const BattleResult* r, int atk, int base)
        : next(n), record(r), base_id(base) {
        veh_ids[0] = atk;
        veh_ids[1] = -2;
    }
    int popup(const PopupRequest& request) override {
        return next->popup(request);
    }
    void redraw(const RedrawRequest& request) override {
        next->redraw(request);
    }
    void notify(const EngineEvent& event) override {
        if (event.type == EV_VEH_KILLED) {
            if (veh_ids[1] == -2) {
                veh_ids[1] = record->veh_id_def;
            }
            for (int& id : veh_ids) {
                if (id == event.a) {
                    id = -1;
                } else if (id > event.a) {
                    id--;
                }
            }
        } else if (event.type == EV_BASE_KILLED) {
            if (base_id == event.a) {
                base_id = -1;
                base_killed = true;
            } else if (base_id > event.a) {
                base_id--;
            }
        }
        next->notify(event);
    }
};

static void clear_result(BattleResult* r, int veh_id_atk, int x, int y) {
    memset(r, 0, sizeof(*r));
    r->outcome = BATTLE_INVALID;
    r->veh_id_atk = veh_id_atk;
    r->veh_id_def = -1;
    r->unit_id_atk = -1;
    r->unit_id_def = -1;
    r->faction_id_atk = -1;
    r->faction_id_def = -1;
    r->x = -1;
    r->y = -1;
    r->tx = x;
    r->ty = y;
    r->base_id = -1;
    r->retreat_x = -1;
    r->retreat_y = -1;
}

/* Find the battle_fight offset for (x, y): BaseOffset index when adjacent,
 * else a TableOffset index within artillery range. */
static bool attack_offset(int veh_id, int x, int y, int* offset, int* table_offset) {
    VEH* veh = &Vehs[veh_id];
    int dist = map_range(veh->x, veh->y, x, y);
    if (dist == 1) {
        for (int i = 0; i < 8; i++) {
            if (wrap(veh->x + BaseOffsetX[i]) == x && veh->y + BaseOffsetY[i] == y) {
                *offset = i;
                *table_offset = 0;
                return true;
            }
        }
        return false;
    }
    if (dist < 2 || dist > MaxTableRange || !can_arty(veh->unit_id, true)
    || dist > arty_range(veh->unit_id)) {
        return false;
    }
    for (int i = 1; i < TableRange[dist]; i++) {
        if (wrap(veh->x + TableOffsetX[i]) == x && veh->y + TableOffsetY[i] == y) {
            *offset = i;
            *table_offset = 1;
            return true;
        }
    }
    return false;
}

static bool valid_attacker(int veh_id_atk, int x, int y) {
    return veh_id_atk >= 0 && veh_id_atk < *VehCount && Units && Vehs
        && on_map(x, y) && mapsq(x, y)
        && on_map(Vehs[veh_id_atk].x, Vehs[veh_id_atk].y)
        && (Vehs[veh_id_atk].x != x || Vehs[veh_id_atk].y != y);
}

int engine_attack_odds(int veh_id_atk, int x, int y, int* def_id) {
    if (def_id) {
        *def_id = -1;
    }
    if (!valid_attacker(veh_id_atk, x, y) || !Rules || Rules->move_rate_roads <= 0) {
        return 0;
    }
    int veh_id_def = stack_fix(veh_at(x, y));
    int offset, table_offset;
    if (veh_id_def < 0 || Vehs[veh_id_def].faction_id == Vehs[veh_id_atk].faction_id
    || (!Vehs[veh_id_atk].is_combat_unit() && !Vehs[veh_id_atk].is_planet_buster())
    || !attack_offset(veh_id_atk, x, y, &offset, &table_offset)) {
        return 0;
    }
    return mod_battle_fight_2(veh_id_atk, offset, x, y, table_offset, 0, def_id);
}

/*
Engine: capturing a base without defenders. In the executable order_veh moved
the unit into the base and called capture_base; here the attack command does
the same for a land combat unit next to the base.
*/
static int capture_empty_base(int veh_id_atk, int base_id, BattleResult* r, int attack_flags) {
    VEH* veh = &Vehs[veh_id_atk];
    int faction_id = veh->faction_id;
    int faction_id_def = Bases[base_id].faction_id;
    int x = Bases[base_id].x;
    int y = Bases[base_id].y;
    if (map_range(veh->x, veh->y, x, y) != 1 || veh->triad() != TRIAD_LAND
    || !veh->is_combat_unit() || veh->is_probe() || is_ocean(mapsq(x, y))) {
        r->outcome = BATTLE_NO_TARGET;
        return r->outcome;
    }
    r->faction_id_def = faction_id_def;
    r->base_id = base_id;
    if (!(attack_flags & ATTACK_EXECUTE)) {
        r->outcome = BATTLE_NO_TARGET;
        return r->outcome;
    }
    if (!at_war(faction_id, faction_id_def)) {
        if (is_human(faction_id) && (attack_flags & ATTACK_INTERACTIVE)
        && break_treaty(faction_id, faction_id_def, DIPLO_TRUCE|DIPLO_TREATY|DIPLO_PACT)) {
            r->outcome = BATTLE_CANCELLED;
            return r->outcome;
        }
        if (faction_id_def) {
            act_of_aggression(faction_id, faction_id_def);
        }
    }
    int cost = mod_hex_cost(veh->unit_id, faction_id, veh->x, veh->y, x, y, 0);
    if (cost <= 0) {
        cost = Rules->move_rate_roads;
    }
    veh->moves_spent += cost;
    veh->state |= VSTATE_HAS_MOVED;
    veh_put(veh_id_atk, x, y);
    BattleEvent ev = {BE_BASE_CAPTURED, base_id, faction_id_def, faction_id, 0, 0, x, y};
    r->events[r->event_count++] = ev;
    mod_capture_base(base_id, faction_id, 0);
    r->base_captured = true;
    r->outcome = BATTLE_BASE_CAPTURED;
    return r->outcome;
}

void battle_record_set(BattleResult* record); // combat_fight.cpp

int engine_attack(int veh_id_atk, int x, int y, BattleResult* r, int attack_flags) {
    if (!r) {
        return BATTLE_INVALID;
    }
    clear_result(r, veh_id_atk, x, y);
    if (!valid_attacker(veh_id_atk, x, y)) {
        return r->outcome;
    }
    if (!Rules || Rules->move_rate_roads <= 0) {
        r->outcome = BATTLE_NO_RULES;
        return r->outcome;
    }
    VEH* veh = &Vehs[veh_id_atk];
    r->unit_id_atk = veh->unit_id;
    r->faction_id_atk = veh->faction_id;
    r->x = veh->x;
    r->y = veh->y;
    r->hp_atk_before = veh->cur_hitpoints();
    int base_id = base_at(x, y);
    int veh_id_def = stack_fix(veh_at(x, y));
    if (veh_id_def < 0) {
        if (base_id >= 0 && Bases[base_id].faction_id != veh->faction_id) {
            return capture_empty_base(veh_id_atk, base_id, r, attack_flags);
        }
        r->outcome = BATTLE_NO_TARGET;
        return r->outcome;
    }
    int offset, table_offset;
    if (Vehs[veh_id_def].faction_id == veh->faction_id || (!veh->is_combat_unit() && !veh->is_planet_buster())
    || !attack_offset(veh_id_atk, x, y, &offset, &table_offset)) {
        r->outcome = BATTLE_NO_TARGET;
        return r->outcome;
    }
    r->base_id = base_id;
    const bool planet_buster = veh->is_planet_buster() != 0;
    const bool missile = veh->is_missile() != 0;
    const int saved_flags = *VehAttackFlags;
    *VehAttackFlags = attack_flags;
    *CurrentVehID = veh_id_atk;
    IdTrackingSink tracker(event_sink(), r, veh_id_atk, base_id);
    EventSink* previous = set_event_sink(&tracker);
    battle_record_set(r);
    int def_id = -1;
    r->return_value = mod_battle_fight_2(veh_id_atk, offset, x, y, table_offset, 1, &def_id);
    battle_record_set(nullptr);
    set_event_sink(previous);
    *VehAttackFlags = saved_flags;

    int atk_after = tracker.veh_ids[0];
    int def_after = (tracker.veh_ids[1] == -2) ? r->veh_id_def : tracker.veh_ids[1];
    r->veh_id_atk = atk_after;
    r->base_id = tracker.base_id;
    r->base_destroyed = tracker.base_killed;
    bool started = false;
    bool detached = false;
    for (int i = 0; i < r->event_count; i++) {
        const BattleEvent& ev = r->events[i];
        if (ev.type == BE_START) {
            started = true;
        } else if (ev.type == BE_ROUND) {
            r->rounds++;
            if (ev.a == 1) {
                r->attacker_hits++;
            } else {
                r->defender_hits++;
            }
        } else if (ev.type == BE_PROMOTED) {
            if (ev.a == veh_id_atk) {
                r->promoted_atk = true;
            } else {
                r->promoted_def = true;
            }
        } else if (ev.type == BE_MARINE_DETACH) {
            detached = true;
        }
    }
    r->veh_id_def = def_after;
    r->hp_atk_after = atk_after >= 0 ? Vehs[atk_after].cur_hitpoints() : 0;
    r->hp_def_after = def_after >= 0 ? Vehs[def_after].cur_hitpoints() : 0;
    if (r->outcome == BATTLE_CANCELLED) {
        return r->outcome;
    }
    if (!started) {
        r->outcome = (r->return_value == 2 ? BATTLE_NO_TARGET : BATTLE_INVALID);
    } else if (planet_buster) {
        r->outcome = BATTLE_PLANET_BUSTER;
    } else if (r->combat_type == CT_CAN_ARTY) {
        r->outcome = BATTLE_ARTILLERY;
    } else if (r->retreat_x >= 0) {
        r->outcome = BATTLE_DEFENDER_RETREATED;
    } else if (detached) {
        r->outcome = BATTLE_MARINE_DETACH;
    } else if (r->num_relics > 0 && atk_after >= 0) {
        r->outcome = BATTLE_ATTACKER_WON;
    } else if (r->hp_def_before > 0 && r->rounds > 0) {
        // Who lost the last round decides the battle (rounds stop at 0 hp).
        bool atk_alive = false;
        for (int i = r->event_count - 1; i >= 0; i--) {
            if (r->events[i].type == BE_ROUND) {
                atk_alive = r->events[i].b > 0;
                bool def_alive = r->events[i].c > 0;
                if (atk_alive && def_alive) {
                    r->outcome = BATTLE_BOTH_SURVIVED;
                } else {
                    r->outcome = atk_alive ? BATTLE_ATTACKER_WON : BATTLE_DEFENDER_WON;
                }
                break;
            }
        }
        if (missile && r->outcome == BATTLE_ATTACKER_WON) {
            r->veh_id_atk = -1; // the missile is spent
        }
    } else {
        r->outcome = (atk_after >= 0) ? BATTLE_ATTACKER_WON : BATTLE_DEFENDER_WON;
    }
    return r->outcome;
}

} // namespace engine
} // namespace alpha
