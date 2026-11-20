#include "StationServicesAI.h"
#include "StationServices.h"
#include "Sim.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "ShipInsuranceSubsystem.h"
#include "Engine/GameInstance.h"

bool StationServicesAI::FactionAllows(const Ship* Craft,const Ship* Base)
{
    return Craft && Base && !Craft->IsHostileTo(Base);
}
FStationDockContext StationServicesAI::Context(FName Viewed)
{
    FStationDockContext Result;auto* Simulation=Sim::GetSim();
    auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    const auto* Dock=Craft?Craft->GetCarrier():nullptr;
    const auto* Base=Viewed.IsNone()?Dock:(Simulation?Simulation->FindShip(TCHAR_TO_UTF8(*Viewed.ToString())):nullptr);
    if(FStationServices::IsDocked() && Dock)Result.ActualDock=FName(UTF8_TO_TCHAR(Dock->GetName()));
    // An explicit viewed station never silently switches to the current docking location.
    if(!Base || Base->IsDead() || Base->IsDying() || !Base->Design())
    {Result.Facility=Viewed;Result.Name=Viewed.IsNone()?TEXT(""):Viewed.ToString();return Result;}
    Result.bFactionAllowed=FactionAllows(Craft,Base);
    Result.Design=FName(UTF8_TO_TCHAR(Base->Design()->name));Result.Facility=FName(UTF8_TO_TCHAR(Base->GetName()));
    Result.Name=UTF8_TO_TCHAR(Base->GetName());Result.bDocked=FStationServices::IsDocked() && Base==Dock;
    if(auto* Region=Base->GetRegion())
    {
        Result.Region=UTF8_TO_TCHAR(Region->GetName());
        if(const auto* System=Region->GetSystem())Result.System=UTF8_TO_TCHAR(System->GetName());
    }
    return Result;
}
TConstArrayView<FStationServiceDescriptor> StationServicesAI::Descriptors()
{
    static const FStationServiceDescriptor Items[]={
        {ESSWStationService::HullSales,TEXT("SQUADRON AIRCRAFT SALES"),TEXT("UI.ShipStorePopupClass"),TEXT("Purchase eligible aircraft from the carrier OOB squadrons while docked. Assigned aircraft remain unavailable.")},
        {ESSWStationService::Cargo,TEXT("CARGO STORAGE"),TEXT("UI.CargoManifestPopupClass"),TEXT("View cargo and transfer unreserved items while docked.")},
        {ESSWStationService::Market,TEXT("MARKETPLACE"),TEXT("UI.MarketplacePopupClass"),TEXT("Browse and trade goods available at this location.")},
        {ESSWStationService::Insurance,TEXT("INSURANCE"),TEXT("UI.InsurancePopupClass"),TEXT("Review owned-hull coverage through Hangar.")},
        {ESSWStationService::Repair,TEXT("REPAIR"),TEXT("UI.HangarPopupClass"),TEXT("Request full docked repairs through Hangar; a quote is shown before purchase.")},
        {ESSWStationService::Loadout,TEXT("FIGHTER LOADOUT"),TEXT("UI.HangarPopupClass"),TEXT("Choose the assigned fighter in Hangar to modify its loadout.")},
        {ESSWStationService::HullSelection,TEXT("HANGAR"),TEXT("UI.HangarPopupClass"),TEXT("Review owned ships and select an eligible docked hull.")},
        {ESSWStationService::Refit,TEXT("SHIP REFITTING"),TEXT("UI.HangarPopupClass"),TEXT("Open Hangar to fit an eligible hull at this shipyard.")},
        {ESSWStationService::Manufacturing,TEXT("MANUFACTURING"),TEXT("UI.ProductionPopupClass"),TEXT("Review production and install jobs supported by this facility.")},
        {ESSWStationService::Blueprints,TEXT("BLUEPRINTS"),TEXT("UI.ProductionPopupClass"),TEXT("Open the blueprint page in Production Facilities.")},
        {ESSWStationService::SkillBooks,TEXT("SKILL BOOKS"),TEXT("UI.SkillsPopupClass"),TEXT("Review prerequisites and purchase available skill books.")}
    };
    return MakeArrayView(Items);
}
bool StationServicesAI::CanUse(const FStationDockContext& Context,ESSWStationService Service,FText& Reason)
{
    if(!Context.bFactionAllowed){Reason=INVTEXT("This facility denies services to hostile factions.");return false;}
    if(!Context.bDocked || Context.Facility.IsNone() || Context.ActualDock!=Context.Facility)
    {Reason=Context.Name.IsEmpty()?INVTEXT("Dock at the selected NPC station to use services."):
        FText::Format(INVTEXT("Dock at {0} to use this service."),FText::FromString(Context.Name));return false;}
    if(!FStationServiceCatalog::Get().HasService(Context.Design,Context.Facility,Service))
    {Reason=INVTEXT("This station does not provide this service.");return false;}
    Reason=INVTEXT("Available at the current dock. Individual rank, skill, ownership, stock and credit requirements still apply.");return true;
}

bool StationServicesAI::CanRepair(UGameInstance* Instance,FText& Reason,FName Viewed)
{
    if(!CanUse(Context(Viewed),ESSWStationService::Repair,Reason))return false;
    const auto* Wallet=Instance?Instance->GetSubsystem<UShipInsuranceSubsystem>():nullptr;
    const int64 Cost=FStationServices::FullRepairCost();
    if(!Wallet){Reason=INVTEXT("Pilot wallet unavailable.");return false;}
    if(Cost<=0){Reason=INVTEXT("The current fighter or ship needs no repairs.");return false;}
    Reason=FText::Format(INVTEXT("FULL REPAIR: {0} CR  |  BALANCE: {1} CR"),FText::AsNumber(Cost),FText::AsNumber(Wallet->GetBalance()));
    if(Wallet->GetBalance()<Cost){Reason=FText::Format(INVTEXT("INSUFFICIENT CREDITS - {0}"),Reason);return false;}
    return true;
}
bool StationServicesAI::ExecuteRepair(UGameInstance* Instance,FText& Reason,FName Viewed)
{
    return CanRepair(Instance,Reason,Viewed) && FStationServices::RepairAll(Instance,Reason);
}
