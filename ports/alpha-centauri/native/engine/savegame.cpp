/* SPDX-License-Identifier: GPL-3.0-or-later
 * Original SAV container I/O. game_data(), game_io(), map_read()/map_write(),
 * mod_load_daemon() and mod_save_daemon() follow Thinker Mod src/savegame.cpp
 * at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 *
 * Differences from Thinker:
 * - file_feed() copies of packed memory are replaced by explicit little-endian
 *   field encoding (save_fields.inc), so the result is host-endian independent;
 * - reading goes into a scratch copy of the state, committed only on success;
 * - data versions 7..12 only; encryption/PBEM sections are rejected;
 * - legacy (< v11) files are written back in their own layout (Thinker always
 *   writes v12) and the first 0x31C header block of v11+ files is preserved
 *   (Thinker writes zeroes there, the original wrote stack garbage);
 * - MapWin/MapWinPtr/CanvasWidth values are stored in the state rather than in
 *   live windows, so loading keeps them for the UI and saving writes them back.
 */
#include "engine.h"

#include <memory>

namespace alpha {
namespace engine {

namespace {

struct Encoder {
    std::vector<uint8_t>* out;
    template <class T> void scalar(T& v) {
        typedef typename std::make_unsigned<T>::type U;
        U u = (U)v;
        for (size_t i = 0; i < sizeof(T); i++) {
            out->push_back((uint8_t)((uint64_t)u >> (8 * i)));
        }
    }
};

struct Decoder {
    const uint8_t* p;
    size_t n;
    size_t pos;
    bool ok;
    template <class T> void scalar(T& v) {
        typedef typename std::make_unsigned<T>::type U;
        if (!ok || n - pos < sizeof(T)) {
            ok = false;
            return;
        }
        uint64_t u = 0;
        for (size_t i = 0; i < sizeof(T); i++) {
            u |= (uint64_t)p[pos + i] << (8 * i);
        }
        pos += sizeof(T);
        v = (T)(U)u;
    }
};

struct Counter {
    size_t n;
    template <class T> void scalar(T&) {
        n += sizeof(T);
    }
};

template <class F, class T, size_t N> void io(F& f, T (&a)[N]);
template <class F, class T> void io(F& f, T& v);

template <class F, class T, size_t N> void io_prefix(F& f, T (&a)[N], size_t count) {
    for (size_t i = 0; i < count && i < N; i++) {
        io(f, a[i]);
    }
}

template <class F, class T, size_t N> void io(F& f, T (&a)[N]) {
    for (size_t i = 0; i < N; i++) {
        io(f, a[i]);
    }
}

} // namespace

#include "save_fields.inc"

namespace {

template <class F, class T> void io(F& f, T& v) {
    if constexpr (std::is_integral<T>::value) {
        f.scalar(v);
    } else {
        io_fields(f, v, false);
    }
}

template <class F, class T> void io_record(F& f, T& v, bool legacy) {
    if constexpr (std::is_class<T>::value) {
        io_fields(f, v, legacy);
    } else {
        io(f, v);
    }
}

/* file_feed() analog over memory. */
struct Stream {
    bool write;
    std::vector<uint8_t>* out;
    const uint8_t* in;
    size_t in_len;
    size_t pos;
    bool ok;
};

template <class T>
bool feed(Stream& s, T& value, bool legacy = false) {
    if (s.write) {
        Encoder e{s.out};
        io_record(e, value, legacy);
        return true;
    }
    Decoder d{s.in, s.in_len, s.pos, true};
    io_record(d, value, legacy);
    if (!d.ok) {
        s.ok = false;
        return false;
    }
    s.pos = d.pos;
    return true;
}

template <class T>
bool feed_n(Stream& s, T* values, size_t count, bool legacy = false) {
    for (size_t i = 0; i < count; i++) {
        if (!feed(s, values[i], legacy)) {
            return false;
        }
    }
    return true;
}

bool feed_bytes(Stream& s, uint8_t* bytes, size_t count) {
    if (s.write) {
        s.out->insert(s.out->end(), bytes, bytes + count);
        return true;
    }
    if (s.in_len - s.pos < count) {
        s.ok = false;
        return false;
    }
    memcpy(bytes, s.in + s.pos, count);
    s.pos += count;
    return true;
}

uint32_t le32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

void put_le32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

const int LegacyMarkerOffset = 0x304; // LegacyGamePrefs::SaveFileVersion

int game_io(Stream& fs, MapWindow* state) {
    if (!feed(fs, state->DrawToggleB)) return 1;
    if (!feed(fs, state->DrawToggleA)) return 1;
    if (!feed(fs, state->DrawFlags)) return 1;
    if (!feed(fs, state->ZoomFactor)) return 1;
    if (!feed(fs, state->TileX)) return 1;
    return !feed(fs, state->TileY);
}

int map_write(Stream& fs) {
    EngineState* st = engine_state();
    if (!feed(fs, st->map)) {
        return 1;
    }
    if (!feed_n(fs, *MapTiles, (size_t)*MapAreaTiles)) {
        return 1;
    }
    return !feed_bytes(fs, *MapAbstract, (size_t)*MapAbstractCount);
}

int map_read(Stream& fs, int* error) {
    EngineState* st = engine_state();
    map_shutdown();
    if (!feed(fs, st->map)) {
        *error = SAVE_ERR_TRUNCATED;
        return 1;
    }
    debug("map_read x=%d y=%d\n", *MapAreaX, *MapAreaY);
    // Thinker map_read: bounds, then map_init() recomputes the derived counts.
    // Original also requires an even width (tiles are stored as (x + X*y)/2).
    if (*MapAreaX <= 0 || *MapAreaX > 0x1000 || (*MapAreaX & 1)
    || *MapAreaY <= 0 || *MapAreaY > 0x1000 || *MapLandmarkCount < 0
    || *MapLandmarkCount > MaxLandmarkNum) {
        *error = SAVE_ERR_MAP;
        return 1;
    }
    if (map_init()) {
        *error = SAVE_ERR_MAP;
        return 1;
    }
    if (!feed_n(fs, *MapTiles, (size_t)*MapAreaTiles)) {
        *error = SAVE_ERR_TRUNCATED;
        return 1;
    }
    if (!feed_bytes(fs, *MapAbstract, (size_t)*MapAbstractCount)) {
        *error = SAVE_ERR_TRUNCATED;
        return 1;
    }
    // Original: fixup_landmarks() runs here; it is a load-time fixup applied
    // by load_daemon_bytes() so pure container round trips stay exact.
    return 0;
}

/*
Thinker game_data(), on the bound state. Returns 0 or a SaveError.
*/
int game_data(Stream& fs, int write_file) {
    EngineState* st = engine_state();
    const bool w = write_file != 0;
    *SaveFileWrite = write_file;
    *SaveFileEncrypt = 0;
    *SaveFileStatus = 0;

    // First header block: LegacyGamePrefs (0x31C). Its SaveFileVersion field
    // (the "marker") selects the expanded 0x398 GamePrefs block that follows.
    uint8_t legacy[LegacyPrefsSize];
    int marker;
    if (w) {
        marker = (int)le32(st->legacy_prefs + LegacyMarkerOffset);
        if (marker >= 11) {
            memcpy(legacy, st->legacy_prefs, sizeof(legacy));
        } else {
            std::vector<uint8_t> enc;
            Encoder e{&enc};
            io_fields(e, *cs, true);
            assert(enc.size() == sizeof(legacy));
            memcpy(legacy, enc.data(), sizeof(legacy));
        }
        if (!feed_bytes(fs, legacy, sizeof(legacy))) {
            return SAVE_ERR_TRUNCATED;
        }
        if (marker >= 11 && !feed(fs, *cs)) {
            return SAVE_ERR_TRUNCATED;
        }
    } else {
        if (!feed_bytes(fs, legacy, sizeof(legacy))) {
            return SAVE_ERR_TRUNCATED;
        }
        marker = (int)le32(legacy + LegacyMarkerOffset);
        memcpy(st->legacy_prefs, legacy, sizeof(legacy));
        if (marker >= 11) {
            if (!feed(fs, *cs)) {
                return SAVE_ERR_TRUNCATED;
            }
        } else {
            Decoder d{legacy, sizeof(legacy), 0, true};
            io_fields(d, *cs, true);
            for (int i = 33; i < 64; i++) {
                SecretProjects[i] = -1; // memset 0xFF from FAC_MANIFOLD_HARMONICS
            }
        }
    }
    const int version = *SaveFileVersion;
    if (version < 7 || version > 12) {
        return SAVE_ERR_VERSION;
    }
    if (*GameState & STATE_UNK_10000000) {
        return SAVE_ERR_ENCRYPTED; // Thinker: encrypted DiploPlrList/TimeControl block follows
    }
    if (*GameMoreRules & MRULES_UNK_20) {
        return SAVE_ERR_PBEM; // Thinker: diplomacy string tables after game_io
    }
    debug_ver("game_data ver=%d wrt=%d\n", version, w);
    int32_t player_id = MapWin->cOwner;
    if (!feed(fs, player_id)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!w) {
        if (player_id >= 0 && player_id < MaxPlayerNum) {
            MapWin->cOwner = player_id;
        } else {
            debug("game_data error player=%d\n", player_id);
            return SAVE_ERR_PLAYER;
        }
        for (int i = 0; i < 64; ++i) {
            if (SecretProjects[i] >= MaxBaseNum) {
                debug("game_data error project=%d base=%d\n", i, SecretProjects[i]);
                return SAVE_ERR_PROJECT;
            }
        }
    }
    const bool legacy_records = version < 11;
    if (!w) {
        memset((void*)Factions, 0, sizeof(Faction) * MaxPlayerNum);
    }
    if (!feed_n(fs, Factions, MaxPlayerNum, legacy_records)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed_n(fs, MFactions, MaxPlayerNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed_n(fs, FactionRankingsUnk, MaxPlayerNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!w && !*SaveFileMenu && !(*GameState & STATE_GAME_DONE)) {
        if (bit_count(FactionStatus[0] & FactionStatus[1]) > 1) {
            for (int i = 1; i < MaxPlayerNum; ++i) {
                if (is_alive(i) && is_human(i) && Factions[i].unk_102) {
                    *SaveFileStatus = 1;
                    return SAVE_ERR_SECURITY;
                }
            }
        }
    }
    if (w) {
        if (map_write(fs)) {
            return SAVE_ERR_TRUNCATED;
        }
    } else {
        int error = SAVE_OK;
        if (map_read(fs, &error)) {
            return error;
        }
    }
    if (!feed_n(fs, &FactionTurnMight[0][0], (size_t)MaxTurnMightNum * 8)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed(fs, *ReplayEventSize)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (*ReplayEventSize < 0 || *ReplayEventSize > 8192
    || *VehCount < 0 || *VehCount > conf.max_veh_num || *VehCount > MaxVehNum
    || *BaseCount < 0 || *BaseCount > MaxBaseNum) {
        debug("game_data error bases=%d vehs=%d replay=%d\n", *BaseCount, *VehCount, *ReplayEventSize);
        return SAVE_ERR_COUNTS; // Fix: added bounds checking
    }
    if (*ReplayEventSize) {
        // Replay events are fed as a byte run of ReplayEventSize bytes.
        std::vector<uint8_t> raw;
        if (w) {
            Encoder e{&raw};
            for (int i = 0; i < MaxReplayEventNum; i++) {
                io_fields(e, ReplayEvents[i], false);
            }
            raw.resize((size_t)*ReplayEventSize);
            if (!feed_bytes(fs, raw.data(), raw.size())) {
                return SAVE_ERR_TRUNCATED;
            }
        } else {
            raw.assign((size_t)MaxReplayEventNum * WireSize_ReplayEvent, 0);
            if (!feed_bytes(fs, raw.data(), (size_t)*ReplayEventSize)) {
                return SAVE_ERR_TRUNCATED;
            }
            Decoder d{raw.data(), raw.size(), 0, true};
            for (int i = 0; i < MaxReplayEventNum; i++) {
                io_fields(d, ReplayEvents[i], false);
            }
        }
    } else if (!w) {
        memset((void*)ReplayEvents, 0, sizeof(ReplayEvent) * MaxReplayEventNum);
    }
    if (!feed_n(fs, Continents, MaxRegionNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (w && version < 11) {
        // Inverse of the v<11 weapon id conversion below.
        UNIT units[MaxProtoNum];
        memcpy((void*)units, (const void*)Units, sizeof(units));
        for (int i = 0; i < MaxProtoNum; ++i) {
            if (units[i].weapon_id >= 15) {
                units[i].weapon_id -= 3;
            }
        }
        if (!feed_n(fs, units, MaxProtoNum)) {
            return SAVE_ERR_TRUNCATED;
        }
    } else if (!feed_n(fs, Units, MaxProtoNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed_bytes(fs, st->bunglist, BunglistSize)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!w && version < 11) {
        for (int i = 0; i < MaxProtoNum; ++i) {
            if (Units[i].weapon_id >= 12) {
                Units[i].weapon_id += 3;
            }
        }
    }
    if (!w) {
        memset((void*)Vehs, 0, sizeof(VEH) * MaxVehNum);
    }
    if (!feed_n(fs, Vehs, (size_t)*VehCount)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!w && version <= 7) {
        Units[BSC_MIND_WORMS].icon_offset = 3;
        Units[BSC_ALIEN_ARTIFACT].icon_offset = 2;
    }
    if (!w) {
        memset((void*)Bases, 0, sizeof(BASE) * MaxBaseNum);
    }
    if (w && version < 11) {
        for (int i = 0; i < *BaseCount; ++i) {
            BASE base = Bases[i];
            for (int& item : base.queue_items) {
                if (item <= -65) {
                    item = (int)((uint32_t)item + 31u); // inverse of the read conversion
                }
            }
            if (!feed(fs, base, true)) {
                return SAVE_ERR_TRUNCATED;
            }
        }
    } else {
        if (!feed_n(fs, Bases, (size_t)*BaseCount, version < 11)) {
            return SAVE_ERR_TRUNCATED;
        }
        if (!w && version < 11) {
            for (int i = 0; i < *BaseCount; ++i) {
                for (int& item : Bases[i].queue_items) {
                    if (item <= -34) {
                        item = (int)((uint32_t)item - 31u);
                    }
                }
            }
        }
    }
    if (*GameState & STATE_IS_SCENARIO) {
        if (!feed_bytes(fs, st->scenario_rules, sizeof(st->scenario_rules))) {
            return SAVE_ERR_TRUNCATED;
        }
    } else if (!w) {
        memset(st->scenario_rules, 0, sizeof(st->scenario_rules));
    }
    if (!feed(fs, st->scenario_name)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed(fs, st->objectives)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed_n(fs, Monuments, MaxPlayerNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (version < 12) {
        if (!w) {
            memset(TectonicDetonationCount, 0, 0x20u);
        }
    } else if (!feed_n(fs, TectonicDetonationCount, MaxPlayerNum)) {
        return SAVE_ERR_TRUNCATED;
    }
    int32_t field_23D20 = w ? 0 : MapWin->field_23D20; // Thinker: zeroed before writing
    int index = clamp(MapWin->CursorListIndex, 0, 31);
    if (!feed(fs, MapWin->InVehMode)
    || !feed(fs, MapWin->VehID)
    || !feed(fs, MapWin->CursorListX[index])
    || !feed(fs, MapWin->CursorListY[index])
    || !feed(fs, field_23D20)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!w) {
        MapWin->field_23D20 = field_23D20;
    }
    for (int i = 0; i < 8; ++i) {
        if (game_io(fs, MapWinPtr[i])) {
            return SAVE_ERR_TRUNCATED;
        }
    }
    if (game_io(fs, &WorldWin->oMapWin)) {
        return SAVE_ERR_TRUNCATED;
    }
    if (!feed(fs, *CanvasWidth) || !feed(fs, *CanvasHeight)) {
        return SAVE_ERR_TRUNCATED;
    }
    return SAVE_OK;
}

const char TerranHeader[] = "TERRAN";

int read_container(const uint8_t* data, size_t length, SaveInfo* info) {
    SaveInfo local = {SAVE_OK, 0, false, 0};
    SaveInfo& r = info ? *info : local;
    r = local;
    // header_check: NUL-terminated name then one byte (0x1A)
    if (!data || length < sizeof(TerranHeader) + 1 + 4
    || memcmp(data, TerranHeader, sizeof(TerranHeader)) != 0
    || data[sizeof(TerranHeader)] != 0x1A) {
        return r.error = SAVE_ERR_HEADER;
    }
    size_t pos = sizeof(TerranHeader) + 1;
    int32_t outer = (int32_t)le32(data + pos);
    pos += 4;
    if (outer < 0x56) {
        return r.error = SAVE_ERR_HEADER;
    }
    if (length - pos >= 0x31C) {
        r.legacy_marker = (int)le32(data + pos + LegacyMarkerOffset);
    }
    EngineState* target = engine_state();
    std::unique_ptr<EngineState> tmp(new EngineState(*target));
    engine_bind(tmp.get());
    Stream fs = {false, nullptr, data, length, pos, true};
    int error = game_data(fs, 0);
    if (error == SAVE_OK) {
        if (length - fs.pos >= 4) {
            tmp->save_tick_seed = le32(data + fs.pos);
            fs.pos += 4;
            r.has_tick_seed = true;
        }
        if (fs.pos != length) {
            error = SAVE_ERR_TRUNCATED; // trailing data
        }
    }
    r.consumed = fs.pos;
    r.error = error;
    if (error == SAVE_OK) {
        tmp->current_base = nullptr;
        *target = *tmp;
    }
    engine_bind(target);
    return error;
}

} // namespace

const char* save_error_name(int error) {
    switch (error) {
    case SAVE_OK: return "ok";
    case SAVE_ERR_HEADER: return "not an original TERRAN game";
    case SAVE_ERR_VERSION: return "unsupported save version";
    case SAVE_ERR_ENCRYPTED: return "encrypted save not supported";
    case SAVE_ERR_PBEM: return "PBEM save not supported";
    case SAVE_ERR_TRUNCATED: return "truncated or oversized game file";
    case SAVE_ERR_PLAYER: return "invalid current player";
    case SAVE_ERR_PROJECT: return "invalid secret project base";
    case SAVE_ERR_COUNTS: return "invalid record counts";
    case SAVE_ERR_MAP: return "invalid embedded map";
    case SAVE_ERR_SECURITY: return "save failed tamper check";
    case SAVE_ERR_IO: return "file I/O error";
    }
    return "unknown save error";
}

int game_read_bytes(const uint8_t* data, size_t length, SaveInfo* info) {
    if (!engine_state()) {
        return SAVE_ERR_IO;
    }
    return read_container(data, length, info);
}

int game_write_bytes(std::vector<uint8_t>& out) {
    if (!engine_state()) {
        return SAVE_ERR_IO;
    }
    out.clear();
    out.insert(out.end(), TerranHeader, TerranHeader + sizeof(TerranHeader));
    out.push_back(0x1A);
    uint8_t version[4];
    put_le32(version, 0x56);
    out.insert(out.end(), version, version + 4);
    Stream fs = {true, &out, nullptr, 0, 0, true};
    int error = game_data(fs, 1);
    if (error != SAVE_OK) {
        out.clear();
        return error;
    }
    uint8_t seed[4];
    put_le32(seed, engine_state()->save_tick_seed);
    out.insert(out.end(), seed, seed + 4);
    return SAVE_OK;
}

void upgrade_save_version() {
    EngineState* st = engine_state();
    memset(st->legacy_prefs, 0, sizeof(st->legacy_prefs));
    put_le32(st->legacy_prefs + LegacyMarkerOffset, 12);
    *SaveFileVersion = 12;
}

/*
Thinker mod_load_daemon after game_data(): load-time fixups.
*/
int load_daemon_bytes(const uint8_t* data, size_t length) {
    SaveInfo info;
    int error = game_read_bytes(data, length, &info);
    if (error == SAVE_ERR_HEADER) {
        return SAVE_LOAD_NOT;
    }
    if (error != SAVE_OK) {
        engine_notify(EV_GAME_LOADED, SAVE_LOAD_ERROR, error);
        return error == SAVE_ERR_SECURITY ? SAVE_LOAD_SECURITY : SAVE_LOAD_ERROR;
    }
    fixup_landmarks(); // map_read
    engine_notify(EV_MAP_CHANGED);
    if (*GameRules & RULES_IRONMAN && info.has_tick_seed) {
        game_srand(engine_state()->save_tick_seed);
    }
    *ComputeBaseID = -1;
    // Original: Path_init(Paths) (WP3 path cache) and world_linearize_contours()
    // when called with flag != 0 (map editor) are not part of loading a game.
    memcpy(AltNatural, ElevDetail, sizeof(ElevDetail));
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        Faction* plr = &Factions[fc];
        plr->player_flags &= ~PFLAG_MAP_REVEALED;
        if (plr->satellites_nutrient || plr->satellites_mineral
        || plr->satellites_energy || plr->satellites_ODP) {
            plr->player_flags |= PFLAG_MAP_REVEALED;
        }
        for (int i = 0; i < MaxTechnologyNum; ++i) {
            if ((Tech[i].flags & TFLAG_REVEALS_MAP) && has_tech(i, fc)) {
                plr->player_flags |= PFLAG_MAP_REVEALED;
            }
        }
    }
    for (int fc = 1; fc < MaxPlayerNum; ++fc) {
        bool active = false;
        for (int i = 0; i < *BaseCount; ++i) {
            if (Bases[i].faction_id == fc) {
                active = true;
                break;
            }
        }
        if (!active) {
            for (int i = 0; i < *VehCount; ++i) {
                if (Vehs[i].faction_id == fc && Vehs[i].plan() == PLAN_COLONY) {
                    active = true;
                    break;
                }
            }
        }
        set_alive(fc, active);
    }
    int index = clamp(MapWin->CursorListIndex, 0, 31);
    int cursor_x = MapWin->CursorListX[index];
    int cursor_y = MapWin->CursorListY[index];
    for (int i = 0; i < 32; ++i) {
        MapWin->CursorListX[i] = cursor_x;
        MapWin->CursorListY[i] = cursor_y;
    }
    rebuild_vehicle_bits();
    rebuild_base_bits();
    *ControlTurnMove = MapWin->cOwner;
    if (*MultiplayerActive) {
        *PbemActive = 0;
    } else if ((*GameMoreRules & MRULES_UNK_20) || bit_count(FactionStatus[0]) > 1) {
        *PbemActive = 1; // Original: check_tamper() (PBEM) not supported
    } else {
        *PbemActive = 0;
    }
    *GameDrawState |= 4;
    engine_notify(EV_GAME_LOADED, SAVE_LOAD_VALID);
    return SAVE_LOAD_VALID;
}

int save_daemon_bytes(std::vector<uint8_t>& out) {
    // Original: Ironman/PBEM/network saves set STATE_UNK_10000000 and encrypt;
    // not supported here, so those flags must be clear (game_data rejects them).
    int error = game_write_bytes(out);
    engine_notify(EV_GAME_SAVED, error);
    return error;
}

int load_daemon(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return SAVE_LOAD_NONE;
    }
    std::vector<uint8_t> data;
    uint8_t buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        data.insert(data.end(), buf, buf + n);
    }
    fclose(fp);
    return load_daemon_bytes(data.data(), data.size());
}

int save_daemon(const char* filename) {
    std::vector<uint8_t> data;
    int error = save_daemon_bytes(data);
    if (error != SAVE_OK) {
        return error;
    }
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        return SAVE_ERR_IO;
    }
    bool ok = fwrite(data.data(), 1, data.size(), fp) == data.size();
    ok = (fclose(fp) == 0) && ok;
    if (!ok) {
        remove(filename);
        return SAVE_ERR_IO;
    }
    return SAVE_OK;
}

size_t wire_size_record(const char* name, bool legacy) {
    std::string n = name;
    if (n == "VEH") return WireSize_VEH;
    if (n == "UNIT") return WireSize_UNIT;
    if (n == "BASE") return legacy ? WireSizeLegacy_BASE : WireSize_BASE;
    if (n == "Faction") return legacy ? WireSizeLegacy_Faction : WireSize_Faction;
    if (n == "MFaction") return WireSize_MFaction;
    if (n == "MAP") return WireSize_MAP;
    if (n == "GamePrefs") return legacy ? WireSizeLegacy_GamePrefs : WireSize_GamePrefs;
    if (n == "Monument") return WireSize_Monument;
    if (n == "Continent") return WireSize_Continent;
    if (n == "MapMeta") return WireSize_MapMeta;
    if (n == "ReplayEvent") return WireSize_ReplayEvent;
    return 0;
}

/* Counting visitor check used by tests: the visitors in save_fields.inc must emit
 * exactly the wire size of each record. */
size_t wire_size_counted(const char* name, bool legacy);
size_t wire_size_counted(const char* name, bool legacy) {
    std::string n = name;
    Counter c{0};
    if (n == "VEH") { VEH v{}; io_fields(c, v, legacy); }
    else if (n == "UNIT") { UNIT v{}; io_fields(c, v, legacy); }
    else if (n == "BASE") { BASE v{}; io_fields(c, v, legacy); }
    else if (n == "Faction") { std::unique_ptr<Faction> v(new Faction()); io_fields(c, *v, legacy); }
    else if (n == "MFaction") { MFaction v{}; io_fields(c, v, legacy); }
    else if (n == "MAP") { MAP v{}; io_fields(c, v, legacy); }
    else if (n == "GamePrefs") { GamePrefs v{}; io_fields(c, v, legacy); }
    else if (n == "Monument") { Monument v{}; io_fields(c, v, legacy); }
    else if (n == "Continent") { Continent v{}; io_fields(c, v, legacy); }
    else if (n == "MapMeta") { MapMeta v{}; io_fields(c, v, legacy); }
    else if (n == "ReplayEvent") { ReplayEvent v{}; io_fields(c, v, legacy); }
    return c.n;
}

} // namespace engine
} // namespace alpha
