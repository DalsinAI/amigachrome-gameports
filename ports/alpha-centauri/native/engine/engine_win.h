/* SPDX-License-Identifier: GPL-3.0-or-later
 * Minimal window-state records from Thinker Mod src/engine_win.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 * Only the fields that game logic and savegame.cpp touch are kept. Field
 * names are Thinker's so ported code compiles unchanged. These records hold
 * no renderer objects: the UI owns all drawing and reads/writes these fields.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

struct Sprite;      // opaque: owned by the UI layer
struct NetMessage { int unused; };

/* MapWindow view fields serialized by game_io() in savegame.cpp. */
struct MapWindow {
    int DrawToggleB;
    int DrawToggleA;
    int DrawFlags;
    int ZoomFactor;
    int TileX;
    int TileY;
};

/* Console (MapWin): the main map window. Only logic-visible fields. */
struct Console : MapWindow {
    int cOwner; // current player faction id, saved in game_data()
    int InVehMode;
    int VehID;
    int ReadyVehID;
    int field_23BE4;
    int field_23BE8;
    int CursorListIndex;
    int CursorListPrev;
    int CursorListX[32];
    int CursorListY[32];
    int field_23D20;
};

struct WorldWindow {
    MapWindow oMapWin;
};

struct StatusWindow {
    int pending_veh_id;
    int active_veh_id;
};

} // namespace engine
} // namespace alpha
