#include "AwardInfoRegistry.h"
#include "Logging/LogMacros.h"

TWeakObjectPtr<UDataTable> UAwardInfoRegistry::RanksTable;
TWeakObjectPtr<UDataTable> UAwardInfoRegistry::MedalsTable;

TMap<int32, const FRankInfo*>  UAwardInfoRegistry::RankById;
TMap<int32, const FMedalInfo*> UAwardInfoRegistry::MedalById;

TArray<const FRankInfo*> UAwardInfoRegistry::RanksSortedByPoints;

// ============================================================
// Initialization
// ============================================================

void UAwardInfoRegistry::Initialize(UDataTable* InRanksTable, UDataTable* InMedalsTable)
{
    RanksTable = InRanksTable;
    MedalsTable = InMedalsTable;

    BuildCaches();
}

bool UAwardInfoRegistry::IsInitialized()
{
    return (RankById.Num() > 0 || MedalById.Num() > 0);
}

void UAwardInfoRegistry::Reset()
{
    RanksTable.Reset();
    MedalsTable.Reset();

    RankById.Reset();
    MedalById.Reset();
    RanksSortedByPoints.Reset();
}

// ============================================================
// Cache Builder
// ============================================================

void UAwardInfoRegistry::BuildCaches()
{
    RankById.Reset();
    MedalById.Reset();
    RanksSortedByPoints.Reset();

    // -------------------------
    // Ranks
    // -------------------------
    if (UDataTable* DT = RanksTable.Get())
    {
        const TMap<FName, uint8*>& RowMap = DT->GetRowMap();

        for (const TPair<FName, uint8*>& Pair : RowMap)
        {
            const FRankInfo* Row =
                reinterpret_cast<const FRankInfo*>(Pair.Value);

            if (!Row)
                continue;

            RankById.Add(Row->RankId, Row);
            RanksSortedByPoints.Add(Row);
        }

        // Sort by promotion threshold ascending
        RanksSortedByPoints.Sort([](const FRankInfo* A, const FRankInfo* B)
            {
                if (!A) return false;
                if (!B) return true;

                if (A->TotalPoints != B->TotalPoints) {
                    return A->TotalPoints < B->TotalPoints;
                }

                return A->RankId < B->RankId;
            });

        UE_LOG(LogTemp, Log,
            TEXT("[AwardInfoRegistry] Cached %d ranks."),
            RankById.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[AwardInfoRegistry] RanksTable is null."));
    }

    // -------------------------
    // Medals
    // -------------------------
    if (UDataTable* DT = MedalsTable.Get())
    {
        const TMap<FName, uint8*>& RowMap = DT->GetRowMap();

        for (const TPair<FName, uint8*>& Pair : RowMap)
        {
            const FMedalInfo* Row =
                reinterpret_cast<const FMedalInfo*>(Pair.Value);

            if (!Row)
                continue;

            MedalById.Add(Row->MedalId, Row);
        }

        UE_LOG(LogTemp, Log,
            TEXT("[AwardInfoRegistry] Cached %d medals."),
            MedalById.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[AwardInfoRegistry] MedalsTable is null."));
    }
}

// ============================================================
// Rank Lookups
// ============================================================

const FRankInfo* UAwardInfoRegistry::FindRank(int32 RankId)
{
    if (const FRankInfo* const* Found = RankById.Find(RankId))
        return *Found;

    return nullptr;
}

FString UAwardInfoRegistry::RankName(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return Row->RankName;

    return TEXT("Conscript");
}

FString UAwardInfoRegistry::RankAbrv(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return Row->RankAbrv;

    return TEXT("");
}

FString UAwardInfoRegistry::RankDescription(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return Row->RankDesc;

    return TEXT("");
}

FString UAwardInfoRegistry::RankAwardText(int32 RankId)
{
    if (const FRankInfo* Row = FindRank(RankId))
        return Row->RankAwardText;

    return TEXT("");
}

int32 UAwardInfoRegistry::RankFromName(const FString& InRankName)
{
    if (InRankName.IsEmpty())
        return 0;

    for (const TPair<int32, const FRankInfo*>& Pair : RankById)
    {
        const FRankInfo* Row = Pair.Value;
        if (!Row)
            continue;

        if (Row->RankName.Equals(InRankName, ESearchCase::IgnoreCase) ||
            Row->RankAbrv.Equals(InRankName, ESearchCase::IgnoreCase))
        {
            return Row->RankId;
        }
    }

    return 0;
}

int32 UAwardInfoRegistry::RankIdFromName(const FString& RankName)
{
    return RankFromName(RankName);
}

// ============================================================
// Progression / Command
// ============================================================

int32 UAwardInfoRegistry::RankForTotalPoints(int32 TotalPoints)
{
    if (RanksSortedByPoints.Num() == 0)
        return 0;

    int32 BestRankId = RanksSortedByPoints[0]->RankId;

    for (const FRankInfo* Rank : RanksSortedByPoints)
    {
        if (!Rank)
            continue;

        if (TotalPoints >= Rank->TotalPoints)
            BestRankId = Rank->RankId;
        else
            break;
    }

    return BestRankId;
}

int32 UAwardInfoRegistry::CommandRankRequired(int32 ShipClassMask)
{
    if (ShipClassMask <= 0 || RanksSortedByPoints.Num() == 0)
        return 0;

    for (const FRankInfo* Rank : RanksSortedByPoints)
    {
        if (!Rank)
            continue;

        if ((ShipClassMask & Rank->GrantedShipClasses) != 0)
            return Rank->RankId;
    }

    return RanksSortedByPoints.Last()->RankId;
}

bool UAwardInfoRegistry::CanCommand(int32 ShipClassMask, int32 PlayerRankId)
{
    const FRankInfo* Rank = FindRank(PlayerRankId);
    if (!Rank)
        return false;

    return (ShipClassMask & Rank->GrantedShipClasses) != 0;
}

// ============================================================
// Medal Lookups
// ============================================================

const FMedalInfo* UAwardInfoRegistry::FindMedal(int32 MedalId)
{
    if (const FMedalInfo* const* Found = MedalById.Find(MedalId))
        return *Found;

    return nullptr;
}

FString UAwardInfoRegistry::MedalName(int32 MedalId)
{
    if (const FMedalInfo* Row = FindMedal(MedalId))
        return Row->MedalName;

    return TEXT("");
}

FString UAwardInfoRegistry::MedalDescription(int32 MedalId)
{
    if (const FMedalInfo* Row = FindMedal(MedalId))
        return Row->MedalDesc;

    return TEXT("");
}

FString UAwardInfoRegistry::MedalAwardText(int32 MedalId)
{
    if (const FMedalInfo* Row = FindMedal(MedalId))
        return Row->MedalAwardText;

    return TEXT("");
}
