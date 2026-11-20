#pragma once
#include "CoreMinimal.h"

// Capabilities are authored SSW station roles, independent of price and simulation equations.
enum class ESSWStationService : uint8
{
    Cargo,Market,Insurance,Repair,Loadout,HullSelection,Refit,Manufacturing,Blueprints,SkillBooks,HullSales
};
class STARSHATTERWARS_API FStationServiceCatalog
{
public:
    static const FStationServiceCatalog& Get();
    bool Load(const FString& Path,FString& Error);
    bool LoadJson(const FString& Json,FString& Error);
    bool HasService(FName Design,FName Facility,ESSWStationService Service) const;
private:
    struct FEntry { FName Design,Facility;TSet<ESSWStationService> Services; };
    TArray<FEntry> Entries;
};
