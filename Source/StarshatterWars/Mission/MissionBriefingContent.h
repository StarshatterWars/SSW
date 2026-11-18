#pragma once
#include "Mission.h"
#include "MissionElement.h"
#include "MissionLoad.h"
#include "Instruction.h"
#include "GameStructs_System.h"

// Shared by agent offers and the live Mission panel.
namespace MissionBriefingContent
{
    inline FString Build(const Mission& Model)
    {
        const auto Text = [](const char* Value) { return FString(UTF8_TO_TCHAR(Value ? Value : "")).TrimStartAndEnd(); };
        FString Body = FString::Printf(TEXT("LOCATION\n%s / %s"), *Text(Model.GetSystem()), *Text(Model.GetRegion()));
        const auto Section = [&Body, &Text](const TCHAR* Caption, const char* Value)
        {
            const FString Content = Text(Value);
            if (!Content.IsEmpty()) Body += FString::Printf(TEXT("\n\n%s\n%s"), Caption, *Content);
        };
        // Legacy getter is not const-qualified; this display path only reads it.
        if(auto* Player=const_cast<Mission&>(Model).GetPlayer())
        {
            if(const auto* Design=Player->GetShipDesign())
                Body+=FString::Printf(TEXT("\n\nASSIGNED SHIP\n%s"),*(Design->DisplayName.IsEmpty()?Design->ShipName:Design->DisplayName));
            TArray<FString> Loads;
            for(ListIter<MissionLoad> Load=Player->GetLoadouts();++Load;)
            {
                const FString Name=Text(Load->GetName().data());
                if(!Name.IsEmpty())Loads.AddUnique(Name);
            }
            if(!Loads.IsEmpty())Body+=TEXT("\nLOADOUT: ")+FString::Join(Loads,TEXT(" / "));
            else Body+=TEXT("\nLOADOUT: Mission assigned");
            int32 Index=0;
            for(ListIter<Instruction> Point=Player->NavList();++Point;)
            {
                if(Index==0)Body+=TEXT("\n\nFLIGHT PLAN");
                const FString Sector=Text(Point->GetRegionName());
                const FString Target=Text(Point->GetTargetName());
                Body+=FString::Printf(TEXT("\nNAV %02d  %s%s"),++Index,*Sector,Target.IsEmpty()?TEXT(""):*(TEXT(" | ")+Target));
            }
        }
        Section(TEXT("BRIEFING"), Model.GetDescription());
        Section(TEXT("OBJECTIVES"), Model.GetObjective());
        Section(TEXT("SITUATION"), Model.GetSituation());
        return Body;
    }
}
