#include "MissionDebriefResult.h"
#include "Mission.h"
#include "SimElement.h"
#include "Instruction.h"
#include "SimEvent.h"
#include "Sim.h"
#include "Ship.h"

FMissionDebriefResult EvaluateMissionDebrief(const FMissionDebriefEvidence& E)
{
    if (E.bLost) return {false,TEXT("FAILED"),TEXT("Player ship lost or destroyed in a collision.")};
    if (E.bFailedObjective) return {false,TEXT("FAILED"),TEXT("A primary objective failed or was aborted.")};
    if (E.bBasicFlightTraining)
    {
        const bool Success = E.bDocked && E.RoutePoints == 5 && E.CompletedRoutePoints == E.RoutePoints;
        return {Success,Success ? TEXT("SUCCESS") : TEXT("INCOMPLETE"),
            FString::Printf(TEXT("Training route: %d/%d waypoints complete. Recovery: %s."),
                E.CompletedRoutePoints,E.RoutePoints,E.bDocked ? TEXT("docked safely") : TEXT("not docked"))};
    }
    if (E.RequiredObjectives > 0)
    {
        const bool Success = E.CompletedObjectives == E.RequiredObjectives;
        return {Success,Success ? TEXT("SUCCESS") : TEXT("INCOMPLETE"),
            FString::Printf(TEXT("Primary objectives: %d/%d complete."),E.CompletedObjectives,E.RequiredObjectives)};
    }
    return {false,TEXT("NOT ASSESSED"),TEXT("No measurable primary objectives are defined. Points and recovery do not establish success.")};
}

FMissionDebriefEvidence CollectMissionDebriefEvidence(const Mission* MissionPtr,SimElement* Element,ShipStats* Stats)
{
    FMissionDebriefEvidence E;
    E.bBasicFlightTraining = MissionPtr && MissionPtr->GetIdentity()==101
        && MissionPtr->GetMissionType()==int(EMissionType::TRAINING);
    if (Stats)
    {
        Stats->Summarize();
        E.bDocked=Stats->HasEvent(ESimEvent::DOCK);
        E.bLost=Stats->GetDeaths()>0 || Stats->GetColls()>0;
    }
    if (Element)
    {
        for (int32 I=0; I<Element->NumObjectives(); ++I)
        {
            Instruction* O=Element->GetObjective(I);
            if (!O || O->GetPriority()!=Instruction::PRIMARY) continue;
            ++E.RequiredObjectives;
            if (MissionPtr && MissionPtr->IsStoryMission() && O->GetAction() == EInstruction::Defend)
            {
                Sim* Simulation = Sim::GetSim();
                SimElement* Ward = Simulation ? Simulation->FindElement(O->GetTargetName()) : nullptr;
                bool bWardSafe = Ward && Ward->GetNumShips() > 0;
                if (Ward)
                    for (int32 Index = 1; Index <= Ward->GetNumShips(); ++Index)
                    {
                        const Ship* Protected = Ward->GetShip(Index);
                        bWardSafe &= Protected && !Protected->IsDead() && !Protected->IsDying() && Protected->GetIntegrity() >= 1;
                    }
                if (bWardSafe) ++E.CompletedObjectives;
                else E.bFailedObjective = true;
                continue;
            }
            if (O->GetStatus()==INSTRUCTION_STATUS::COMPLETE) ++E.CompletedObjectives;
            if (O->GetStatus()==INSTRUCTION_STATUS::FAILED || O->GetStatus()==INSTRUCTION_STATUS::ABORTED) E.bFailedObjective=true;
        }
        ListIter<Instruction> Route=Element->GetFlightPlan();
        while (++Route)
        {
            Instruction* N=Route.value();
            ++E.RoutePoints;
            if (N->GetStatus()==INSTRUCTION_STATUS::COMPLETE
                || (N->GetAction()==EInstruction::RTB && E.bDocked
                    && N->GetStatus()<=INSTRUCTION_STATUS::ACTIVE)) ++E.CompletedRoutePoints;
        }
    }
    return E;
}
