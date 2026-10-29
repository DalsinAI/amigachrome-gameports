/* SPDX-License-Identifier: GPL-3.0-or-later
 * Ported from Thinker Mod src/engine_data.h at dabe62f2d1abba03de40f3dd3048494843f6a9af.
 * Copyright (c) Thinker Mod authors, MIT licence; see engine/THINKER-LICENSE.
 * Changes: only GamePrefs kept; legacy v2/v10 layouts are handled by the
 * explicit field I/O in savegame.cpp instead of packed copies.
 * Include engine.h rather than this header directly.
 */
#pragma once

namespace alpha {
namespace engine {

struct GamePrefs {
    int32_t GamePreferences;
    int32_t GameMorePreferences;
    int32_t GameWarnings;
    int32_t GameRules;
    int32_t dword_9A64A0;
    int32_t GameInterludeState[2];
    int32_t dword_9A64AC;
    int32_t dword_9A64B0;
    int32_t TutWinMapState[2];
    int32_t dword_9A64BC;
    int32_t GameState;
    int32_t DiffLevel;
    int32_t VehCount;
    int32_t BaseCount;
    int32_t dword_9A64D0;
    int32_t CurrentTurn;
    int32_t CurrentMissionYear;
    int32_t CurrentVehID;
    int32_t GameTimeControl[2];
    uint8_t FactionStatus[4];
    int32_t FactionRankings[8];
    int32_t RankingFactionIDUnk1;
    int32_t RankingFactionIDUnk2;
    int32_t SecretProjects[64];
    int32_t GovernorFaction;
    int32_t ProposalPassCount[11];
    int32_t ProposalCallTurn[11];
    uint8_t TechOwners[89];
    char    LastSavePath[256];
    uint8_t pad0[3];
    int32_t ClimateValueA;
    int32_t ClimateValueB;
    int32_t ClimateValueC;
    int32_t ClimateFutureChange;
    int32_t ScoreBonusPts;
    int32_t ScorePopTotal[8];
    int32_t SunspotDuration;
    int32_t MountPlanetX;
    int32_t MountPlanetY;
    int32_t DustCloudDuration;
    int32_t SaveFileVersion;
    int32_t ScnVictFacilityObj;
    int32_t SolarFlaresEvent;
    int32_t GameMoreRules;
    int32_t CurrentFaction;
    int32_t ClimateLevel;
};

} // namespace engine
} // namespace alpha
