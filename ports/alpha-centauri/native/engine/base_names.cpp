/* SPDX-License-Identifier: GPL-3.0-or-later
 * WP2: base naming. mod_name_base is Thinker Mod src/game.cpp at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE);
 * with conf.new_base_names = 0 (vanilla) it calls name_base, which follows
 * the original executable's name_base (0x4E4090) as documented by OpenSMACX
 * src/base.cpp (GPL-3.0). make_base_unique follows OpenSMACX as well.
 *
 * Changes: the faction text files are not read here. The #BASES /
 * #WATERBASES lists of each faction file and #GENERIC of basename.txt come
 * from base_name_lists, filled by the rules/text loader; text_open() failing
 * in the original corresponds to an empty list. Thinker's new_base_names
 * mode (basenames\*.txt, random variations) is not ported; it falls back to
 * name_base with a "Fix:" note below.
 */
#include "engine.h"
#include "base_wp2.h"

namespace alpha {
namespace engine {

BaseNameLists base_name_lists;

/* The scrambled offset of the original: an LFSR seeded from the map seed and
 * faction id walks positions below `total`. */
static uint32_t name_offset_scramble(int faction_id, uint32_t offset, uint32_t total) {
    if (offset <= 1 || offset > total) {
        return offset;
    }
    int seed = ((*MapRandomSeed + faction_id) & 0xFE) | 1;
    uint32_t loop = 1;
    do {
        if (seed & 1) {
            seed ^= 0x170;
        }
        seed >>= 1;
    } while (seed >= (int)total || ++loop != offset);
    return seed + 1;
}

void __cdecl name_base(int faction_id, char* name, bool save_offset, bool sea_base) {
    if (faction_id < 0 || faction_id >= MaxPlayerNum || !name) {
        assert(0);
        return;
    }
    Faction& f = Factions[faction_id];
    const std::vector<std::string>& sea = base_name_lists.sea[faction_id];
    const std::vector<std::string>& land = base_name_lists.land[faction_id];
    const std::vector<std::string>& generic = base_name_lists.generic;
    if (sea_base && !sea.empty()) {
        uint32_t offset_sea = name_offset_scramble(faction_id,
            (uint32_t)f.base_sea_name_offset + 1, (uint32_t)sea.size());
        if (offset_sea <= sea.size()) {
            strcpy_n(name, MaxBaseNameLen, sea[offset_sea - 1].c_str());
            if (save_offset) {
                f.base_sea_name_offset++;
            }
            return;
        }
    }
    // Land base names or generic
    uint32_t offset = (uint32_t)f.base_name_offset + 1;
    snprintf(name, MaxBaseNameLen, "%s %u", get_noun(faction_id), offset); // default if names exhausted
    if (save_offset) {
        f.base_name_offset++;
    }
    if (land.empty()) {
        return;
    }
    offset = name_offset_scramble(faction_id, offset, (uint32_t)land.size());
    if (offset <= land.size()) {
        strcpy_n(name, MaxBaseNameLen, land[offset - 1].c_str());
        return;
    }
    // Faction list exhausted: continue counting into basename.txt #GENERIC.
    uint32_t index = offset - (uint32_t)land.size();
    if (index <= generic.size()) {
        strcpy_n(name, MaxBaseNameLen, generic[index - 1].c_str());
    }
}

/*
Generate a base name. Thinker's new_base_names mode reads basenames\*.txt;
Fix: that mode is not ported, both settings use the original name_base.
*/
void __cdecl mod_name_base(int faction_id, char* name, bool save_offset, bool water) {
    name_base(faction_id, name, save_offset, water);
}

/*
Check if the base name is unique. If not, append "-2", "-3", ... after it.
*/
void __cdecl make_base_unique(int base_id) {
    if (base_id < 0 || base_id >= *BaseCount) {
        assert(0);
        return;
    }
    int found = 0;
    std::string core_unique_name;
    for (int i = 0; i < *BaseCount; i++) {
        if (base_id != i && !strcmp(Bases[i].name, Bases[base_id].name)) {
            if (!found) { // only create core part of base name on first collision
                Bases[base_id].name[21] = 0;
                core_unique_name = Bases[base_id].name;
            }
            std::string unique_name = core_unique_name;
            unique_name += "-" + std::to_string(++found + 1); // start with appending "-2"
            strcpy_n(Bases[base_id].name, sizeof(Bases[base_id].name), unique_name.c_str());
            i = -1; // Fix: OpenSMACX restarted at 1 and skipped base 0
        }
    }
}

} // namespace engine
} // namespace alpha
