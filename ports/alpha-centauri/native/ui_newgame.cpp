/* SPDX-License-Identifier: GPL-3.0-or-later
 * New game screens (WP7, new code). See ui_newgame.h. The choices follow the
 * original setup dialogs (config_game difficulty and rules, map_menu planet
 * size and conditions) without their windows; the world is built by
 * engine_new_game and the PLANETFALL popup it raises is captured for the
 * planetfall screen.
 */
#include "ui_newgame.h"

#include <stdio.h>

namespace alpha {
namespace ui {

using namespace engine;

static const char* const SizeNames[] = {"Tiny planet", "Small planet", "Standard planet",
    "Large planet", "Huge planet", "Random"};
static const char* const OceanNames[] = {"30-50% of surface", "50-70% of surface", "70-90% of surface", "Random"};
static const char* const ErosionNames[] = {"Strong", "Average", "Weak", "Random"};
static const char* const NativeNames[] = {"Rare", "Average", "Abundant", "Random"};
static const char* const CloudNames[] = {"Sparse", "Average", "Dense", "Random"};
static const char* const OrbitNames[] = {"Colder", "Average", "Hotter", "Random"}; // world_temperature: higher is warmer
static const char* const DiffNames[] = {"Citizen", "Specialist", "Talent", "Librarian", "Thinker", "Transcend"};

/* Captures the PLANETFALL popup that engine_new_game raises and forwards
 * everything to the sink that was installed before. */
class PlanetfallSink : public EventSink {
    public:
    explicit PlanetfallSink(EventSink* next, NewGameState& state) : next_(next), state_(state) {}
    int popup(const PopupRequest& request) override {
        if (!strcmp(request.label, "PLANETFALL")) {
            state_.planetfall_title = request.strings[0] ? request.strings[0] : "";
            state_.planetfall_leader = request.strings[1] ? request.strings[1] : "";
            state_.planetfall_noun = request.strings[2] ? request.strings[2] : "";
            state_.landing_x = request.numbers[0];
            state_.landing_y = request.numbers[1];
            return 0;
        }
        return next_ ? next_->popup(request) : 0;
    }
    void redraw(const RedrawRequest& request) override {
        if (next_) {
            next_->redraw(request);
        }
    }
    void notify(const EngineEvent& event) override {
        if (next_) {
            next_->notify(event);
        }
    }
    private:
    EventSink* next_;
    NewGameState& state_;
};

static int row_count(const NewGameState& s) {
    switch (s.screen) {
    case NG_FACTION: return MaxPlayerNum - 1;
    case NG_SETTINGS: return NGS_COUNT;
    case NG_PLANETFALL: return 1;
    default: return 0;
    }
}

static int cycle(int value, int count, int step) {
    return ((value + step) % count + count) % count;
}

static const char* difficulty_name(int i) {
    if (Difficulty && Difficulty[i] && Difficulty[i][0]) {
        return Difficulty[i];
    }
    return DiffNames[i];
}

static std::string faction_label(int faction_id) {
    MFaction* m = &MFactions[faction_id];
    char buf[160];
    if (m->formal_name_faction[0]) {
        snprintf(buf, sizeof(buf), "%s (%s %s)", m->formal_name_faction, m->title_leader, m->name_leader);
    } else {
        snprintf(buf, sizeof(buf), "Faction %d", faction_id);
    }
    return buf;
}

void newgame_open(NewGameState& state, uint32_t seed) {
    state = NewGameState();
    state.seed = seed ? seed : 1;
    state.screen = NG_FACTION;
}

static int random_or(int value, int count) {
    return value >= count ? -1 : value;
}

NewGameSettings newgame_settings(const NewGameState& s) {
    NewGameSettings ng;
    ng.seed = s.seed;
    ng.player_faction = s.player_faction;
    ng.difficulty = s.difficulty;
    ng.map_size = random_or(s.size, WorldSizeCount);
    ng.ocean = random_or(s.ocean, 3);
    ng.erosion = random_or(s.erosion, 3);
    ng.native = random_or(s.native, 3);
    ng.cloud = random_or(s.cloud, 3);
    ng.orbit = random_or(s.orbit, 3);
    ng.rules = DefaultRules & ~RULES_BLIND_RESEARCH;
    if (s.blind_research) {
        ng.rules |= RULES_BLIND_RESEARCH;
    }
    if (s.look_first) {
        ng.rules |= RULES_LOOK_FIRST;
    }
    return ng;
}

static void change_setting(NewGameState& s, int step) {
    switch (s.row) {
    case NGS_DIFFICULTY: s.difficulty = cycle(s.difficulty, 6, step); break;
    case NGS_SIZE: s.size = cycle(s.size, WorldSizeCount + 1, step); break;
    case NGS_OCEAN: s.ocean = cycle(s.ocean, 4, step); break;
    case NGS_EROSION: s.erosion = cycle(s.erosion, 4, step); break;
    case NGS_NATIVE: s.native = cycle(s.native, 4, step); break;
    case NGS_CLOUD: s.cloud = cycle(s.cloud, 4, step); break;
    case NGS_ORBIT: s.orbit = cycle(s.orbit, 4, step); break;
    case NGS_LOOK_FIRST: s.look_first = !s.look_first; break;
    case NGS_BLIND_RESEARCH: s.blind_research = !s.blind_research; break;
    case NGS_SEED: s.seed = (uint32_t)(s.seed + (uint32_t)step); break;
    default: break;
    }
}

static void start_game(NewGameState& s) {
    PlanetfallSink sink(event_sink(), s);
    EventSink* previous = set_event_sink(&sink);
    s.start_status = engine_new_game(newgame_settings(s));
    set_event_sink(previous);
    if (s.start_status == 0) {
        s.screen = NG_PLANETFALL;
        s.row = 0;
    }
}

bool newgame_key(NewGameState& s, NewGameKey key) {
    const int rows = row_count(s);
    switch (key) {
    case NGK_UP:
        if (rows <= 0) {
            return false;
        }
        s.row = cycle(s.row, rows, -1);
        return true;
    case NGK_DOWN:
        if (rows <= 0) {
            return false;
        }
        s.row = cycle(s.row, rows, 1);
        return true;
    case NGK_LEFT:
    case NGK_RIGHT:
        if (s.screen != NG_SETTINGS) {
            return false;
        }
        change_setting(s, key == NGK_LEFT ? -1 : 1);
        return true;
    case NGK_ENTER:
        if (s.screen == NG_FACTION) {
            s.player_faction = s.row + 1;
            s.screen = NG_SETTINGS;
            s.row = NGS_START;
            return true;
        }
        if (s.screen == NG_SETTINGS) {
            if (s.row != NGS_START) {
                change_setting(s, 1);
                return true;
            }
            start_game(s);
            return true;
        }
        if (s.screen == NG_PLANETFALL) {
            s.screen = NG_DONE;
            return true;
        }
        return false;
    case NGK_BACK:
        if (s.screen == NG_SETTINGS) {
            s.screen = NG_FACTION;
            s.row = s.player_faction - 1;
            return true;
        }
        if (s.screen == NG_FACTION) {
            s.screen = NG_CANCELLED;
            return true;
        }
        return false;
    }
    return false;
}

static void add(NewGameView& v, const std::string& text, bool selected = false, bool heading = false) {
    v.lines.push_back({text, selected, heading});
}

static std::string setting_line(const char* label, const char* value) {
    char buf[160];
    snprintf(buf, sizeof(buf), "%-18s %s", label, value);
    return buf;
}

NewGameView newgame_view(const NewGameState& s) {
    NewGameView v;
    v.screen = s.screen;
    char buf[200];
    switch (s.screen) {
    case NG_FACTION:
        v.title = "Choose your faction";
        for (int i = 1; i < MaxPlayerNum; i++) {
            add(v, faction_label(i), s.row == i - 1);
        }
        break;
    case NG_SETTINGS:
        v.title = "Planet and rules";
        add(v, faction_label(s.player_faction), false, true);
        add(v, setting_line("Difficulty", difficulty_name(s.difficulty)), s.row == NGS_DIFFICULTY);
        add(v, setting_line("Planet size", SizeNames[s.size]), s.row == NGS_SIZE);
        add(v, setting_line("Ocean coverage", OceanNames[s.ocean]), s.row == NGS_OCEAN);
        add(v, setting_line("Erosive forces", ErosionNames[s.erosion]), s.row == NGS_EROSION);
        add(v, setting_line("Native life", NativeNames[s.native]), s.row == NGS_NATIVE);
        add(v, setting_line("Cloud cover", CloudNames[s.cloud]), s.row == NGS_CLOUD);
        add(v, setting_line("Planetary orbit", OrbitNames[s.orbit]), s.row == NGS_ORBIT);
        add(v, setting_line("Look first", s.look_first ? "Yes" : "No"), s.row == NGS_LOOK_FIRST);
        add(v, setting_line("Blind research", s.blind_research ? "Yes" : "No"), s.row == NGS_BLIND_RESEARCH);
        snprintf(buf, sizeof(buf), "%u", (unsigned)s.seed);
        add(v, setting_line("Planet seed", buf), s.row == NGS_SEED);
        add(v, s.start_status > 0 ? "Start (settings rejected)" : "Start", s.row == NGS_START);
        break;
    case NG_PLANETFALL:
        v.title = "Planetfall";
        snprintf(buf, sizeof(buf), "%s %s of the %s", s.planetfall_title.c_str(),
            s.planetfall_leader.c_str(), s.planetfall_noun.c_str());
        add(v, buf, false, true);
        snprintf(buf, sizeof(buf), "Landing site %d, %d on a %d x %d planet.", s.landing_x, s.landing_y,
            *MapAreaX, *MapAreaY);
        add(v, buf);
        add(v, "Continue", true);
        break;
    case NG_DONE:
    case NG_CANCELLED:
        break;
    }
    return v;
}

bool newgame_done(const NewGameState& s) {
    return s.screen == NG_DONE || s.screen == NG_CANCELLED;
}

} // namespace ui
} // namespace alpha
