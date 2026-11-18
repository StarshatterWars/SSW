#pragma once
#include "GameStructs.h"
#include "Mission.h"
#include "MissionElement.h"
#include "MissionLoad.h"
#include "ShipDesignRegistry.h"

namespace FreeFlightHullRules
{
    // Authored assignments retain their own fighter. Owned selection is for normal career entry.
    inline bool Apply(Mission& Model,const FS_PlayerGameInfo& Pilot)
    {
        if(!Model.IsFreeFlight() || !Model.GetPlayer())return false;
        const auto* Hull=Pilot.HangarShips.FindByPredicate([&](const FPlayerHangarShip& Row)
        {return Row.HullId==Pilot.SelectedHangarHull && Row.HullId.IsValid() && !Row.bAssigned && !Row.bLost;});
        const FShipDesign* Design=Hull?ShipDesignRegistry::Find(Hull->Design):nullptr;
        if(!Design)Design=ShipDesignRegistry::Find(FName(TEXT("Falcon")));
        if(!Design)return false;
        auto* Player=Model.GetPlayer();Player->SetShipDesign(Design);
        Player->GetLoadouts().destroy();
        if(Hull && !Hull->LoadoutName.IsEmpty() && Design==ShipDesignRegistry::Find(Hull->Design))
            Player->GetLoadouts().append(new MissionLoad(-1,TCHAR_TO_UTF8(*Hull->LoadoutName)));
        return true;
    }
}
