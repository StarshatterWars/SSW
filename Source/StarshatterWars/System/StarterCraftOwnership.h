#pragma once
#include "GameStructs.h"

namespace StarterCraftOwnership
{
    // A retained lost owned hull is also proof of issuance: loading must not
    // manufacture a replacement or bypass the existing loss/insurance rules.
    inline bool Normalize(FS_PlayerGameInfo& Pilot)
    {
        if(Pilot.Name.IsEmpty() && Pilot.Callsign.IsEmpty())return false;
        const FName Falcon(TEXT("Falcon"));
        int32 Keep=Pilot.HangarShips.IndexOfByPredicate([&](const auto& Hull)
            {return Hull.Design==Falcon && !Hull.bAssigned && Hull.HullId.IsValid() && !Hull.bLost;});
        if(Keep==INDEX_NONE)Keep=Pilot.HangarShips.IndexOfByPredicate([&](const auto& Hull)
            {return Hull.Design==Falcon && !Hull.bAssigned && Hull.HullId.IsValid();});
        if(Keep==INDEX_NONE)Keep=Pilot.HangarShips.IndexOfByPredicate([&](const auto& Hull)
            {return Hull.Design==Falcon && Hull.bAssigned && !Hull.bLost && Hull.HullId==Pilot.AssignedHangarHull;});
        if(Keep==INDEX_NONE)Keep=Pilot.HangarShips.IndexOfByPredicate([&](const auto& Hull)
            {return Hull.Design==Falcon && Hull.bAssigned && !Hull.bLost;});
        bool Changed=false;
        if(Keep==INDEX_NONE)
        {
            FPlayerHangarShip Hull;Hull.HullId=FGuid::NewGuid();Hull.Design=Falcon;Hull.StandardEquipmentLevel=1;
            Hull.Name=TEXT("Falcon");Hull.Location=TEXT("Carrier / Archon");
            Pilot.HangarShips.Add(Hull);Keep=Pilot.HangarShips.Num()-1;Changed=true;
        }
        auto& Starter=Pilot.HangarShips[Keep];
        if(!Starter.HullId.IsValid()){Starter.HullId=FGuid::NewGuid();Changed=true;}
        if(Starter.bAssigned){Starter.bAssigned=false;Starter.Name=TEXT("Falcon");Changed=true;}
        if(Starter.Name==TEXT("Starter Falcon")){Starter.Name=TEXT("Falcon");Changed=true;}
        const FGuid OwnedId=Starter.HullId;
        const bool Alive=!Starter.bLost;
        TSet<FGuid> Removed;
        Pilot.HangarShips.RemoveAll([&](const auto& Hull)
        {
            const bool Discard=Hull.Design==Falcon && Hull.bAssigned;
            if(Discard)Removed.Add(Hull.HullId);
            return Discard;
        });
        Changed|=!Removed.IsEmpty();
        if(Alive && (!Pilot.SelectedHangarHull.IsValid() || Removed.Contains(Pilot.SelectedHangarHull)))
        {Pilot.SelectedHangarHull=OwnedId;Changed=true;}
        if(Alive && (!Pilot.AssignedHangarHull.IsValid() || Removed.Contains(Pilot.AssignedHangarHull)))
        {Pilot.AssignedHangarHull=OwnedId;Changed=true;}
        return Changed;
    }
    inline bool VisibleInHangar(const FPlayerHangarShip& Hull,FGuid Active)
    {return !Hull.bLost && (!Hull.bAssigned || Hull.HullId==Active);}
}
