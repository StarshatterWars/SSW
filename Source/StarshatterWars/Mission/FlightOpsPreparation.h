#pragma once
#include "Sim.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "FlightPlanner.h"
#include "ShipDesign.h"
#include "ShipLoad.h"
#include "CombatGroup.h"
#include "Instruction.h"
#include "SimElement.h"

// Re-resolve all simulation objects at command time; retain only names/slot indices in UI.
struct FFlightOpsPreparation
{
    static int MissionCode(int Type) {
        const EMissionType Codes[]={EMissionType::PATROL,EMissionType::INTERCEPT,EMissionType::ASSAULT,EMissionType::STRIKE,EMissionType::ESCORT,EMissionType::INTEL};
        return int(Codes[FMath::Clamp(Type,0,5)]);
    }
    static bool EligibleObjective(Ship* Carrier,SimElement* Element,int Type) {
        if(!Carrier || !Element || !Element->IsActive() || Element->IsFinished() || Element->IsSquadron())return false;
        const int IFF=Element->GetIFF();
        if(IFF!=Carrier->GetIFF()) {
            if(Element->GetIntelLevel()<EIntel::LOCATED)return false;
            if(auto* Group=Element->GetCombatGroup())if(Group->GetIntelLevel()<EIntel::LOCATED)return false;
        }
        Ship* Leader=Element->GetShip(1);
        switch(Type) {
        case 1:return IFF>0 && IFF!=Carrier->GetIFF() && Leader && Leader->IsDropship();
        case 2:return IFF>0 && IFF!=Carrier->GetIFF() && Leader && (Leader->IsStarship() || Leader->IsStatic());
        case 3:return IFF>0 && IFF!=Carrier->GetIFF() && Leader && Leader->IsGroundUnit();
        case 4:return (IFF==0 || IFF==Carrier->GetIFF()) && (!Leader || !Leader->IsStatic());
        default:return false;
        }
    }
    static TArray<FString> Objectives(Ship* Carrier,int Type) {
        TArray<FString> Result;
        if(auto* Simulation=Sim::GetSim())for(int I=0;I<Simulation->GetElements().size();++I) {
            auto* Element=Simulation->GetElements()[I];
            if(EligibleObjective(Carrier,Element,Type))Result.Add(ANSI_TO_TCHAR(Element->GetName().data()));
        }
        Result.Sort();return Result;
    }
    static TArray<FString> Loadouts(Ship* Carrier,int Squadron) {
        TArray<FString> Result;
        Hangar* H=Carrier?Carrier->GetHangar():nullptr;
        const ShipDesign* Design=H && Squadron>=0 && Squadron<H->NumSquadrons()?H->SquadronDesign(Squadron):nullptr;
        if(Design)for(int I=0;I<Design->loadouts.size();++I)if(auto* Load=Design->loadouts[I])Result.Add(ANSI_TO_TCHAR(Load->GetName()));
        return Result;
    }
    static bool Prepare(Ship* Carrier,int Squadron,const TArray<int32>& Slots,int Type,
        const FString& ObjectiveName,const FString& LoadName,bool Package,int& Pattern,FString& Status) {
        Hangar* H=Carrier?Carrier->GetHangar():nullptr;
        Sim* Simulation=Sim::GetSim();
        if(!H || !Simulation || Carrier->IsDead() || !Carrier->GetRegion() || Squadron<0 || Squadron>=H->NumSquadrons() || Slots.Num()<1 || Slots.Num()>4) {
            Status=TEXT("Select 1-4 stored fighters from one squadron.");return false;
        }
        const ShipDesign* Design=H->SquadronDesign(Squadron);
        if(!Design){Status=TEXT("Squadron design unavailable.");return false;}
        for(int Index:Slots)if(Index<0 || Index>=H->SquadronSize(Squadron) || H->GetState(H->GetSlot(Squadron,Index))!=Hangar::STORAGE) {
            Status=TEXT("Selected fighters are no longer in storage.");return false;
        }
        SimElement* Target=nullptr;
        if(Package && Type>=1 && Type<=4) {
            for(int I=0;I<Simulation->GetElements().size();++I) {
                auto* Candidate=Simulation->GetElements()[I];
                if(EligibleObjective(Carrier,Candidate,Type) && ObjectiveName==ANSI_TO_TCHAR(Candidate->GetName().data())){Target=Candidate;break;}
            }
            if(!Target){Status=TEXT("Choose an available objective for this mission type.");return false;}
        }
        int Load[16];for(int& Station:Load)Station=-1;
        const int* Source=H->GetLoadout(H->GetSlot(Squadron,Slots[0]));
        if(Package && !LoadName.IsEmpty()) {
            Source=nullptr;
            for(int I=0;I<Design->loadouts.size();++I)if(auto* Preset=Design->loadouts[I])
                if(LoadName==ANSI_TO_TCHAR(Preset->GetName())){Source=Preset->GetLoad();break;}
            if(!Source){Status=TEXT("Selected loadout is no longer available.");return false;}
        }
        if(Source)FMemory::Memcpy(Load,Source,sizeof(Load));
        FlightDeck* Deck=nullptr;int Queue=MAX_int32;
        for(int I=0;I<Carrier->NumFlightDecks();++I) {
            auto* Candidate=Carrier->GetFlightDeck(I);
            if(Candidate && Candidate->IsLaunchDeck() && H->PreflightQueue(Candidate)<Queue) {
                Queue=H->PreflightQueue(Candidate);Deck=Candidate;
            }
        }
        if(!Deck){Status=TEXT("No launch deck available.");return false;}
        const char* Callsign=Simulation->FindAvailCallsign(Carrier->GetIFF());
        if(!Callsign || !*Callsign){Status=TEXT("No callsign available.");return false;}
        SimElement* Element=Simulation->CreateElement(Callsign,Carrier->GetIFF(),Package?MissionCode(Type):0);
        if(!Element){Status=TEXT("Could not create flight element.");return false;}
        Element->SetSquadron(H->SquadronName(Squadron));Element->SetCarrier(Carrier);Element->SetLoadout(Load);
        if(Package) {
            if(Target) {
                const EInstruction Actions[]={EInstruction::Patrol,EInstruction::Intercept,EInstruction::Assault,EInstruction::Strike,EInstruction::Escort};
                Element->AddObjective(new Instruction(Actions[Type],Target->GetName().data()));
            }
            FlightPlanner Planner(Carrier);
            if(Type==4 && Target)Planner.CreateEscortRoute(Element,Target);
            else if(Type>=1 && Type<=3 && Target)Planner.CreateStrikeRoute(Element,Target);
            else {Planner.CreatePatrolRoute(Element,Pattern);Pattern=(Pattern+1)%4;}
        }
        int Prepared=0;
        for(int Index:Slots)if(H->GotoAlert(Squadron,Index,Deck,Element,Package?Load:nullptr,Package))++Prepared;
        if(!Prepared){Simulation->DestroyElement(Element);Status=TEXT("No fighters prepared; selection changed.");return false;}
        Status=FString::Printf(TEXT("%d fighter(s): %s. %s"),Prepared,ANSI_TO_TCHAR(Element->GetName().data()),
            Package?TEXT("Package launches automatically when ready."):TEXT("Alert holds for your LAUNCH command."));
        return true;
    }
};
