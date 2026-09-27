/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      AwardInfoRegistry
    FILE:           AwardInfoRegistry.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UAwardInfoRegistry

    Runtime registry for:
      - Rank lookups (FRankInfo DataTable)
      - Medal lookups (FMedalInfo DataTable)

    This replaces legacy:
      - rank_table
      - medal_table
      - PlayerCharacter static rank/medal helpers
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"

#include "GameStructs.h"    // FRankInfo, FMedalInfo
#include "AwardInfoRegistry.generated.h"

UCLASS()
class STARSHATTERWARS_API UAwardInfoRegistry : public UObject
{
    GENERATED_BODY()

public:

    // Initialize once after GameData loads DataTables
    static void Initialize(UDataTable* InRanksTable, UDataTable* InMedalsTable);

    static bool IsInitialized();
    static void Reset();

    // ------------------------------------------------------------
    // RANK LOOKUPS
    // ------------------------------------------------------------
    static const FRankInfo* FindRank(int32 RankId);

    static FString RankName(int32 RankId);
    static FString RankAbrv(int32 RankId);
    static FString RankDescription(int32 RankId);
    static FString RankAwardText(int32 RankId);

    static int32 RankFromName(const FString& RankName);
    static int32 RankIdFromName(const FString& RankName);

    // Progression / command helpers
    static int32 RankForTotalPoints(int32 TotalPoints);
    static int32 CommandRankRequired(int32 ShipClassMask);
    static bool  CanCommand(int32 ShipClassMask, int32 PlayerRankId);

    // ------------------------------------------------------------
    // MEDAL LOOKUPS
    // ------------------------------------------------------------
    static const FMedalInfo* FindMedal(int32 MedalId);

    static FString MedalName(int32 MedalId);
    static FString MedalDescription(int32 MedalId);
    static FString MedalAwardText(int32 MedalId);

private:

    static void BuildCaches();

private:

    static TWeakObjectPtr<UDataTable> RanksTable;
    static TWeakObjectPtr<UDataTable> MedalsTable;

    static TMap<int32, const FRankInfo*>  RankById;
    static TMap<int32, const FMedalInfo*> MedalById;

    // Sorted by TotalPoints ascending
    static TArray<const FRankInfo*> RanksSortedByPoints;
};
