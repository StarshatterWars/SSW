#include "ProductionCatalog.h"
#include "JsonRecordCatalog.h"
#include "SimDataEvents.h"
#include "Misc/Paths.h"
namespace
{
TMap<FName,FSSWProductionFacility> ProductionSites;
TMap<FName,FSSWProductionRecipe> ProductionRecipes;
TMap<FName,FSSWProductionFacility> GroundProfiles;
TSet<FName> PublishedGround;
bool bProductionCatalogLoaded=false;
void EnsureProductionCatalog(){if(!bProductionCatalogLoaded){FString Error;if(!SSWProductionCatalog::Reload(Error))UE_LOG(LogTemp,Error,TEXT("[ProductionCatalog] %s"),*Error);}}
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
    TMap<FName,FSSWProductionFacility> NewProfiles;
    if(!SSWJsonRecords::Load(FPaths::ProjectContentDir()/TEXT("GameData/Production/factories.json"),NewProfiles,Error))return false;
    for(const auto& Pair:NewProfiles)
        if(!Pair.Value.Recipe.IsNone() && (!NewBatches.Contains(Pair.Value.Recipe) || !FMath::IsFinite(Pair.Value.CapacityTonnes) || Pair.Value.CapacityTonnes<=0))
        {Error=TEXT("Invalid ground building production profile: ")+Pair.Key.ToString();return false;}
    GroundProfiles=MoveTemp(NewProfiles);PublishedGround.Reset();
    ProductionSites=MoveTemp(NewSites);ProductionRecipes=MoveTemp(NewBatches);bProductionCatalogLoaded=true;
    UE_LOG(LogTemp,Display,TEXT("[ProductionCatalog] JSON-only facilities=%d recipes=%d"),ProductionSites.Num(),ProductionRecipes.Num());return true;
}
const TMap<FName,FSSWProductionFacility>& SSWProductionCatalog::Facilities(){EnsureProductionCatalog();return ProductionSites;}
const TMap<FName,FSSWProductionRecipe>& SSWProductionCatalog::Recipes(){EnsureProductionCatalog();return ProductionRecipes;}

const TMap<FName,FSSWProductionFacility>& SSWProductionCatalog::BuildingProfiles(){EnsureProductionCatalog();return GroundProfiles;}
FName SSWProductionCatalog::GroundKey(const FGroundFactorySite& Site)
{return FName(*(TEXT("Ground/")+Site.System+TEXT("/")+Site.Region+TEXT("/")+Site.Facility));}
bool SSWProductionCatalog::ResolveGround(const FGroundFactorySite& Site,FSSWProductionFacility& Out)
{
    const auto* Profile=BuildingProfiles().Find(Site.BuildingDesign);
    if(!Profile || (Profile->Recipe.IsNone() && Profile->FacilityType!=ESSWProductionFacilityType::Storage) || Site.System.IsEmpty() || Site.Region.IsEmpty() || Site.Facility.IsEmpty())return false;
    Out=*Profile;Out.Facility=Site.Facility+TEXT(" (")+Site.Region+TEXT(")");Out.System=Site.System;Out.Region=Site.Region;Out.Planet=Site.Planet;Out.Empire=Site.Empire;Out.bSurface=true;
    return true;
}
bool SSWProductionCatalog::RegisterGround(FEconomyState& State,const FGroundFactorySite& Site)
{
    FSSWProductionFacility Resolved;if(!ResolveGround(Site,Resolved))return false;
    const FName Id=GroundKey(Site);
    // Existing identity and destruction state survive mission reloads; a new scene is not a repair.
    if(const auto* Existing=State.GroundFactories.Find(Id))return Existing->BuildingDesign==Site.BuildingDesign;
    State.GroundFactories.Add(Id,Site);ProductionSites.Add(Id,MoveTemp(Resolved));PublishedGround.Add(Id);FSimDataEvents::Publish(State);return true;
}
void SSWProductionCatalog::RestoreGround(const FEconomyState& State)
{
    for(FName Id:PublishedGround)ProductionSites.Remove(Id);
    PublishedGround.Reset();
    for(const auto& Pair:State.GroundFactories)
    {
        FSSWProductionFacility Site;
        if(Pair.Key==GroundKey(Pair.Value) && ResolveGround(Pair.Value,Site))
        {ProductionSites.Add(Pair.Key,MoveTemp(Site));PublishedGround.Add(Pair.Key);}
    }
}
