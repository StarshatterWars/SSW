#include "SSWNetworkShipControls.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "WeaponGroup.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "HardPoint.h"
#include "Sensor.h"
#include "SimRegion.h"

void SSWNetworkShipControls::Apply(Ship& Vessel,const FVector& Rotation,const FVector& Translation,float Throttle,bool Augmenter,bool Primary)
{
    if (Vessel.IsDead() || Vessel.IsDying()) return;
    if (!Vessel.IsAutoTrackEngaged())
    {
        Vessel.SetManualThrottle(Throttle); Vessel.SetManualAugmenter(Augmenter);
        for(int32 Axis=0;Axis<3;++Axis)
        { Vessel.SetManualRotationAxis(Axis,Rotation[Axis]); Vessel.SetManualTranslationAxis(Axis,Translation[Axis]); }
    }
    if (Primary) Vessel.FirePrimary();
}
void SSWNetworkShipControls::Clear(Ship& Vessel)
{
    Vessel.ClearManualRotationInput();
    Vessel.SetManualAugmenter(false);
    // Do not invoke FullStop: network loss must not teleport velocity to zero.
    if (!Vessel.IsAutoTrackEngaged() && !Vessel.IsAutoNavEngaged() && !Vessel.IsAutoLandEngaged()) Vessel.SetManualThrottle(0);
}

void SSWNetworkShipControls::Target(Ship& Vessel,FName Action)
{
    Ship* Fighter=&Vessel;
    if(Fighter->IsDead() || Fighter->IsDying()) return;

    if (Action == TEXT("SSW_TargetTracking"))

    {

        Fighter->SetAutoTrack(!Fighter->IsAutoTrackEngaged());

        return;

    }

    if (Action == TEXT("IA_DropWeaponTarget"))

    {

        Fighter->DropTarget();

        return;

    }

    if (Action == TEXT("IA_TargetNearest") || Action == TEXT("IA_TargetHostile"))

    {

        Fighter->LockTarget(ESimObject::SHIP, true, Action == TEXT("IA_TargetHostile"));

        return;

    }

    const bool bPrevious = Action == TEXT("IA_PreviousTarget");

    SimRegion* Region = Fighter->GetRegion();

    if (!Region || (!bPrevious && Action != TEXT("IA_NextTarget"))) return;

    TArray<Ship*> Candidates;

    int32 CurrentIndex = INDEX_NONE;

    for (ListIter<Ship> Ships = Region->GetShips(); ++Ships;)

    {

        Ship* Candidate = Ships.value();

        if (!Candidate || Candidate == Fighter || Candidate->IsDead() || Candidate->IsDying() ||

            Candidate->GetIFF() == Fighter->GetIFF() || Candidate->GetIFF() == 0) continue;

        if (Candidate == Fighter->GetTarget()) CurrentIndex = Candidates.Num();

        Candidates.Add(Candidate);

    }

    if (Candidates.IsEmpty()) return;

    const int32 Offset = bPrevious ? -1 : 1;

    const int32 NextIndex = CurrentIndex == INDEX_NONE ? (bPrevious ? Candidates.Num() - 1 : 0) :

        (CurrentIndex + Offset + Candidates.Num()) % Candidates.Num();

    Fighter->LockTarget(Candidates[NextIndex]);

}

void SSWNetworkShipControls::SensorCommand(Ship& Vessel,FName Action)
{
    Ship* Fighter=&Vessel;
    if(Fighter->IsDead() || Fighter->IsDying()) return;

    if (Action == TEXT("IA_EMCONUp")) Fighter->SetEMCON(Fighter->GetEMCON() + 1);

    else if (Action == TEXT("IA_EMCONDown")) Fighter->SetEMCON(Fighter->GetEMCON() - 1);

    else if (auto* Sensors = Fighter->GetSensor())

    {

        if (Action == TEXT("IA_SensorRangeUp")) Sensors->IncreaseRange();

        else if (Action == TEXT("IA_SensorRangeDown")) Sensors->DecreaseRange();

        else if (Action == TEXT("IA_SensorGroundMode"))

        {

            if (Fighter->IsAirborne() && Sensors->GetMode() < ESensorMode::PST) Fighter->SetSensorMode(ESensorMode::GM);

        }

        else if (Action == TEXT("IA_SensorMode"))

        {

            const auto Mode = Sensors->GetMode();

            if (Mode >= ESensorMode::PST) Fighter->DropTarget();

            else Fighter->SetSensorMode(Mode == ESensorMode::GM ? ESensorMode::PAS :

                static_cast<ESensorMode>(static_cast<int32>(Mode) + 1));

        }

    }

}

void SSWNetworkShipControls::CarriedStores(Ship& Vessel,TArray<FName>& Out)
{
    Out.Reset(); const auto* Design=Vessel.Design(); int* Loadout=Vessel.GetLoadout();
    if(!Design) return;
    for(int32 Index=0;Index<Design->hardpoints.size();++Index)
    {
        FName Type; auto* Station=Design->hardpoints[Index];
        const auto* Selected=Station && Loadout?Station->GetWeaponDesign(Loadout[Index]):nullptr;
        if(Selected && !Selected->primary)
            for(ListIter<WeaponGroup> Groups=Vessel.GetWeapons();++Groups;)
                for(ListIter<Weapon> Weapons=Groups->GetWeapons();++Weapons;)
                    if(auto* Weapon=Weapons.value();Weapon && Weapon->Design()==Selected && Weapon->Ammo()>0 &&
                        Weapon->GetMountRelativeLocation().Equals(Station->MountLocation(),.01))
                        Type=FName(UTF8_TO_TCHAR(Selected->name.data()));
        Out.Add(Type);
    }
}
