/* SPDX-License-Identifier: GPL-3.0-or-later
 * Random number generators, ported from Thinker Mod src/random.h at
 * dabe62f2d1abba03de40f3dd3048494843f6a9af (MIT; see engine/THINKER-LICENSE).
 *
 * All generator state lives in EngineState::rng (RandomState) so a game is
 * reproducible from its seeds. Nothing reads the clock: next_rand() mixes an
 * explicit tick counter where Thinker used QueryPerformanceCounter and
 * GetTickCount, and setup_random() only mixes that state.
 *
 * game_rand()/game_srand() reproduce the MSVC C runtime rand()/srand() that the
 * original executable called (state * 214013 + 2531011, bits 16..30).
 * random()/random_get() reproduce the executable's Random::get/game_random LCG.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

void random_bind(RandomState* state);

/* Original game RNG (MSVC rand). game_rand returns 0..32767. */
int game_rand();
void game_srand(uint32_t seed);
uint32_t game_rand_state();
int32_t game_randv(int32_t value);

/* game_random(low, high) as called by original code (Random::get). */
int32_t game_random(int32_t low, int32_t high);

uint32_t pair_hash(uint32_t a, uint32_t b);
/* Seed the explicit next_rand() state; replaces the QPC/GetTickCount source. */
void next_rand_seed(uint32_t x, uint32_t y);
uint32_t next_rand();
void setup_random();
void random_reseed(uint32_t value);
uint32_t random_state();
int32_t random(int32_t limit);
int32_t random_get(int32_t low, int32_t high);

class GameRandom {
    private:
    uint32_t* state;
    public:
    explicit GameRandom(uint32_t* backing = nullptr) : state(backing) {}
    void bind(uint32_t* backing) { state = backing; }
    void reseed(uint32_t value);
    uint32_t get_state();
    int32_t get(int32_t limit);
    int32_t get(int32_t low, int32_t high);
    MAP* pick_tile(int dy, int& x, int& y);
};
extern GameRandom map_rand;

template <class T, class C>
const T& pick_random(const std::set<T,C>& s) {
    auto it = std::begin(s);
    std::advance(it, random((int32_t)s.size()));
    return *it;
}

template <class T>
const T& pick_random(const std::set<T>& s) {
    auto it = std::begin(s);
    std::advance(it, random((int32_t)s.size()));
    return *it;
}

} // namespace engine
} // namespace alpha
