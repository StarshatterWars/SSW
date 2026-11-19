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

    Gameplay loads validated awards.json values into owned rank and medal records.
    DataTable overloads are retained solely for legacy conversion and parity tests.
    No rank or medal DataTable asset is required by boot, gameplay or UI lookups.
=============================================================================*/



#include "AwardInfoRegistry.h"
#include "Logging/LogMacros.h"

TArray<FRankInfo> UAwardInfoRegistry::OwnedRanks;
TArray<FMedalInfo> UAwardInfoRegistry::OwnedMedals;
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

void UAwardInfoRegistry::Initialize(const TArray<FRankInfo>& Ranks, const TArray<FMedalInfo>& Medals)
{
    Reset();
    OwnedRanks = Ranks; OwnedMedals = Medals;
    for (const auto& Row : OwnedRanks) RankById.Add(Row.RankId, &Row);
    for (const auto& Row : OwnedMedals) MedalById.Add(Row.MedalId, &Row);
}

bool UAwardInfoRegistry::IsInitialized()
{
    const bool bHasRanks = RankById.Num() > 0;
    const bool bHasMedals = MedalById.Num() > 0;
    return bHasRanks || bHasMedals;
}

void UAwardInfoRegistry::Reset()
{
    OwnedRanks.Reset(); OwnedMedals.Reset();
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

int32 UAwardInfoRegistry::RankForPoints(int32 Points, int32 UnloadedFallback)
{
    if (RankById.IsEmpty()) return UnloadedFallback;
    int32 Result = 0;
    int32 BestThreshold = -1;
    for (const auto& Pair : RankById)
    {
        const FRankInfo* Row = Pair.Value;
        if (!Row || Row->TotalPoints < 0 || Points < Row->TotalPoints) continue;
        if (Row->TotalPoints > BestThreshold || (Row->TotalPoints == BestThreshold && Row->RankId > Result))
        {
            BestThreshold = Row->TotalPoints;
            Result = Row->RankId;
        }
    }
    return Result;
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

void UAwardInfoRegistry::ForEachMedal(TFunctionRef<void(const FMedalInfo&)> Fn)
{
    TArray<int32> Ids;
    MedalById.GetKeys(Ids);
    Ids.Sort(); // The legacy authored medal table is ordered by ascending flag.
    for (int32 Id : Ids)
        if (const FMedalInfo* Row = FindMedal(Id)) Fn(*Row);
}

int32 UAwardInfoRegistry::RankFromName(const FString& Name)
{
    const FString Key=Name.TrimStartAndEnd();
    if (Key.IsEmpty() || RankById.IsEmpty()) return 0;
    int32 Result=MAX_int32;
    for (const auto& Pair : RankById)
    {
        const FRankInfo* Row=Pair.Value;
        if (Row && (Row->Name.Equals(Key,ESearchCase::IgnoreCase) ||
                    (!Row->Abrv.IsEmpty() && Row->Abrv.Equals(Key,ESearchCase::IgnoreCase))))
            Result=FMath::Min(Result,Row->RankId);
    }
    return Result==MAX_int32 ? 0 : Result;
}
