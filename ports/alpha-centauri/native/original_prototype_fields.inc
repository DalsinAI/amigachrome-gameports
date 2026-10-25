/* SPDX-License-Identifier: GPL-3.0-or-later
 * Wire offsets from Thinker engine_veh.h::UNIT, MIT,
 * Copyright (c) Thinker Mod authors; see THINKER-LICENSE.txt.
 * Pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Explicit packed Windows wire offsets, NOT native structure layout.
 */
WIRE_STRING(name, 32, 0x0)
WIRE_SCALAR(uint32_t, ability_flags, 0x20)
WIRE_SCALAR(uint8_t, chassis_id, 0x24)
WIRE_SCALAR(uint8_t, weapon_id, 0x25)
WIRE_SCALAR(uint8_t, armor_id, 0x26)
WIRE_SCALAR(uint8_t, reactor_id, 0x27)
WIRE_SCALAR(uint8_t, carry_capacity, 0x28)
WIRE_SCALAR(uint8_t, cost, 0x29)
WIRE_SCALAR(int8_t, plan, 0x2a)
WIRE_SCALAR(int8_t, group_id, 0x2b)
WIRE_SCALAR(uint8_t, obsolete_factions, 0x2c)
WIRE_SCALAR(uint8_t, combat_factions, 0x2d)
WIRE_SCALAR(int8_t, icon_offset, 0x2e)
WIRE_SCALAR(int8_t, pad_1, 0x2f)
WIRE_SCALAR(uint16_t, unit_flags, 0x30)
WIRE_SCALAR(int16_t, preq_tech, 0x32)
