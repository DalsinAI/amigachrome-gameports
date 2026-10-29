/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original SAV/SC game container read/write, ported from Thinker Mod
 * src/savegame.cpp (game_data, game_io, map_read/map_write, load/save daemon)
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 *
 * Wire format: "TERRAN\0" 0x1A, int32 outer version 0x56, game_data(), then a
 * uint32 tick seed. Every record is encoded field by field in little-endian
 * order (save_fields.inc), so big-endian hosts (68k) read and write files that
 * are byte-identical to the original's. Supported data versions are 7..12, the
 * same range as original_game.cpp. Encrypted (Ironman/network) and PBEM saves
 * are rejected with an explicit error instead of being misread.
 *
 * Versions below 11 are converted on load exactly like Thinker (zero-extended
 * facility/intel arrays, weapon ids >= 12 shifted by +3, project queue ids
 * <= -34 shifted by -31, v7 icon fixups) and written back in the same version
 * with the inverse conversion. upgrade_save_version() switches to v12.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

enum SaveError {
    SAVE_OK = 0,
    SAVE_ERR_HEADER = 1,        // not "TERRAN\0\x1a" or outer version < 0x56
    SAVE_ERR_VERSION = 2,       // data version outside 7..12
    SAVE_ERR_ENCRYPTED = 3,     // STATE_UNK_10000000 (Ironman/network) not supported
    SAVE_ERR_PBEM = 4,          // MRULES_UNK_20 (PBEM diplomacy tables) not supported
    SAVE_ERR_TRUNCATED = 5,
    SAVE_ERR_PLAYER = 6,        // current player outside 0..7
    SAVE_ERR_PROJECT = 7,       // secret project base id >= 512
    SAVE_ERR_COUNTS = 8,        // replay/vehicle/base counts out of range
    SAVE_ERR_MAP = 9,           // map dimensions invalid
    SAVE_ERR_SECURITY = 10,     // Thinker tamper check (SaveFileStatus = 1)
    SAVE_ERR_IO = 11,           // file open/write failure
};
const char* save_error_name(int error);

/* Thinker load_daemon return values (savegame.h). */
enum SaveLoadStatus {
    SAVE_LOAD_VALID = 0,
    SAVE_LOAD_NONE = 1,
    SAVE_LOAD_NOT = 2,
    SAVE_LOAD_OLD = 3,
    SAVE_LOAD_ERROR = 4,
    SAVE_LOAD_SECURITY = 5,
};

struct SaveInfo {
    int error;            // SaveError
    size_t consumed;      // bytes used by header + game_data (+ seed when present)
    bool has_tick_seed;   // the 4-byte seed footer was present
    int legacy_marker;    // version field of the first 0x31C header block
};

/* Pure container import into the bound EngineState: header, game_data() and
 * the optional seed footer. No load-time game fixups are applied (see
 * load_daemon_bytes). On failure the bound state is left unchanged and the
 * SaveError is returned. Trailing bytes after the seed are an error. */
int game_read_bytes(const uint8_t* data, size_t length, SaveInfo* info = nullptr);
/* Pure container export of the bound EngineState in its current
 * SaveFileVersion: header, game_data() and the tick seed (save_tick_seed).
 * Like Thinker, MapWin->field_23D20 is written as 0. */
int game_write_bytes(std::vector<uint8_t>& out);

/* Thinker mod_load_daemon equivalent on bytes: game_read_bytes followed by the
 * original load-time fixups (fixup_landmarks, AltNatural, PFLAG_MAP_REVEALED,
 * set_alive, cursor list, rebuild_vehicle_bits/rebuild_base_bits, ...).
 * Returns a SaveLoadStatus. Also repairs prototype component ids past their
 * rules tables (fix_prototype_ids). */
int load_daemon_bytes(const uint8_t* data, size_t length);
/* Set prototype chassis/weapon/armor ids past their tables to 0 and reactor
 * ids above MaxReactorNum to REC_FISSION in Units. Returns the number of
 * prototypes changed. load_daemon_bytes calls it. */
int fix_prototype_ids();
/* Thinker mod_save_daemon equivalent on bytes (seed = save_tick_seed). */
int save_daemon_bytes(std::vector<uint8_t>& out);
/* File versions (stdio). */
int load_daemon(const char* filename);
int save_daemon(const char* filename);

/* Switch the bound state to data version 12 (the version Thinker writes). */
void upgrade_save_version();

/* Wire-size helpers for tests and tools. */
size_t wire_size_record(const char* record_name, bool legacy);

} // namespace engine
} // namespace alpha
