#include "OwnedHullCondition.h"
#include "ShipEquipment.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "WeaponDesign.h"
#include "OwnedHullLocation.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "SimSystem.h"
#include "SimComponent.h"
#include "Sim.h"
#include "Mission.h"
#include "StarshatterPlayerSubsystem.h"
#include "Engine/GameInstance.h"

bool FOwnedHullCondition::IsOwnedLive(const FPlayerHangarShip& Row,FGuid Assigned,FName Design,FName Cargo,bool FreeFlight)
{
    return FreeFlight && !Row.bAssigned && !Row.bLost && Row.HullId.IsValid() && Row.HullId==Assigned && Row.Design==Design &&
        Cargo==FName(*(TEXT("Ship/")+Row.HullId.ToString(EGuidFormats::Digits)));
}
bool FOwnedHullCondition::CaptureActive(FS_PlayerGameInfo& Player)
{
    auto* Simulation=Sim::GetSim(); auto* Craft=Simulation?Simulation->GetPlayerShip():nullptr;
    if(!Craft || !Craft->Design() || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked() || !Simulation->GetMission())return false;
    if(auto* Current=Player.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==Player.AssignedHangarHull && !Row.bLost && Row.Design==FName(UTF8_TO_TCHAR(Craft->Design()->name));}))CaptureAmmo(*Craft,*Current);
    if(!Simulation->GetMission()->IsFreeFlight())return false;
    auto* Owned=Player.HangarShips.FindByPredicate([&](const auto& Row){return IsOwnedLive(Row,Player.AssignedHangarHull,
        FName(UTF8_TO_TCHAR(Craft->Design()->name)),Craft->GetCargoInventoryKey(),true);});
    if(!Owned)return false;
    Capture(*Craft,*Owned);
    const FString Place = OwnedHullLocation::Current(*Craft);
    if (!Place.IsEmpty()) Owned->Location = Place;
    return true;
}

void FOwnedHullCondition::CaptureAmmo(Ship& Craft,FPlayerHangarShip& Row)
{
    Row.SavedWeaponAmmo.Reset();Row.SavedWeaponTypes.Reset();
    for(ListIter<WeaponGroup> Groups=Craft.GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(Weapons->Design() && Weapons->Ammo()>=0)
            {Row.SavedWeaponAmmo.Add(Weapons->GetIndex(),Weapons->Ammo());Row.SavedWeaponTypes.Add(Weapons->GetIndex(),FName(UTF8_TO_TCHAR(Weapons->Design()->name.data())));}
}
void FOwnedHullCondition::ApplyAmmo(Ship& Craft,const FPlayerHangarShip& Row)
{
    for(ListIter<WeaponGroup> Groups=Craft.GetWeapons();++Groups;)
        for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
            if(Weapons->Design())if(const auto* Count=Row.SavedWeaponAmmo.Find(Weapons->GetIndex()))
                if(*Count>=0 && *Count<=1000000 && Row.SavedWeaponTypes.FindRef(Weapons->GetIndex())==FName(UTF8_TO_TCHAR(Weapons->Design()->name.data())))Weapons->SetAmmo(*Count);
}
void FOwnedHullCondition::Capture(Ship& Craft,FPlayerHangarShip& Row)
{
    CaptureAmmo(Craft,Row);
    Row.bHasCondition=true;
    Row.HullCondition=Craft.Design() && Craft.Design()->integrity>0 ? FMath::Clamp(static_cast<float>(Craft.GetIntegrity()/Craft.Design()->integrity),0.f,1.f) : 1.f;
    Row.SystemConditions.Reset(); Row.RepairQueue.Reset();
    auto& Systems=Craft.GetSystems();
    for(int32 Index=0;Index<Systems.size();++Index)
    {
        auto* System=Systems[Index]; if(!System)continue;
        auto& State=Row.SystemConditions.AddDefaulted_GetRef();
        State.SystemIndex=Index; State.SystemName=FName(UTF8_TO_TCHAR(System->GetName()));
        State.Availability=System->availability; State.Safety=System->safety; State.Stability=System->stability;
        State.SafetyOverload=System->safety_overload; State.NetAvailability=System->net_avail; State.Status=static_cast<int32>(System->Status);
        auto& Parts=System->GetComponents();
        for(int32 PartIndex=0;PartIndex<Parts.size();++PartIndex)
        {
            auto* Part=Parts[PartIndex]; if(!Part)continue;
            auto* Slot=Row.Components.FindByPredicate([&](const auto& Item){return Item.SystemIndex==Index && Item.ComponentIndex==PartIndex;});
            if(!Slot)
            {
                Slot=&Row.Components.AddDefaulted_GetRef(); Slot->SystemIndex=Index; Slot->ComponentIndex=PartIndex;
                Slot->Component=FName(UTF8_TO_TCHAR(Part->Name())); Slot->SystemName=UTF8_TO_TCHAR(System->GetName());
            }
            Slot->bHasCondition=true; Slot->ConditionPercent=Part->availability; Slot->ConditionStatus=static_cast<int32>(Part->Status);
            Slot->MaintenanceSeconds=Part->time_remaining; Slot->SpareCount=Part->spares; Slot->JerryCount=Part->jerried;
        }
    }
    if(auto* Equipment=Craft.GetEquipment())Equipment->Capture(Row);
    auto& Queue=Craft.RepairQueue();
    for(int32 Index=0;Index<Queue.size();++Index)
        for(int32 SystemIndex=0;SystemIndex<Systems.size();++SystemIndex)
            if(Queue[Index]==Systems[SystemIndex]) {Row.RepairQueue.Add(SystemIndex);break;}
}
void FOwnedHullCondition::ApplyComponent(SimComponent& Part,const FHangarComponent& Stored)
{
    if(Stored.ItemId.IsValid())Part.ItemId=Stored.ItemId;Part.InstalledLevel=FMath::Clamp(Stored.Level,1,5);
    if(!Stored.bHasCondition)return;
    Part.availability=FMath::Clamp(FMath::IsFinite(Stored.ConditionPercent)?Stored.ConditionPercent:100.f,0.f,100.f);
    Part.Status=static_cast<SYSTEM_STATUS>(FMath::Clamp(Stored.ConditionStatus,static_cast<int32>(SYSTEM_STATUS::DESTROYED),static_cast<int32>(SYSTEM_STATUS::UNKNOWN)));
    Part.time_remaining=FMath::Max(0.f,FMath::IsFinite(Stored.MaintenanceSeconds)?Stored.MaintenanceSeconds:0.f);
    Part.spares=FMath::Max(0,Stored.SpareCount);Part.jerried=FMath::Max(0,Stored.JerryCount);
}

void FOwnedHullCondition::Apply(Ship& Craft,const FPlayerHangarShip& Row)
{
    ApplyAmmo(Craft,Row);
    if(auto* Equipment=Craft.GetEquipment())Equipment->Apply(Row);
    if(!Row.bHasCondition || !Craft.Design())return;
    Craft.SetIntegrity(FMath::Clamp(FMath::IsFinite(Row.HullCondition)?Row.HullCondition:1.f,0.f,1.f)*Craft.Design()->integrity);
    auto& Systems=Craft.GetSystems();
    for(const auto& Slot:Row.Components)
    {
        if(!Slot.bHasCondition || Slot.SystemIndex<0 || Slot.SystemIndex>=Systems.size())continue;
        auto* System=Systems[Slot.SystemIndex]; if(!System)continue;
        auto& Parts=System->GetComponents();
        if(Slot.ComponentIndex<0 || Slot.ComponentIndex>=Parts.size())continue;
        auto* Part=Parts[Slot.ComponentIndex];
        if(!Part)continue;
        static const TCHAR* Grades[]={TEXT("I"),TEXT("II"),TEXT("III"),TEXT("IV"),TEXT("V")};
        const FName NativeName(UTF8_TO_TCHAR(Part->Name()));
        const FName GradedName(*(Slot.Component.ToString()+TEXT(" ")+Grades[FMath::Clamp(Slot.Level,1,5)-1]));
        if(NativeName!=Slot.Component && NativeName!=GradedName)continue;
        Part->availability=FMath::Clamp(FMath::IsFinite(Slot.ConditionPercent)?Slot.ConditionPercent:100.f,0.f,100.f);
        Part->Status=static_cast<SYSTEM_STATUS>(FMath::Clamp(Slot.ConditionStatus,static_cast<int32>(SYSTEM_STATUS::DESTROYED),static_cast<int32>(SYSTEM_STATUS::UNKNOWN)));
        Part->time_remaining=FMath::Max(0.f,FMath::IsFinite(Slot.MaintenanceSeconds)?Slot.MaintenanceSeconds:0.f);
        Part->spares=Slot.SpareCount; Part->jerried=FMath::Max(0,Slot.JerryCount);
    }
    for(int32 Index=0;Index<Systems.size();++Index)if(Systems[Index])Systems[Index]->CalcStatus();
    for(const auto& State:Row.SystemConditions)
    {
        if(State.SystemIndex<0 || State.SystemIndex>=Systems.size())continue;
        auto* System=Systems[State.SystemIndex];
        if(!System || State.SystemName!=FName(UTF8_TO_TCHAR(System->GetName())))continue;
        System->availability=FMath::Clamp(FMath::IsFinite(State.Availability)?State.Availability:1.f,0.f,1.f);
        System->safety=FMath::Clamp(FMath::IsFinite(State.Safety)?State.Safety:1.f,0.f,1.f);
        System->stability=FMath::Clamp(FMath::IsFinite(State.Stability)?State.Stability:1.f,0.f,1.f);
        System->safety_overload=FMath::Max(0.f,FMath::IsFinite(State.SafetyOverload)?State.SafetyOverload:0.f);
        System->net_avail=FMath::Clamp(FMath::IsFinite(State.NetAvailability)?State.NetAvailability:-1.f,-1.f,1.f);
        System->Status=static_cast<SYSTEM_STATUS>(FMath::Clamp(State.Status,static_cast<int32>(SYSTEM_STATUS::DESTROYED),static_cast<int32>(SYSTEM_STATUS::UNKNOWN)));
    }
    Craft.RepairQueue().clear();
    for(int32 Index:Row.RepairQueue)if(Index>=0 && Index<Systems.size() && Systems[Index] && !Craft.RepairQueue().contains(Systems[Index]))Craft.RepairQueue().append(Systems[Index]);
}
void FOwnedHullCondition::ApplyPristine(Ship& Craft)
{
    if(Craft.Design())Craft.SetIntegrity(Craft.Design()->integrity);
    for(int32 Index=0;Index<Craft.GetSystems().size();++Index)if(Craft.GetSystems()[Index])Craft.GetSystems()[Index]->RestoreAtStation();
    Craft.RepairQueue().clear();
}
bool FOwnedHullCondition::RepairTransaction(Ship& Craft,FS_PlayerGameInfo& Player,FPlayerHangarShip* Owned,
    TFunctionRef<bool()> Pay,TFunctionRef<bool()> Repair,TFunctionRef<bool()> Persist)
{
    const auto BeforePlayer=Player; FPlayerHangarShip BeforeLive; Capture(Craft,BeforeLive);
    const FGuid OwnedId=Owned?Owned->HullId:FGuid();
    if(Pay() && Repair())
    {
        if(OwnedId.IsValid())
            if(auto* Current=Player.HangarShips.FindByPredicate([&](const auto& Row){return Row.HullId==OwnedId;}))Capture(Craft,*Current);
        if(Persist())return true;
    }
    Player=BeforePlayer; Apply(Craft,BeforeLive); return false;
}
bool FOwnedHullCondition::RepairAndSave(UGameInstance* Instance,Ship& Craft,int64 Cost,FText& Reason)
{
    auto* Simulation=Sim::GetSim(); auto* Players=Instance?Instance->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
    if(!Players || !Simulation || !Simulation->HasSimulationAuthority() || Simulation->IsNetworked() || Cost<=0)return false;
    auto& Player=Players->GetMutablePlayerInfo();
    auto* Owned=Player.HangarShips.FindByPredicate([&](const auto& Row){return IsOwnedLive(Row,Player.AssignedHangarHull,
        Craft.Design()?FName(UTF8_TO_TCHAR(Craft.Design()->name)):NAME_None,Craft.GetCargoInventoryKey(),Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight());});
    const bool OK=RepairTransaction(Craft,Player,Owned,
        [&](){return FEconomyLedger::Pay(Players->GetEconomy(),FEconomyLedger::Pilot(),TEXT("Station repair service"),Cost,FGuid::NewGuid(),TEXT("Full ship repairs"),ECommercePurpose::Service);},
        [&](){return Craft.RepairAtStation();},[&](){return Players->SaveActivePlayerProgress();});
    Reason=OK?INVTEXT("Hull and all installed systems fully repaired and saved. Grades and loadout retained."):
        INVTEXT("Repair could not be saved. Ship condition and credits were restored. Please retry.");
    return OK;
}
