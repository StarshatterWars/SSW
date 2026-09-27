/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      Awards
    FILE:           AwardInfo.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    FAwardInfo

    Thin static helper facade for rank + medal text lookups.

    This is OPTION B (REGISTRY-BASED):
      - No DataTable pointers at call sites
      - No per-row wrapper storage
      - UI calls remain simple:
            FAwardInfo::RankName(RankId)
            FAwardInfo::RankDescription(RankId)
            FAwardInfo::MedalName(MedalId)
            FAwardInfo::MedalDescription(MedalId)

    Source of truth is now split DataTables:
      - FRankInfo rows (Ranks table)
      - FMedalInfo rows (Medals table)

    UAwardInfoRegistry must be initialized during GameData boot.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"

/**
 * Static award lookup helpers (no loading, no persistence).
 * Delegates to UAwardInfoRegistry (runtime caches).
 */
class STARSHATTERWARS_API FAwardInfo
{
public:
    // ------------------------------------------------------------
    // Ranks
    // ------------------------------------------------------------
    static FString RankName(int32 RankId);
    static FString RankAbrv(int32 RankId);
    static FString RankDescription(int32 RankId);

    // Optional (nice for the AwardShowDlg rank ceremony text)
    static FString RankAwardText(int32 RankId);

    // ------------------------------------------------------------
    // Medals
    // ------------------------------------------------------------
    static FString MedalName(int32 MedalId);
    static FString MedalDescription(int32 MedalId);

    // Optional (award ceremony text distinct from desc in legacy data)
    static FString MedalAwardText(int32 MedalId);
};
