/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP6 computer players: native life spawning.
 * Ported from Thinker Mod src/gameturn.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Deviations from Thinker are marked "Fix:" (behaviour) or "Original:"
 * (replaced executable or Windows code) and listed in ENGINE-WP6.md.
 */
#include "ai.h"

namespace alpha {
namespace engine {

void __cdecl alien_fauna() {
    using namespace ai;
    const int player_id = *CurrentPlayerFaction;
    int growth_limit = *MapAreaTiles / (*CurrentTurn / 4 + 32);
    for (int iter = 0; iter < growth_limit; iter++) {
        int tx = game_randv(*MapAreaX);
        int ty = game_randv(*MapAreaY);
        tx = tx + (ty & 1) - (tx & 1);
        if (!on_map(tx, ty)) {
            continue;
        }
        MAP* sq = mapsq(tx, ty);
        bool is_sea = is_ocean(sq);
        if (!is_sea) {
            if (!(sq->items & BIT_FOREST)) {
                continue;
            }
        } else if (!(sq->items & BIT_FARM)) {
            continue;
        }
        int best_score = 0;
        int best_x = -1;
        int best_y = -1;
        for (int i = 0; i < 8; i++) {
            int nx = wrap(tx + BaseOffsetX[i]);
            int ny = ty + BaseOffsetY[i];
            MAP* nsq = mapsq(nx, ny);
            if (nsq && is_ocean(nsq) == is_sea
            && nsq->alt_level() >= ALT_OCEAN_SHELF
            && (nsq->alt_level() <= ALT_ONE_ABOVE_SEA || nsq->climate & 0x18)
            && (nsq->landmarks & (LM_DISABLE|LM_VOLCANO)) != LM_VOLCANO
            && (nsq->landmarks & (LM_DISABLE|LM_DUNES)) != LM_DUNES) {
                int score = 4 * (3 - ((nsq->climate >> 3) & 3)) - (nsq->val3 >> 6);
                if ((nsq->val3 & 0xC0) <= 64
                && !(nsq->items & (BIT_FOREST|BIT_FARM|BIT_SOLAR|BIT_MINE|BIT_BASE_IN_TILE))
                && score > best_score) {
                    best_score = score;
                    best_x = nx;
                    best_y = ny;
                }
            }
        }
        if (best_x >= 0) {
            MAP* nsq = mapsq(best_x, best_y);
            uint32_t new_item = (is_sea ? BIT_FARM : BIT_FOREST);
            if (!(nsq->items & new_item)) {
                nsq->items = (new_item | nsq->items) & ~BIT_FUNGUS;
                for (int i = 1; i < MaxPlayerNum; i++) {
                    if (sq->is_known(i) // source tile visible or revealed
                    && sq->visible_items[i - 1] & new_item) {
                        nsq->visible_items[i - 1] = (new_item | nsq->visible_items[i - 1]) & ~BIT_FUNGUS;
                        if (i == player_id) {
                            int base_id = base_find_3(tx, ty, -1, -1, -1, player_id);
                            if (base_id < 0 || Bases[base_id].faction_id != player_id
                            || (*BaseFindDist > 3 && whose_territory(player_id, tx, ty, 0, 0) != player_id)) {
                                draw_tiles(tx, ty, 2);
                            } else {
                                if (!MapWin_focus(MapWinPtr ? MapWinPtr[0] : NULL, tx, ty)) {
                                    draw_tiles(tx, ty, 2);
                                }
                                parse_says(0, Bases[base_id].name, -1, -1);
                                if (conf.game_event_popup) {
                                    if (is_sea) {
                                        POP2("KELPGROWS", "kelp_sm.pcx", -1);
                                    } else {
                                        POP2("FORESTGROWS", "forgr_sm.pcx", -1);
                                    }
                                } else {
                                    if (is_sea) {
                                        NetMsg_pop(NetMsg, "KELPGROWS", 5000, 0, "kelp_sm.pcx");
                                    } else {
                                        NetMsg_pop(NetMsg, "FORESTGROWS", 5000, 0, "forgr_sm.pcx");
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    bool allow_locusts = (TechOwners[TECH_CentPsi] || TechOwners[TECH_Quantum]);
    bool early_limit = (*CurrentTurn < 2 * (10 * (3 - *MapNativeLifeForms) - *DiffLevel));
    if (*GameRules & RULES_SCN_NO_NATIVE_LIFE) {
        return;
    }
    int veh_limit = (conf.modify_unit_limit ? (MaxVehNum*3)/4 /* Fix: MaxVehModNum (8192) is beyond the engine capacity */ : 1792);
    if (project_base(FAC_VOICE_OF_PLANET) != SP_Unbuilt || *VehCount >= veh_limit) {
        return;
    }
    if (!(*CurrentTurn % (2 * (5 - *MapNativeLifeForms)))) {
        int iter = 0;
        int mx;
        int my;
        while (true) {
            mx = game_randv(*MapAreaX);
            my = game_randv(*MapAreaY);
            mx = mx + (my & 1) - (mx & 1);
            MAP* sq = mapsq(mx, my);
            assert(sq);
            if (++iter >= 1000) {
                break; // skip next block
            }
            if (sq->is_fungus()) {
                break;
            }
            if (is_ocean(sq) && sq->anything_at() < 0 && !bad_reg(sq->region)
            && Continents[sq->region].tile_count >= 50) {
                break;
            }
        }
        if (iter < 1000) {
            int spawn_val = *CurrentTurn * (((*GameState & STATE_PERIHELION_ACTIVE) != 0) + 1)
                / (4 * (4 * (8 - *MapNativeLifeForms) - *DiffLevel)) + *MapNativeLifeForms + 1;
            if (early_limit) {
                spawn_val = 1;
            }
            if (conf.spawn_sealurks && *ExpansionEnabled && game_rand() % 2) {
                // replace timeGetTime() random values
                int veh_id = veh_init(BSC_SEALURK, 0, mx, my);
                if (veh_id >= 0) {
                    veh_promote(veh_id);
                    if (allow_locusts) {
                        veh_init(BSC_LOCUSTS_OF_CHIRON, 0, mx, my);
                    }
                }
            } else {
                int veh_id = veh_init(BSC_ISLE_OF_THE_DEEP, 0, mx, my);
                if (veh_id >= 0) {
                    if (spawn_val >= veh_cargo(veh_id)) {
                        spawn_val = veh_cargo(veh_id);
                    }
                    while (spawn_val > 0) {
                        if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                            sleep(veh_init(BSC_MIND_WORMS, 0, mx, my));
                        } else {
                            sleep(veh_init(BSC_SPORE_LAUNCHER, 0, mx, my));
                        }
                        --spawn_val;
                    }
                    veh_promote(veh_id);
                    if (allow_locusts) {
                        veh_init(BSC_LOCUSTS_OF_CHIRON, 0, mx, my);
                    }
                }
            }
        }
    }
    auto is_visible_veh = [](int veh_id, int faction_id) {
        return (Vehs[veh_id].faction_id == faction_id
            && !Vehs[veh_id].is_invisible_lurker())
            || Vehs[veh_id].is_visible(faction_id);
    };
    int spawn_base_id = -1;
    int spawn_region = -1;
    int spawn_x = -1;
    int spawn_y = -1;
    int loop_limit = 2 * *MapNativeLifeForms + 2;

    for (int iter = 0; iter < loop_limit; iter++) {
        int rx = game_randv(*MapAreaX);
        int ry = game_randv(*MapAreaY);
        rx = rx + (ry & 1) - (rx & 1);
        MAP* sq = mapsq(rx, ry);
        assert(sq);
        if (is_ocean(sq) || !sq->is_fungus()) {
            continue;
        }
        if (bad_reg(sq->region) || sq->base_who() >= 0) {
            continue;
        }
        int base_id = base_find_3(rx, ry, -1, sq->region, -1, -1);
        int veh_owner_id = sq->veh_who();
        if (veh_owner_id > 0 && !(base_id >= 0 && *BaseFindDist <= 2)) {
            for (int i = 0; i < 16; i++) {
                int px = wrap(rx + TableOffsetX[i + 9]);
                int py = ry + TableOffsetY[i + 9];
                MAP* psq = mapsq(px, py);
                if (psq && !is_ocean(psq)
                && !psq->is_known(veh_owner_id)
                && psq->anything_at() < 0
                && !zoc_any(px, py, 0)) {
                    int veh_id;
                    if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                        veh_id = veh_init(BSC_MIND_WORMS, 0, px, py);
                    } else {
                        veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, px, py);
                    }
                    if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                        draw_tile(px, py, 2);
                    }
                    break;
                }
            }
        }
        if (veh_owner_id < 0
        && *BaseFindDist > 1 && *BaseFindDist < 6
        && sq->is_fungus() && !goody_at(rx, ry)) {
            spawn_base_id = base_id; // Fix: update only when spawning
            spawn_region = sq->region;
            spawn_x = rx;
            spawn_y = ry;
            int veh_id;
            if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                veh_id = veh_init(BSC_MIND_WORMS, 0, rx, ry);
            } else {
                veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, rx, ry);
            }
            if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                draw_tile(rx, ry, 2);
            }
        }
    }
    // Enable XMINDWORMS event spawning additional native life with minor rewrites
    // Previously this event was never triggered due to flawed original code
    if (spawn_base_id >= 0 && *DiffLevel >= DIFF_LIBRARIAN
    && !(*GameRules & RULES_BELL_CURVE)
    && !(conf.skip_random_events & (1 << 23))
    && *CurrentTurn >= 10 * (5 - *MapNativeLifeForms)) {
        BASE* base = &Bases[spawn_base_id];
        Faction* plr = &Factions[base->faction_id];
        int spawn_val = clamp(min(*MapNativeLifeForms + base->pop_size + base->eco_damage/8,
            ((*MapNativeLifeForms + 1)
            * (*GameState & STATE_PERIHELION_ACTIVE ? 2 : 1)
            * (base->pop_size + 3)) / 4), 1 + allow_locusts, 10);
        if (plr->base_count < clamp(*CurrentTurn / 8, 4, 20)) {
            spawn_val = (spawn_val + 1) / 2;
        }
        if (early_limit) {
            spawn_val = 1 + allow_locusts;
        }
        int show_popup = 0;
        int px;
        int py;
        int i = 0;
        MAP* sq;
        while (true) {
            px = wrap(spawn_x + TableOffsetX[i]);
            py = spawn_y + TableOffsetY[i];
            sq = mapsq(px, py);
            if (sq
            && sq->is_fungus()
            && sq->region == spawn_region
            && sq->anything_at() <= 0
            && !goody_at(px, py)
            && game_rand() % 2) {
                break;
            }
            if (++i >= 21) {
                return; // skip event, suitable tiles missing
            }
        }
        for (int iter = 0; iter < spawn_val; iter++) {
            int veh_id;
            if (!iter && allow_locusts) {
                veh_id = veh_init(BSC_LOCUSTS_OF_CHIRON, 0, px, py);
            } else if (game_rand() % 5 || !*ExpansionEnabled || !conf.spawn_spore_launchers) {
                veh_id = veh_init(BSC_MIND_WORMS, 0, px, py);
            } else {
                veh_id = veh_init(BSC_SPORE_LAUNCHER, 0, px, py);
            }
            if (veh_id >= 0 && is_visible_veh(veh_id, player_id)) {
                draw_tile(px, py, 2);
            }
            if (veh_id >= 0 && sq->is_known(player_id) && base->faction_id == player_id) {
                // skip is_visible_veh check since units are hidden on fungus by default
                show_popup = 1;
            }
        }
        if (show_popup) {
            if (!MapWin_focus(MapWinPtr ? MapWinPtr[0] : NULL, base->x, base->y)) {
                draw_tiles(base->x, base->y, 2);
            }
            parse_says(0, base->name, -1, -1);
            NetMsg_pop(NetMsg, "XMINDWORMS", -5000, 0, "mindworm_sm.pcx");
        }
    }
}

} // namespace engine
} // namespace alpha
