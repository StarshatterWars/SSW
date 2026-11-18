#include "PlayerHullLoss.h"

bool FPlayerHullLoss::Finalize(FS_PlayerGameInfo& Pilot,FGuid Hull)
{
    if(!Hull.IsValid() || Pilot.AssignedHangarHull!=Hull)return false;
    auto* Lost=Pilot.HangarShips.FindByPredicate([Hull](const auto& Entry){return Entry.HullId==Hull;});
    if(!Lost || Lost->bLost)return false;
    const FName Hold(*(TEXT("Ship/")+Hull.ToString(EGuidFormats::Digits)));
    auto& State=Pilot.Economy;
    if(!State.Inventories.Contains(Hold))
        FEconomyLedger::CreateInventory(State,Hold,FEconomyLedger::Pilot(),NAME_None,0);
    if(auto* Inventory=State.Inventories.Find(Hold);Inventory && Inventory->Owner==FEconomyLedger::Pilot())
    {
        // An empty zero-capacity tombstone prevents a dying runtime ship from
        // reimporting its old manifest before the simulation removes its actor.
        Inventory->Items.Reset();Inventory->ManifestItems.Reset();
        Inventory->CapacityTonnes=0;Inventory->ReservedOutputTonnes=0;
    }
    for(auto It=State.Reservations.CreateIterator();It;++It)
        if(It.Value().Inventory==Hold)It.RemoveCurrent();
    for(auto It=State.MiningLeases.CreateIterator();It;++It)
        if(It.Value().Claimant==Hold)It.RemoveCurrent();
    const FString OpticsPrefix=Hold.ToString()+TEXT("/Optics/");
    State.MiningDrones.Remove(Hold);
    for(auto It=State.MiningOptics.CreateIterator();It;++It)
        if(It.Key().ToString().StartsWith(OpticsPrefix))It.RemoveCurrent();
    Lost->bLost=true;Lost->Insurance.bSettled=true;
    Lost->bHasCondition=true;Lost->HullCondition=0;
    if(Pilot.SelectedHangarHull==Hull)Pilot.SelectedHangarHull.Invalidate();
    if(Pilot.AssignedHangarHull==Hull)Pilot.AssignedHangarHull.Invalidate();
    return true;
}
