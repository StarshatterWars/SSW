/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      AwardInfoRegistry
    FILE:           AwardInfoRegistry.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UAwardInfoRegistry

    Unreal-native runtime registry for award, rank, and medal definitions.

    This class replaces legacy:
        - AwardInfo
        - rank_table
        - medal_table
        - LoadAwardTables()
        - static rank/medal lookup functions

    The registry loads and indexes FS_AwardInfo rows from a DataTable
    (typically DT_Awards) and provides fast, centralized lookup services
    for rank titles, medal names, descriptions, and progression logic.

    RESPONSIBILITIES
    ================
    - Load award definitions from UDataTable
    - Separate rank entries from medal entries
    - Provide rank lookup (RankName, RankDescription, RankAbrv)
    - Provide medal lookup (MedalName, MedalDescription)
    - Compute command eligibility by ship class
    - Determine rank progression by TotalPoints
    - Provide clean runtime API for UI and gameplay systems

    ARCHITECTURAL ROLE
    ==================
    FS_AwardInfo:
        Data-only struct (row definition)

    UAwardInfoRegistry:
        Runtime logic + indexing + lookup layer

    UStarshatterPlayerSubsystem:
        Owns player state (FS_PlayerGameInfo)
        Uses this registry for rank/award display and evaluation

    DESIGN NOTES
    ============
    - No persistence logic lives here.
    - No player state is owned here.
    - No gameplay mutation occurs here.
    - Pure lookup + evaluation layer.
    - Safe for UI use.
    - Safe for subsystem injection.

    FUTURE EXTENSIONS
    =================
    - Cached TMap<int32, const FS_AwardInfo*> for O(1) lookups
    - Rank progression evaluation by PlayerPoints
    - Medal eligibility evaluation helpers
    - Async asset loading for insignia images
=============================================================================*/



#include "AwardInfoRegistry.h"
#include "Logging/LogMacros.h"

TWeakObjectPtr<UDataTable> UAwardInfoRegistry::RankTable;
TWeakObjectPtr<UDataTable> UAwardInfoRegistry::MedalTable;

TMap<int32, const FRankInfo*> UAwardInfoRegistry::RankById;
TMap<int32, const FMedalInfo*> UAwardInfoRegistry::MedalById;

void UAwardInfoRegistry::Initialize(UDataTable* InRankTable, UDataTable* InMedalTable)
{
    RankTable = InRankTable;
    MedalTable = InMedalTable;
    BuildCaches();
}

bool UAwardInfoRegistry::IsInitialized()
{
    const bool bHasRanks = RankTable.IsValid() && RankById.Num() > 0;
    const bool bHasMedals = MedalTable.IsValid() && MedalById.Num() > 0;
    return bHasRanks || bHasMedals;
}

void UAwardInfoRegistry::Reset()
{
    RankTable.Reset();
    MedalTable.Reset();
    RankById.Reset();
    MedalById.Reset();
}

const TCHAR* UAwardInfoRegistry::SafeStr(const FString& S, const TCHAR* Fallback)
{
    return S.IsEmpty() ? Fallback : *S;
}

void UAwardInfoRegistry::BuildCaches()
{
    RankById.Reset();
    MedalById.Reset();

    if (UDataTable* RT = RankTable.Get())
    {
        const TMap<FName, uint8*>& RowMap = RT->GetRowMap();
        for (const TPair<FName, uint8*>& Pair : RowMap)
        {
            const FRankInfo* Row = reinterpret_cast<const FRankInfo*>(Pair.Value);
            if (Row)
                RankById.Add(Row->RankId, Row);
        }

        UE_LOG(LogTemp, Log, TEXT("[AwardInfoRegistry] Cached %d ranks from '%s'."),
            RankById.Num(), *RT->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AwardInfoRegistry] RankTable is null."));
    }

    if (UDataTable* MT = MedalTable.Get())
    {
        const TMap<FName, uint8*>& RowMap = MT->GetRowMap();
        for (const TPair<FName, uint8*>& Pair : RowMap)
        {
            const FMedalInfo* Row = reinterpret_cast<const FMedalInfo*>(Pair.Value);
            if (Row)
                MedalById.Add(Row->MedalId, Row);
        }

        UE_LOG(LogTemp, Log, TEXT("[AwardInfoRegistry] Cached %d medals from '%s'."),
            MedalById.Num(), *MT->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[AwardInfoRegistry] MedalTable is null."));
    }
}

// -------------------- Ranks --------------------

const FRankInfo* UAwardInfoRegistry::FindRank(int32 RankId)
{
    return FindByKey<FRankInfo, int32>(RankById, RankId);
}

const TCHAR* UAwardInfoRegistry::RankName(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return SafeStr(Row->Name, TEXT("Cadet"));
    return TEXT("Cadet");
}

const TCHAR* UAwardInfoRegistry::RankAbrv(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return SafeStr(Row->Abrv, TEXT(""));
    return TEXT("");
}

const TCHAR* UAwardInfoRegistry::RankDescription(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return SafeStr(Row->Desc, TEXT(""));
    return TEXT("");
}

// -------------------- Medals --------------------

const FMedalInfo* UAwardInfoRegistry::FindMedal(int32 MedalId)
{
    return FindByKey<FMedalInfo, int32>(MedalById, MedalId);
}

const TCHAR* UAwardInfoRegistry::MedalName(int32 MedalId)
{
    if (const FMedalInfo* Row = FindMedal(MedalId))
        return SafeStr(Row->Name, TEXT(""));
    return TEXT("");
}

const TCHAR* UAwardInfoRegistry::MedalAbrv(int32 /*MedalId*/)
{
    // If medals truly don’t have abbreviations, delete this API.
    return TEXT("");
}

const TCHAR* UAwardInfoRegistry::MedalDescription(int32 MedalId)
{
    if (const FMedalInfo* Row = FindMedal(MedalId))
        return SafeStr(Row->Desc, TEXT(""));
    return TEXT("");
}
