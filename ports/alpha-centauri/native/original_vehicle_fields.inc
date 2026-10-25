/* SPDX-License-Identifier: GPL-3.0-or-later
 * Wire offsets from Thinker engine_veh.h::VEH, MIT,
 * Copyright (c) Thinker Mod authors; see THINKER-LICENSE.txt.
 * Pin dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Explicit packed Windows wire offsets, NOT native structure layout.
 */
WIRE_SCALAR(int16_t, x, 0x0)
WIRE_SCALAR(int16_t, y, 0x2)
WIRE_SCALAR(uint32_t, state, 0x4)
WIRE_SCALAR(uint16_t, flags, 0x8)
WIRE_SCALAR(int16_t, unit_id, 0xa)
WIRE_SCALAR(int16_t, pad_0, 0xc)
WIRE_SCALAR(uint8_t, faction_id, 0xe)
WIRE_SCALAR(uint8_t, year_end_lurking, 0xf)
WIRE_SCALAR(uint8_t, damage_taken, 0x10)
WIRE_SCALAR(uint8_t, order, 0x11)
WIRE_SCALAR(uint8_t, waypoint_count, 0x12)
WIRE_SCALAR(uint8_t, patrol_current_point, 0x13)
WIRE_ARRAY(int16_t, waypoint_x, 4, 0x14)
WIRE_ARRAY(int16_t, waypoint_y, 4, 0x1c)
WIRE_SCALAR(uint8_t, morale, 0x24)
WIRE_SCALAR(uint8_t, movement_turns, 0x25)
WIRE_SCALAR(uint8_t, order_auto_type, 0x26)
WIRE_SCALAR(uint8_t, visibility, 0x27)
WIRE_SCALAR(uint8_t, moves_spent, 0x28)
WIRE_SCALAR(int8_t, rotate_angle, 0x29)
WIRE_SCALAR(uint8_t, iter_count, 0x2a)
WIRE_SCALAR(uint8_t, status_icon, 0x2b)
WIRE_SCALAR(uint8_t, probe_action, 0x2c)
WIRE_SCALAR(uint8_t, probe_sabotage_id, 0x2d)
WIRE_SCALAR(int16_t, home_base_id, 0x2e)
WIRE_SCALAR(int16_t, next_veh_id_stack, 0x30)
WIRE_SCALAR(int16_t, prev_veh_id_stack, 0x32)
