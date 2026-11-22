#include "ShipStoreSubsystem.h"
#include "ShipSkinCatalog.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterSystemDesignSubsystem.h"
#include "WeaponDesignRegistry.h"
#include "StationServicesAI.h"
#include "OwnedHullLocation.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "StationServices.h"
#include "Sim.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/GameInstance.h"

bool UShipStoreSubsystem::RefreshCatalog(FText& Reason)
{
    TArray<FShipStoreOffer> Candidate;
    if(const auto* Systems=GetGameInstance()->GetSubsystem<UStarshatterSystemDesignSubsystem>())
        for(const auto& System:Systems->GetDesignsByName())for(const auto& Part:System.Value.Components)
            for(const auto& Grade:System.Value.ComponentGrades)
                if(const auto Variant=Systems->FindComponentGrade(System.Key,FName(*Part.Name),Grade.Level);Variant && Variant->BaseCost>0)
                {
                    FShipStoreOffer Offer;Offer.Category=EShipStoreCategory::Components;Offer.SystemDesign=System.Key;
                    Offer.Component=FName(*Part.Name);Offer.Grade=Grade.Level;Offer.Id=ShipStoreAI::ComponentId(System.Key,Offer.Component,Grade.Level);
                    Offer.Name=Part.Name;Offer.Group=System.Value.Name;Offer.Price=Variant->BaseCost;
                    Offer.Description=FString::Printf(TEXT("%s component. Quality %d / V. Performance %.1f%%; power demand %.1f%%. Purchase into local storage, then install at a shipyard using Hangar → Fit."),
                        *System.Value.Name,Grade.Level,Variant->PerformanceMultiplier*100,Variant->PowerDemandMultiplier*100);
                    Candidate.Add(MoveTemp(Offer));
                }
    FString Error;TArray<FShipSkinCatalogEntry> Skins;
    if(!ShipSkinCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/skins.json"),Skins,Error))
    {Reason=FText::FromString(Error);return false;}
    for(const auto& Skin:Skins)if(!Skin.bIncluded && ShipSkinCatalog::Resolve(Skin))
    {
        FShipStoreOffer Offer;Offer.Id=Skin.Id;Offer.Category=EShipStoreCategory::Skins;Offer.Design=Skin.Hull;
        Offer.Name=Skin.Name;Offer.Group=Skin.Hull.ToString();Offer.Price=Skin.PriceCredits;
        Offer.Description=Skin.Description+TEXT(" Permanent pilot license. Apply to a compatible owned hull through Player Logbook → Skins while docked.");
        Offer.Access=StaticEnum<EShipSkinCategory>()->GetDisplayNameTextByValue(int64(Skin.Category)).ToString();
        Candidate.Add(MoveTemp(Offer));
    }
    FString Json;TSharedPtr<FJsonObject> Root;const TArray<TSharedPtr<FJsonValue>>* Items=nullptr;double Version=0;
    if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Ships/store_equipment.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Root) || !Root ||
        !Root->TryGetNumberField(TEXT("schemaVersion"),Version) || Version!=1 || !Root->TryGetArrayField(TEXT("Items"),Items))
    {Reason=INVTEXT("Equipment store catalog is missing or invalid.");return false;}
    TSet<FName> Ids;for(const auto& Offer:Candidate)Ids.Add(Offer.Id);
    for(const auto& Value:*Items)
    {
        const auto Object=Value->AsObject();FString Id,Kind;double Price=0;FShipStoreOffer Offer;
        if(!Object || !Object->TryGetStringField(TEXT("Id"),Id) || Id.IsEmpty() ||
            !Object->TryGetStringField(TEXT("Category"),Kind) || (Kind!=TEXT("Missiles") && Kind!=TEXT("Drones")) ||
            !Object->TryGetStringField(TEXT("Name"),Offer.Name) || Offer.Name.IsEmpty() ||
            !Object->TryGetStringField(TEXT("Description"),Offer.Description) ||
            !Object->TryGetNumberField(TEXT("PriceCredits"),Price) || !FMath::IsFinite(Price) || Price<1 || Price>1.e9 || Price!=FMath::FloorToDouble(Price))
        {Reason=INVTEXT("Equipment catalog contains an invalid offer.");return false;}
        Offer.Id=FName(*Id);Offer.Category=Kind==TEXT("Missiles")?EShipStoreCategory::Missiles:EShipStoreCategory::Drones;
        FString Weapon;Object->TryGetStringField(TEXT("Weapon"),Weapon);Object->TryGetStringField(TEXT("Group"),Offer.Group);
        if(!Weapon.IsEmpty())
        {
            const auto* Design=WeaponDesignRegistry::Find(FName(*Weapon));
            if(!Design || Design->bSecret || Design->bPrimary || Design->bBeam)
            {Reason=INVTEXT("Equipment offer references an unavailable weapon.");return false;}
            Offer.Design=FName(*Weapon);
        }
        if(Offer.Id.IsNone() || Ids.Contains(Offer.Id)){Reason=INVTEXT("Duplicate equipment identity.");return false;}
        Ids.Add(Offer.Id);Offer.Price=int64(Price);Candidate.Add(MoveTemp(Offer));
    }
    Offers=MoveTemp(Candidate);Reason=FText::GetEmpty();return true;
}

FString UShipStoreSubsystem::Dock() const
{auto* S=Sim::GetSim();const auto* Craft=S?S->GetPlayerShip():nullptr;return Craft?OwnedHullLocation::Dock(*Craft):FString();}

int64 UShipStoreSubsystem::Balance() const
{const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();return P?P->GetPlayerInfo().Economy.Wallets.FindRef(FEconomyLedger::Pilot()):0;}

int32 UShipStoreSubsystem::Stored(FName Id) const
{const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();return P?ShipStoreAI::Stored(P->GetPlayerInfo(),Id,Dock()):0;}

bool UShipStoreSubsystem::CanPurchase(FName Id,int32 Quantity,FText& Reason) const
{
    const auto* Offer=Offers.FindByPredicate([Id](const auto& Row){return Row.Id==Id;});
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();const auto* S=Sim::GetSim();
    if(!Offer || !P || !P->HasLoaded()){Reason=INVTEXT("Select an available catalog item.");return false;}
    const auto Context=StationServicesAI::Context();FText MarketReason,HullReason;
    if(!StationServicesAI::CanUse(Context,ESSWStationService::Market,MarketReason) &&
        !StationServicesAI::CanUse(Context,ESSWStationService::HullSales,HullReason))
    {Reason=MarketReason;return false;}
    return ShipStoreAI::CanPurchase(P->GetPlayerInfo(),*Offer,Quantity,Dock(),S && S->HasSimulationAuthority() && !S->IsNetworked(),Reason);
}

bool UShipStoreSubsystem::Purchase(FName Id,int32 Quantity,int64 ExpectedTotal,FText& Reason)
{
    if(!CanPurchase(Id,Quantity,Reason))return false;
    const auto* Offer=Offers.FindByPredicate([Id](const auto& Row){return Row.Id==Id;});
    if(!Offer || ExpectedTotal!=Offer->Price*Quantity){Reason=INVTEXT("The quote changed. Review the new total.");return false;}
    auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    P->GetEconomy(); // Reconcile the legacy account before taking a transactional snapshot.
    return ShipStoreAI::Purchase(P->GetMutablePlayerInfo(),*Offer,Quantity,Dock(),true,FGuid::NewGuid(),
        [P](){P->MarkDirty();return P->SaveActivePlayerProgress();},Reason);
}

bool UShipStoreSubsystem::CanTransferMiningDrone(int32 Quantity,FText& Reason) const
{
    auto* S=Sim::GetSim();const auto* Craft=S?S->GetPlayerShip():nullptr;
    const auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!S || !S->HasSimulationAuthority() || S->IsNetworked() || !Craft || !Craft->Design() || !P || !P->HasLoaded() || !FStationServices::IsDocked() || Dock().IsEmpty())
    {Reason=INVTEXT("Dock with your own fighter or ship to transfer equipment.");return false;}
    if(!StationServicesAI::CanUse(StationServicesAI::Context(),ESSWStationService::Cargo,Reason))return false;
    if(!P->GetPlayerInfo().AssignedHangarHull.IsValid() || Craft->GetCargoInventoryKey()!=FName(*(TEXT("Ship/")+P->GetPlayerInfo().AssignedHangarHull.ToString(EGuidFormats::Digits))))
    {Reason=INVTEXT("Equipment transfer requires a persistent owned cargo hold.");return false;}
    if(Quantity<1 || Quantity>1000 || Stored(TEXT("Drone/MiningDrone"))<Quantity || Craft->GetCargoUsedTonnes()+Quantity*.5>Craft->Design()->CargoCapacity.Tonnes+UE_SMALL_NUMBER)
    {Reason=INVTEXT("Check stored quantity and free cargo space (0.5 t per mining drone).");return false;}
    Reason=INVTEXT("Move locally stored mining drones to cargo; no additional purchase fee.");return true;
}
bool UShipStoreSubsystem::TransferMiningDrone(int32 Quantity,FText& Reason)
{
    if(!CanTransferMiningDrone(Quantity,Reason))return false;
    auto* Craft=Sim::GetSim()->GetPlayerShip();auto* P=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    Craft->SyncCargoInventory();P->GetEconomy();
    const bool Saved=ShipStoreAI::TransferMiningDrone(P->GetMutablePlayerInfo(),Dock(),Craft->GetCargoInventoryKey(),Quantity,true,
        [P,Craft](){Craft->ApplyCargoInventoryToManifest();P->MarkDirty();return P->SaveActivePlayerProgress();},Reason);
    Craft->ApplyCargoInventoryToManifest();return Saved;
}
