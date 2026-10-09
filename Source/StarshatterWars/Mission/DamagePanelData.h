#pragma once
#include "CoreMinimal.h"
#include "Ship.h"
#include "Power.h"
#include "Drive.h"
#include "QuantumDrive.h"
#include "Shield.h"
#include "Thruster.h"
#include "Computer.h"
#include "NavSystem.h"
#include "FlightDeck.h"
#include "Sensor.h"
#include "Weapon.h"
#include "WeaponGroup.h"

// Snapshot-only adapter for the legacy twelve-cell warning panel.
// Can be reused by a registered panel; retains no simulation pointers.
namespace SSWDamagePanel
{
    struct FCell {
        FString Label;
        int32 Status=-1; // absent equipment leaves its fixed cell empty
    };
    struct FStatus {
        bool Present=false, Maintenance=false;
        SYSTEM_STATUS Worst=SYSTEM_STATUS::NOMINAL;
        void Add(SimSystem* S) {
            if(!S)return;
            Present=true;
            const auto Value=S->GetStatus();
            if(Value==SYSTEM_STATUS::DESTROYED || Value==SYSTEM_STATUS::CRITICAL || Value==SYSTEM_STATUS::DEGRADED) {
                if(int32(Value)<int32(Worst))Worst=Value;
            }
            else if(Value==SYSTEM_STATUS::MAINT || Value==SYSTEM_STATUS::REPAIR || Value==SYSTEM_STATUS::REPLACE) Maintenance=true;
        }
        int32 Value() const {
            return !Present ? -1 : int32(Worst==SYSTEM_STATUS::NOMINAL && Maintenance ? SYSTEM_STATUS::MAINT : Worst);
        }
    };
    template<class T> void AddList(FStatus& Status,List<T>& Items) {
        for(int32 I=0;I<Items.size();++I)Status.Add(Items[I]);
    }
    inline FString WeaponName(WeaponGroup* Group,int32 Index) {
        FString Name=Group->Name()?FString(ANSI_TO_TCHAR(Group->Name())).TrimStartAndEnd().ToUpper():FString();
        if(Name.IsEmpty() || Name.Contains(TEXT("PRI WEP")) || Name.Contains(TEXT("WEP PRI")) ||
           Name.Contains(TEXT("SEC WEP")) || Name.Contains(TEXT("WEP SEC")) || Name==TEXT("PRIMARY") || Name==TEXT("SECONDARY")) {
            if(auto* Design=Group->GetDesign())Name=FString(ANSI_TO_TCHAR(Design->name.data())).ToUpper();
        }
        if(Name.IsEmpty())Name=FString::Printf(TEXT("WEAPON %d"),Index+1);
        return Name;
    }
    inline void Build(Ship* ShipNow,TArray<FCell>& Cells) {
        Cells.Reset(); if(!ShipNow)return;
        Cells.SetNum(12);
        const TCHAR* Labels[]={TEXT("REACTOR"),TEXT("DRIVE"),TEXT("QUANTUM"),TEXT("SHIELD"),TEXT(""),TEXT(""),TEXT(""),TEXT(""),TEXT("SENSOR"),TEXT("COMPUTER"),TEXT("THRUSTER"),TEXT("FLT DECK")};
        for(int32 I=0;I<12;++I)Cells[I].Label=Labels[I];
        FStatus States[12];
        AddList(States[0],ShipNow->GetReactors());
        AddList(States[1],ShipNow->GetDrives());
        States[2].Add(ShipNow->GetQuantumDrive());
        States[3].Add(ShipNow->GetShield()); States[3].Add(ShipNow->GetDecoy());
        if(!ShipNow->GetShield() && ShipNow->GetDecoy())Cells[3].Label=TEXT("DECOY");
        auto& Groups=ShipNow->GetWeapons();
        for(int32 I=0;I<4 && I<Groups.size();++I)if(auto* Group=Groups[I]) {
            Cells[I+4].Label=WeaponName(Group,I);
            AddList(States[I+4],Group->GetWeapons());
        }
        // Distinguish genuinely separate groups with identical display names.
        for(int32 I=0;I<4 && I<Groups.size();++I) {
            const FString Name=Cells[I+4].Label; if(Name.IsEmpty())continue;
            int32 Matches=0;for(int32 J=0;J<4 && J<Groups.size();++J)
                if(Groups[J] && WeaponName(Groups[J],J)==Name)++Matches;
            if(Matches>1)Cells[I+4].Label+=FString::Printf(TEXT(" %d"),I+1);
        }
        States[8].Add(ShipNow->GetSensor()); States[8].Add(ShipNow->GetProbeLauncher());
        AddList(States[9],ShipNow->GetComputers()); States[9].Add(ShipNow->GetNavSystem());
        States[10].Add(ShipNow->GetThruster());
        AddList(States[11],ShipNow->GetFlightDecks());
        for(int32 I=0;I<12;++I)Cells[I].Status=States[I].Value();
    }
    inline FLinearColor Color(int32 Status,double Time,const FLinearColor& Nominal) {
        if(Status<0)return FLinearColor::Transparent;
        if(Status!=int32(SYSTEM_STATUS::NOMINAL) && FMath::Fmod(Time,0.5)<0.25)
            return FLinearColor(0.03f,0.03f,0.03f,1);
        switch(SYSTEM_STATUS(Status)) {
        case SYSTEM_STATUS::DESTROYED:return FLinearColor(0.12f,0.12f,0.12f,1);
        case SYSTEM_STATUS::CRITICAL:return FLinearColor::Red;
        case SYSTEM_STATUS::DEGRADED:return FLinearColor::Yellow;
        default:return Nominal;
        }
    }
}
