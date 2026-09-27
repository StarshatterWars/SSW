#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/DataTable.h"
#include "GameStructs.h" // for FRankInfo/FMedalInfo (or move them)
#include "AwardInfoRegistry.generated.h"

UCLASS()
class STARSHATTERWARS_API UAwardInfoRegistry : public UObject
{
    GENERATED_BODY()

public:
    static void Initialize(UDataTable* InRankTable, UDataTable* InMedalTable);
    static bool IsInitialized();
    static void Reset();

    // Ranks
    static const FRankInfo* FindRank(int32 RankId);
    static const TCHAR* RankName(int32 RankId);
    static const TCHAR* RankAbrv(int32 RankId);
    static const TCHAR* RankDescription(int32 RankId);

    // Medals
    static const FMedalInfo* FindMedal(int32 MedalId);
    static const TCHAR* MedalName(int32 MedalId);
    static const TCHAR* MedalAbrv(int32 MedalId);          // optional if medals don’t have abrv
    static const TCHAR* MedalDescription(int32 MedalId);

private:
    static void BuildCaches();

    template<typename TRow, typename TKey>
    static const TRow* FindByKey(const TMap<TKey, const TRow*>& Map, TKey Key)
    {
        if (const TRow* const* Found = Map.Find(Key))
            return *Found;
        return nullptr;
    }

    static const TCHAR* SafeStr(const FString& S, const TCHAR* Fallback);

private:
    static TWeakObjectPtr<UDataTable> RankTable;
    static TWeakObjectPtr<UDataTable> MedalTable;

    static TMap<int32, const FRankInfo*> RankById;
    static TMap<int32, const FMedalInfo*> MedalById;
};
