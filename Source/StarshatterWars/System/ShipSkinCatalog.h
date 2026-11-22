#pragma once
#include "CoreMinimal.h"
#include "GameStructs.h"
#include "GameStructs_System.h"
#include "GameStructs_skins.h"
#include "AsteroidResource.h"

struct FShipSkinCatalogEntry
{
    FName Id;
    FName Hull;
    FString Name;
    FString SkinName;
    FString Description;
    bool bIncluded = false;
    EShipSkinCategory Category = EShipSkinCategory::Normal;
    ESSWAsteroidRarity Rarity = ESSWAsteroidRarity::Common;
    int64 PriceCredits = 0;
};

namespace ShipSkinCatalog
{
    bool Load(const FString& Path, TArray<FShipSkinCatalogEntry>& Out, FString& Error);
    bool IsOwned(const FS_PlayerGameInfo& Pilot, const FShipSkinCatalogEntry& Entry);
    bool IsPurchased(const FS_PlayerGameInfo& Pilot, const FShipSkinCatalogEntry& Entry);
    const FShipSkin* Resolve(const FShipSkinCatalogEntry& Entry);
    bool Purchase(FS_PlayerGameInfo& Pilot, const FShipSkinCatalogEntry& Entry,
        const FString& Dock, FText& Reason);
    bool Apply(FS_PlayerGameInfo& Pilot, FGuid HullId, const FShipSkinCatalogEntry* Entry,
        const FString& Dock, FText& Reason);
}
