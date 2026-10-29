/* SPDX-License-Identifier: GPL-3.0-or-later
 * Ported from Thinker Mod src/random.cpp at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: state moved into EngineState::rng; game_rand/game_srand implemented
 * (MSVC CRT algorithm) instead of calling the executable; no clock sources.
 */
#include "engine.h"

namespace alpha {
namespace engine {

static RandomState fallback_state;
static RandomState* rs = &fallback_state;
GameRandom map_rand(&fallback_state.map_rand_state);

void random_bind(RandomState* state) {
    rs = state ? state : &fallback_state;
    map_rand.bind(&rs->map_rand_state);
}

int game_rand() {
    rs->game_rand_state = rs->game_rand_state * 214013u + 2531011u;
    return (int)((rs->game_rand_state >> 16) & 0x7FFF);
}

void game_srand(uint32_t seed) {
    rs->game_rand_state = seed;
}

uint32_t game_rand_state() {
    return rs->game_rand_state;
}

int32_t game_randv(int32_t value) {
    // RNG state only advances when the value is within bounds
    return (value > 1 ? game_rand() % value : 0);
}

int32_t game_random(int32_t low, int32_t high) {
    return random_get(low, high);
}

uint32_t pair_hash(uint32_t a, uint32_t b) {
    uint32_t x;
    x = (a ^ (b << 16) ^ (~b >> 16));
    x *= 0x603a32a7;
    x ^= (x >> 16);
    x *= 0x5a522677;
    x ^= (x >> 16);
    return x;
}

static inline uint32_t rotl(const uint32_t x, int k) {
    return (x << k) | (x >> (32 - k));
}

void next_rand_seed(uint32_t x, uint32_t y) {
    rs->next_rand_x = x;
    rs->next_rand_y = y;
    rs->next_rand_tick = 0;
}

uint32_t next_rand() {
    uint32_t& nx = rs->next_rand_x;
    uint32_t& ny = rs->next_rand_y;
    if (!nx && !ny) {
        nx = 0x9E3779B9u;
        ny = 0x7F4A7C15u + rotl(nx, 16);
    }
    uint32_t t = nx ^ (nx << 8) ^ (rs->next_rand_tick++);
    nx = ny;
    ny = (ny ^ (ny >> 22)) ^ (t ^ (t >> 9));
    return rotl(nx + ny, 7) + ny;
}

void setup_random() {
    uint32_t seed = next_rand();
    random_reseed(seed);
    map_rand.reseed(seed ^ 0xffff);
    debug("random_reseed %u\n", seed);
}

void random_reseed(uint32_t value) {
    rs->random_seed = value;
}

uint32_t random_state() {
    return rs->random_seed;
}

/*
Returns same values as the game engine function game_random(0, n) and Random::get(0, n).
*/
int32_t random(int32_t limit) {
    rs->random_seed = 1664525 * rs->random_seed + 1013904223;
    return (int32_t)(((rs->random_seed & 0xffff) * (uint32_t)limit) >> 16);
}

int32_t random_get(int32_t low, int32_t high) {
    rs->random_seed = 1664525 * rs->random_seed + 1013904223;
    if (low > high) {
        std::swap(low, high);
    }
    return (int32_t)((uint32_t)low + (((rs->random_seed & 0xffff) * (uint32_t)(high - low)) >> 16));
}

void GameRandom::reseed(uint32_t value) {
    *state = value;
}

uint32_t GameRandom::get_state() {
    return *state;
}

int32_t GameRandom::get(int32_t low, int32_t high) {
    *state = 1664525 * *state + 1013904223;
    if (low > high) {
        std::swap(low, high);
    }
    return (int32_t)((uint32_t)low + (((*state & 0xffff) * (uint32_t)(high - low)) >> 16));
}

int32_t GameRandom::get(int32_t limit) {
    *state = 1664525 * *state + 1013904223;
    return (int32_t)(((*state & 0xffff) * (uint32_t)limit) >> 16);
}

MAP* GameRandom::pick_tile(int dy, int& x, int& y) {
    y = get(*MapAreaY - 2*dy) + dy;
    x = get(*MapAreaX);
    x = ((x ^ y) & 1) ^ x;
    return mapsq(x, y);
}

} // namespace engine
} // namespace alpha
