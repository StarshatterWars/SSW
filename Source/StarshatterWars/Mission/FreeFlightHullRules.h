#pragma once
#include "GameStructs.h"
#include "ShipCommerceAI.h"
#include "OwnedHullLocation.h"
#include "PlayerCharacter.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionLoad.h"
#include "ShipDesignRegistry.h"
#include "ShipSkinCatalog.h"
#include "Misc/Paths.h"
#include "ShipDesign.h"

namespace FreeFlightHullRules
{
    // Authored assignments retain their own fighter. Owned selection is for normal career entry.
    inline bool Apply(Mission& Model,const FS_PlayerGameInfo& Pilot)
    {
        if(!Model.IsFreeFlight() || !Model.GetPlayer())return false;
        const auto* Hull=Pilot.HangarShips.FindByPredicate([&](const FPlayerHangarShip& Row)
        {return Row.HullId==Pilot.SelectedHangarHull && Row.HullId.IsValid() && !Row.bAssigned && (!Row.bSold && !Row.bLost && (Row.HullId==Pilot.AssignedHangarHull || ShipCommerceAI::RentalMayLaunch(Pilot,Row,FDateTime::UtcNow().ToUnixTimestamp())));});
        if (!Hull || !OwnedHullLocation::CanProvision(*Hull, Model, Hull->HullId == Pilot.AssignedHangarHull))
            Hull = Pilot.HangarShips.FindByPredicate([&](const auto& Row)
                { return Row.HullId == Pilot.AssignedHangarHull && !Row.bAssigned && (!Row.bSold && !Row.bLost && (Row.HullId==Pilot.AssignedHangarHull || ShipCommerceAI::RentalMayLaunch(Pilot,Row,FDateTime::UtcNow().ToUnixTimestamp()))) && OwnedHullLocation::CanProvision(Row, Model, true); });
        if(!Hull && Pilot.HangarShips.ContainsByPredicate([&](const auto& Row){return Row.HullId==Pilot.SelectedHangarHull && (Row.bSold || Row.RentalId.IsValid());}))return false;
        const FShipDesign* Design=Hull?ShipDesignRegistry::Find(Hull->Design):nullptr;
        if(Design && Pilot.Rank<PlayerCharacter::HullRankRequired(Design->ShipType,Design->ShipName))Design=nullptr;
        if(!Design)Design=ShipDesignRegistry::Find(FName(TEXT("Falcon")));
        if(!Design)return false;
        auto* Player=Model.GetPlayer();Player->SetShipDesign(Design);
        Player->SetSkin(nullptr);
        if(Hull && Design==ShipDesignRegistry::Find(Hull->Design) && !Hull->SelectedSkin.IsNone())
        {
            TArray<FShipSkinCatalogEntry> Skins;FString Error;
            if(ShipSkinCatalog::Load(FPaths::ProjectContentDir()/TEXT("GameData/Ships/skins.json"),Skins,Error))
                if(const auto* Entry=Skins.FindByPredicate([&](const auto& Skin){return Skin.Id==Hull->SelectedSkin && Skin.Hull==Hull->Design;}))
                    if(ShipSkinCatalog::IsOwned(Pilot,*Entry) && ShipSkinCatalog::Resolve(*Entry))
                        if(const auto* Legacy=ShipDesignRegistry::FindLegacy(Hull->Design))Player->SetSkin(Legacy->FindSkin(TCHAR_TO_UTF8(*Entry->SkinName)));
        }
        Player->GetLoadouts().destroy();
        if(Hull && !Hull->LoadoutName.IsEmpty() && Design==ShipDesignRegistry::Find(Hull->Design))
            Player->GetLoadouts().append(new MissionLoad(-1,TCHAR_TO_UTF8(*Hull->LoadoutName)));
        return true;
    }
}
