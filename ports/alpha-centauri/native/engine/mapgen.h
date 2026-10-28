/* SPDX-License-Identifier: GPL-3.0-or-later
 * World generation and new game setup (WP7). Declarations from Thinker Mod
 * src/mapgen.h, src/map.h and src/game.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE)
 * that are not already in engine_wp.h. Include engine.h first.
 */
#pragma once

namespace alpha {
namespace engine {

// Last three values on AltDefault are increased to enable higher altitude limit
const int AltDefault[] = {0, 15, 32, 45, 60, 75, 80, 100, 120, 140, 160};
static_assert(sizeof(AltDefault) == 44, "");

/* Runtime flags of the original executable used by world generation. They
 * are not saved and not part of EngineState (process-wide, like the
 * original globals). */
extern int* GameHalted;          // set while no game is running (no map redraws)
extern int* WorldAddTemperature; // world_alt_set adjusts temperature when set

int __cdecl near_landmark(int x, int y);
bool locate_landmark(int* x, int* y, bool ocean);
void __cdecl world_climate();
void __cdecl world_temperature();
void __cdecl world_rivers();
void __cdecl world_rainfall();
void __cdecl world_analysis();
void __cdecl world_rocky();
void __cdecl world_riverbeds();
void __cdecl world_shorelines();
void __cdecl world_polar_caps();
void __cdecl mod_world_crater(int x, int y);
void __cdecl mod_world_monsoon(int x, int y);
void __cdecl mod_world_sargasso(int x, int y);
void __cdecl mod_world_ruin(int x, int y);
void __cdecl mod_world_dune(int x, int y);
void __cdecl mod_world_diamond(int x, int y);
void __cdecl mod_world_fresh(int x, int y);
void __cdecl mod_world_volcano(int x, int y, int skip_label);
void __cdecl mod_world_borehole(int x, int y);
void __cdecl mod_world_temple(int x, int y);
void __cdecl mod_world_unity(int x, int y);
void __cdecl mod_world_fossil(int x, int y);
void __cdecl mod_world_canyon(int x, int y);
void __cdecl mod_world_mesa(int x, int y);
void __cdecl mod_world_ridge(int x, int y);
void __cdecl mod_world_geothermal(int x, int y);

/* Executable routines (new code, see mapgen_path.cpp). */
void __cdecl map_wipe();
void __cdecl Path_continents();
int __cdecl world_site(int x, int y, int toggle);

} // namespace engine
} // namespace alpha
