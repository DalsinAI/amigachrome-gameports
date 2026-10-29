# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent integer/wire model for the original technology cache rebuild."""
import struct
from test_original_game import decode, SCHEMA, FORMATS
from test_original_faction import SCHEMA as FACTIONS
from test_units import sections
from test_world_unit_stats import scalar


def effects(text, data, faction):
    if faction not in range(8):
        return 1, None
    info, _, spans = decode(data)
    legacy = int(info['version']) < 11
    state = spans['factions'][faction * (0x20bc if legacy else 0x20cc):]
    definition = spans['faction_definitions'][faction * 0x59c:]
    fs = FACTIONS['LegacyFactionV10' if legacy else 'Faction']
    ds = FACTIONS['MFaction']
    count = scalar(definition, ds, 'faction_bonus_count')
    if count not in range(9):
        return 1, None
    prefs = spans['legacy'] if legacy else spans['preferences']
    owners = next(f for f in SCHEMA['GamePrefs']['fields'] if f['name'] == 'TechOwners')
    offset = owners['offset'] - (0x7c if legacy and owners['offset'] >= 0x184 else 0)
    table = sections(text)['TECHNOLOGY']
    totals = [0, 0, 0, 0, scalar(definition, ds, 'rule_commerce')]
    def increase(index, value):
        totals[index] += value
        return -(1 << 31) <= totals[index] < (1 << 31)
    for index, row in enumerate(table):
        enabled = row[6].lower() != 'disable' and not (row[7].lower() == 'disable' and row[6].lower() != 'none')
        if faction == 0 or index == 88 or not enabled or not prefs[offset + index] & (1 << faction):
            continue
        flags = int(row[8], 2)
        for field, mask in [(0, 256), (1, 128), (2, 64), (4, 4)]:
            if flags & mask and not increase(field, 1):
                return 3, None
    for index in range(count):
        kind = scalar(definition, ds, 'faction_bonus_id', index)
        if kind in [7, 8, 9] and not increase(kind - 7, scalar(definition, ds, 'faction_bonus_val1', index)):
            return 3, None
    totals[:4] = [max(0, value) for value in totals[:4]]
    economy = scalar(state, fs, 'SE_economy_pending')
    if not increase(4, sum(economy > threshold for threshold in [2, 3, 4])):
        return 3, None
    return 0, totals
