#include "ProductionCatalog.h"
#include "JsonRecordCatalog.h"
#include "Misc/Paths.h"
namespace
{
TMap<FName,FSSWProductionFacility> Sites;
TMap<FName,FSSWProductionRecipe> Batches;
bool Loaded=false;
void Ensure(){if(!Loaded){FString Error;if(!SSWProductionCatalog::Reload(Error))UE_LOG(LogTemp,Error,TEXT("[ProductionCatalog] %s"),*Error);}}
}
bool SSWProductionCatalog::Reload(FString& Error)
{
    TMap<FName,FSSWProductionFacility> NewSites;TMap<FName,FSSWProductionRecipe> NewBatches;
    if(!SSWJsonRecords::Load(FPaths::ProjectContentDir()/TEXT("GameData/Production/facilities.json"),NewSites,Error) || !SSWJsonRecords::Load(FPaths::ProjectContentDir()/TEXT("GameData/Cargo/recipes.json"),NewBatches,Error))return false;
    for(auto& Pair:NewBatches)
    {
        TSet<FName> Seen;
        for(auto& Input:Pair.Value.Inputs)
        {
            Input.Item=DataTableUtils::MakeValidName(Input.Item.ToString());
            if(Input.Item.IsNone() || Seen.Contains(Input.Item)){Error=TEXT("Invalid duplicate input: ")+Pair.Key.ToString();return false;}Seen.Add(Input.Item);
        }
    }
    for(const auto& P:NewSites)if(P.Value.Facility.IsEmpty() || P.Value.System.IsEmpty() || P.Value.Region.IsEmpty() || !FMath::IsFinite(P.Value.CapacityTonnes) || P.Value.CapacityTonnes<=0 || !NewBatches.Contains(P.Value.Recipe))
    {Error=TEXT("Invalid facility or missing recipe: ")+P.Key.ToString();return false;}
    Sites=MoveTemp(NewSites);Batches=MoveTemp(NewBatches);Loaded=true;
    UE_LOG(LogTemp,Display,TEXT("[ProductionCatalog] JSON-only facilities=%d recipes=%d"),Sites.Num(),Batches.Num());return true;
}
const TMap<FName,FSSWProductionFacility>& SSWProductionCatalog::Facilities(){Ensure();return Sites;}
const TMap<FName,FSSWProductionRecipe>& SSWProductionCatalog::Recipes(){Ensure();return Batches;}
