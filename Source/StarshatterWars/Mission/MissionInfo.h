/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         MissionInfo.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    MissionInfo
    - Lightweight campaign-level mission metadata
    - Controls availability rules and mission instancing
*/

#pragma once

#include "Text.h"

// Forward declarations only (keep header light):
class Mission;

// +--------------------------------------------------------------------+

class MissionInfo
{
public:
    static const char* TYPENAME() { return "MissionInfo"; }

    MissionInfo();
    ~MissionInfo();

    int operator == (const MissionInfo& m) const { return id == m.id; }
    int operator <  (const MissionInfo& m) const { return id < m.id; }
    int operator <= (const MissionInfo& m) const { return id <= m.id; }

    bool     IsAvailable();

public:
    int      id;
    Text     name;
    Text     player_info;
    Text     description;
    Text     objective;
    Text     system;
    Text     region;
    Text     script;
    int      start;
    int      end;
    int      type;

    int      min_rank;
    int      max_rank;
    int      action_id;
    int      action_status;
    int      exec_once;
    int      start_before;
    int      start_after;

    // Owned mission instance (lazy-loaded by Campaign):
    Mission* mission;
};
