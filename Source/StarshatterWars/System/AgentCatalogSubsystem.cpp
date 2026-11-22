
#include "AgentCatalogSubsystem.h"
#include "Ship.h"
#include "ShipDesignRegistry.h"
#include "SimElement.h"
#include "SimRegion.h"
#include "LocalGameStateSubsystem.h"
#include "TrainingShuttleRules.h"
#include "StationServices.h"
#include "CampaignFleetTransit.h"
#include "StarSystem.h"
#include "FreeFlightAssignment.h"
#include "JsonObjectConverter.h"
#include "FactionTrainingCatalog.h"
#include "DynamicMissionAI.h"
#include "AgentJournalTransaction.h"
#include "Sim.h"
#include "MissionArcRules.h"
#include "JsonRecordCatalog.h"
#include "TrainingTestRules.h"
#include "AgentMissionRules.h"
#include "CampaignPresentationRules.h"
#include "SkillsSubsystem.h"
#include "Sim.h"
#include "TimerSubsystem.h"

#include "Campaign.h"
#include "CampaignSave.h"
#include "CampaignStoryRules.h"
#include "Mission.h"
#include "MissionInfo.h"
#include "CombatGroup.h"
#include "CombatAction.h"
#include "MissionRankRules.h"
#include "PlayerCharacter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SSWGameInstance.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterPlayerSubsystem.h"

void UAgentCatalogSubsystem::LoadCatalog()
{
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Agents/agents.json");
    TMap<FName, FS_AgentCatalogRow> Imported;
    FString Error;
    if (!SSWJsonRecords::Load(Path, Imported, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("%s"), *Error);
        return;
    }

    for (const auto& Pair : Imported)
    {
        const auto& Agent = Pair.Value;
        if (Agent.DisplayName.IsEmpty() || Agent.System.IsEmpty() || Agent.Planet.IsEmpty() ||
            Agent.Region.IsEmpty() || Agent.Station.IsEmpty() || Agent.Division.IsEmpty() ||
            Agent.MinimumRank < 0 || !Agent.ServesEmpire(Agent.Empire) || Agent.Level < 1 || Agent.Level > 5)
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid agent definition: %s"), *Pair.Key.ToString());
            return;
        }
    }
    Catalog = MoveTemp(Imported);
    UE_LOG(LogTemp, Display, TEXT("Imported %d agents from %s"), Catalog.Num(), *Path);
}

bool UAgentCatalogSubsystem::PrepareIndependentOffer(FName Id,FText& Reason)
{
 auto* Game=Cast<USSWGameInstance>(GetGameInstance());auto* Players=Game?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;const auto* Agent=FindAgent(Id);
 if(!Game || Game->IsTrainingTestSession() || !Players || !Agent || !CanContact(Id) || Agent->Class==EAgentClass::Training)return false;
 FDynamicAgentContext Context;Context.Id=Id;Context.Name=Agent->DisplayName;Context.System=Agent->System;Context.Region=Agent->Region;Context.Station=Agent->Station;Context.Pools=Agent->DynamicPools;Context.Level=Agent->Level;Context.Empire=Agent->Empire;
 return DynamicMissionAI::Generate(Players->GetMutablePlayerInfo(),Context,FDateTime::UtcNow().ToUnixTimestamp(),Reason);
}
bool UAgentCatalogSubsystem::StartIndependentAssignment(int32 Id,FText& Reason)
{
 auto* Game=Cast<USSWGameInstance>(GetGameInstance());auto* Players=Game?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;auto* Simulation=Sim::GetSim();auto* Pilot=PlayerCharacter::GetCurrentPlayer();
 if(!Game || !Players || !Simulation || !Simulation->GetMission() || !Simulation->GetMission()->IsFreeFlight() || !Pilot)return false;
 if(Id==FreeFlightAssignment::Id)
 {
     const auto* Order=Game->GetTrainingAgent().Find(AgentAI::PilotKey(*Pilot));
     if(!Order || Order->MissionId!=Id || !CanAccept(Id,Order->Issuer) || !FStationServices::IsDocked())return false;
     const auto Started=AgentJournalTransaction::Commit(Game->GetTrainingAgent(),Players->GetMutablePlayerInfo(),
         [&](){return Game->GetTrainingAgent().Start(AgentAI::PilotKey(*Pilot),Id);},[&](){return Players->SaveActivePlayerProgress();});
     Reason=Started==AgentJournalTransaction::EResult::Committed?INVTEXT("Free Flight clearance granted. Use Launch when your craft is ready."):INVTEXT("Unable to save departure clearance. Retry the assignment.");
     return Started==AgentJournalTransaction::EResult::Committed;
 }
 const auto Result=AgentJournalTransaction::Commit(Game->GetTrainingAgent(),Players->GetMutablePlayerInfo(),[&](){return DynamicMissionAI::Start(*Simulation,Players->GetMutablePlayerInfo(),Game->GetTrainingAgent(),AgentAI::PilotKey(*Pilot),Id,Reason);},[&](){return Players->SaveActivePlayerProgress();});
 if(Result==AgentJournalTransaction::EResult::SaveFailed)Reason=INVTEXT("Unable to save job start; the accepted assignment was retained.");
 return Result==AgentJournalTransaction::EResult::Committed;
}

const FS_AgentCatalogRow* UAgentCatalogSubsystem::FindAgent(FName Id) const
{
    return Catalog.Find(Id);
}

bool UAgentCatalogSubsystem::CanContact(FName Id) const
{
    const auto* Agent = FindAgent(Id);
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    const auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    if (Game && Game->IsTrainingTestSession() && Id != TEXT("JanekFlightTraining")) return false;
    return Agent && Pilot && Agent->ServesEmpire(Pilot->GetEmpire());
}

TArray<FName> UAgentCatalogSubsystem::GetDirectorySystems(FName Origin, const TSet<FName>& Neighbors, EAgentDirectoryScope Scope) const
{
    TSet<FName> Unique;
    for (const auto& Pair : Catalog)
    {
        const FName System(*Pair.Value.System);
        if (CanContact(Pair.Key) && AgentDirectoryRules::IncludesSystem(Origin, System, Neighbors, Scope)) Unique.Add(System);
    }
    auto Systems = Unique.Array();
    Systems.Sort([](FName A, FName B) { return A.LexicalLess(B); });
    return Systems;
}

TArray<FName> UAgentCatalogSubsystem::GetSystemAgents(FName System) const
{
    TArray<FName> Agents;
    if (System.IsNone()) return Agents;
    for (const auto& Pair : Catalog)
        if (FName(*Pair.Value.System) == System && CanContact(Pair.Key)) Agents.Add(Pair.Key);
    Agents.Sort([this](FName A, FName B)
    {
        const auto& Left = Catalog.FindChecked(A);
        const auto& Right = Catalog.FindChecked(B);
        if (Left.Level != Right.Level) return Left.Level < Right.Level;
        const int32 NameOrder = Left.DisplayName.Compare(Right.DisplayName, ESearchCase::IgnoreCase);
        return NameOrder != 0 ? NameOrder < 0 : A.LexicalLess(B);
    });
    return Agents;
}

TArray<int32> UAgentCatalogSubsystem::GetEligibleMissions(FName AgentId) const
{
    TArray<int32> Eligible;
    const auto* Agent = FindAgent(AgentId);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    if (!Agent || !Game || !CanContact(AgentId) || !MissionRankRules::Unlocked(Agent->MinimumRank)) return Eligible;

    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    if (Agent->MissionIds.Contains(FreeFlightAssignment::Id) && Pilot && Players && FreeFlightAssignment::Unlocked(Pilot->GetRank(),Players->GetPlayerInfo()))
        Eligible.Add(FreeFlightAssignment::Id);
    if (!Pilot) return Eligible;
    const FName Key = AgentAI::PilotKey(*Pilot);
    const auto* Skills = Game->GetSubsystem<USkillsSubsystem>();
    if (Agent->Class == EAgentClass::Training)
    {
        auto* Training = Game->GetAgentTrainingCampaign();
        if (!Training || !Game->GetTrainingAgent().GetQueue(Key).IsEmpty()) return Eligible;
        for (int32 Id : Agent->MissionIds)
        {
            const bool bDone = Game->GetTrainingAgent().IsCompleted(Key, Id) ||
                (!Game->IsTrainingTestSession() && Players && Players->GetPlayerInfo().HasCompletedMission(1, Id));
            if (bDone) continue;
            const FName Skill = FTrainingTestRules::RequiredSkill(Id);
            if (FTrainingTestRules::Unlocked(Id, Pilot->GetRank(), Skill.IsNone() || (Skills && Skills->HasSkill(Skill)), Game->IsTrainingTestSession()))
                Eligible.Add(Id);
            return Eligible; // A rank/skill gate never skips a chapter.
        }
        return Eligible;
    }

    if(Players && !Game->IsTrainingTestSession())
        for(int32 Id:DynamicMissionAI::Offers(Players->GetPlayerInfo(),AgentId,FDateTime::UtcNow().ToUnixTimestamp()))Eligible.AddUnique(Id);
    // Authored campaign assignments remain owned by their original campaign.
    if (!Game->IsTrainingTestSession())
        if (auto* Active = Campaign::GetCampaign(); Active && Active->IsActive() && !NeedsCampaignDeployment(Active->GetCampaignId()) && IsCampaignIssuer(AgentId, Active->GetCampaignId()))
        {
            TArray<MissionInfo*> Missions;
            for (int32 Index=0; Index<Active->GetMissionList().size(); ++Index) Missions.Add(Active->GetMissionList()[Index]);
            for (int32 Index=0; Index<Active->GetStoryMissionList().size(); ++Index) Missions.Add(Active->GetStoryMissionList()[Index]);
            for (int32 Index = 0; Index < Missions.Num(); ++Index)
            {
                const auto* Info = Missions[Index];
                if (!Info || Active->IsCutsceneMission(Info->id) || !IsStoryUnlocked(*Active, Info->id)) continue;
                const auto* Action = Info->action_id ? Active->FindAction(Info->action_id) : nullptr;
                const auto* Group = Active->GetPlayerGroup();
                if (!CanOfferCampaignMission(*Agent, Active->GetCampaignId(), *Info, Active->GetTime(), Pilot->GetRank(),
                    Group ? FString(UTF8_TO_TCHAR(Group->GetRegion().data())) : FString(),
                    Players && Players->GetPlayerInfo().HasCompletedMission(Active->GetCampaignId(), Info->id),
                    !Info->action_id || (Action && Action->Status() == Info->action_status))) continue;
                const int32 Id = AgentMissionRules::Encode(Active->GetCampaignId(), Info->id);
                if (Id != INDEX_NONE) Eligible.AddUnique(Id);
                if (Active->IsSequential()) break;
            }
        }

    const auto LocalPool=[&](int32 Id)
    {
        for(const auto& Pair:Catalog)if(Pair.Value.Empire==Agent->Empire && Pair.Value.System.Equals(Agent->System,ESearchCase::IgnoreCase) && Pair.Value.MissionIds.Contains(Id))return true;
        return false;
    };
    if (const auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>())
        for (const auto& Scenario : Data->GetSingleMissionScenarios())
        {
            if (Scenario.Id <= 0 || Scenario.Id >= 1000000 - Mission::RegionalAgentMissionOffset || Scenario.StoryCampaignId > 0 ||
                !(Agent->MissionIds.Contains(Scenario.Id) || (Agent->MissionTypes.Contains(Scenario.MissionType) && LocalPool(Scenario.Id))) ||
                !AgentMissionRules::FactionAllows(Pilot->GetEmpire(),Scenario) ||
                !AgentMissionRules::LocalSystem(Agent->System, Scenario.MissionSystem) ||
                !MissionRankRules::Unlocked(MissionRankRules::Required(Scenario))) continue;

            const int32 Id = Mission::RegionalAgentMissionOffset + Scenario.Id;
            const auto Arc=FMissionArcCatalog::Get().Evaluate(Id,[](int32){return false;});
            if(Arc.Arc && !Arc.Arc->Steps[Arc.Selected].AgentId.IsNone() && Arc.Arc->Steps[Arc.Selected].AgentId!=AgentId)continue;
            if (!Game->GetTrainingAgent().ArcRequirement(Key, Id,
                [Players](int32 PreviousId) { return Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, PreviousId); }).IsEmpty()) continue;
            if (!Players || !Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, Id))
                Eligible.AddUnique(Id);
        }
    return Eligible;
}

bool UAgentCatalogSubsystem::CanReplayTrainingTest(int32 MissionId,FName AgentId) const
{
    auto* Game=Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    const auto* Agent=FindAgent(AgentId);
    if(!Game || !Pilot || !Agent || Agent->Class!=EAgentClass::Training ||
        !Agent->MissionIds.Contains(MissionId) || !FTrainingTestRules::IsLesson(MissionId) || !CanContact(AgentId))return false;
    const FName Key=AgentAI::PilotKey(*Pilot);
    const FName Skill=FTrainingTestRules::RequiredSkill(MissionId);
    const auto* Skills=Game->GetSubsystem<USkillsSubsystem>();
    const auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const bool Completed=Game->GetTrainingAgent().IsCompleted(Key,MissionId) ||
        (Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::TRAINING_CAMPAIGN,MissionId));
    const auto Queue=Game->GetTrainingAgent().GetQueue(Key);
    const bool QueueAvailable=Queue.IsEmpty() || (FStationServices::IsDocked() &&
        !Queue.ContainsByPredicate([](const AgentAI::FAssignment& Item){return Item.MissionId<101 || Item.MissionId>107 || Item.bTestReplay;}));
    return Pilot->GetRank()>=FTrainingTestRules::RequiredRank(MissionId) && (Skill.IsNone() || (Skills && Skills->HasSkill(Skill))) && Completed && QueueAvailable;
}

bool UAgentCatalogSubsystem::CanAccept(int32 MissionId, FName AgentId) const
{
    if (!CanContact(AgentId)) return false;
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    const auto* Agent = FindAgent(AgentId);
    if (!Game || !Pilot || !Agent || Pilot->GetRank() < Agent->MinimumRank) return false;
    if(MissionId==FreeFlightAssignment::Id)
    {
        const auto* Progress=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        const auto Queue=Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot));
        if(Queue.ContainsByPredicate([](const AgentAI::FAssignment& Assignment){return Assignment.MissionId!=FreeFlightAssignment::Id;}))return false;
        return Progress && Agent->MissionIds.Contains(MissionId) && FreeFlightAssignment::Unlocked(Pilot->GetRank(),Progress->GetPlayerInfo()) && FStationServices::IsDocked();
    }
    if(DynamicMissionAI::IsDynamic(MissionId))
    {
        const auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>();FText Reason;
        if(!Players)return false;bool Accepted=false;
        for(const auto& Item:Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot)))if(Item.MissionId==MissionId && Item.Issuer==AgentId && (Item.State==AgentAI::EState::Accepted || Item.State==AgentAI::EState::InProgress))Accepted=true;
        return DynamicMissionAI::Offers(Players->GetPlayerInfo(),AgentId,Accepted?0:FDateTime::UtcNow().ToUnixTimestamp()).Contains(MissionId) && DynamicMissionAI::CanUse(Players->GetPlayerInfo(),MissionId,Reason);
    }
    if (const auto* Scenario=FindScenario(MissionId))
    {
        if(!AgentMissionRules::FactionAllows(Pilot->GetEmpire(),*Scenario))return false;
        const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        if (!Game->GetTrainingAgent().ArcRequirement(AgentAI::PilotKey(*Pilot), MissionId,
            [Players](int32 Id) { return Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, Id); }).IsEmpty()) return false;
    }
    if (FTrainingTestRules::IsLesson(MissionId))
    {
        const FName Skill = FTrainingTestRules::RequiredSkill(MissionId);
        const auto* Skills = Game->GetSubsystem<USkillsSubsystem>();
        if (!Agent->MissionIds.Contains(MissionId) ||
            !FTrainingTestRules::Unlocked(MissionId, Pilot->GetRank(), Skill.IsNone() || (Skills && Skills->HasSkill(Skill)), Game->IsTrainingTestSession())) return false;
    }
    const auto Queue = Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot));
    if (!Queue.IsEmpty() && Queue[0].MissionId == MissionId && Queue[0].Issuer == AgentId && Queue[0].State == AgentAI::EState::Accepted)
    {
        if (FTrainingTestRules::IsLesson(MissionId)) return true;
        int32 CampaignId, OriginalId;
        if (AgentMissionRules::Decode(MissionId, CampaignId, OriginalId))
        {
            auto* Active = Campaign::GetCampaign();
            if (!Active || Active->GetCampaignId() != CampaignId || !Active->IsActive() || !IsCampaignIssuer(AgentId, CampaignId)) return false;
            const auto* Info = Active->GetMissionInfo(OriginalId);
            if (!Info || Active->IsCutsceneMission(OriginalId) || !IsStoryUnlocked(*Active, OriginalId)) return false;
            const auto* Action = Info->action_id ? Active->FindAction(Info->action_id) : nullptr;
            const auto* Group = Active->GetPlayerGroup();
            const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
            return CanOfferCampaignMission(*Agent, CampaignId, *Info, Active->GetTime(), Pilot->GetRank(),
                Group ? FString(UTF8_TO_TCHAR(Group->GetRegion().data())) : FString(),
                Players && Players->GetPlayerInfo().HasCompletedMission(CampaignId, OriginalId),
                !Info->action_id || (Action && Action->Status() == Info->action_status));
        }
    }
    return GetEligibleMissions(AgentId).Contains(MissionId);
}

const FS_CampaignMission* UAgentCatalogSubsystem::FindScenario(int32 Id) const
{
    if (Id < Mission::RegionalAgentMissionOffset || Id >= AgentMissionRules::CampaignBase) return nullptr;
    const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    return Data ? Data->FindSingleMissionScenario(Id - Mission::RegionalAgentMissionOffset) : nullptr;
}

FText UAgentCatalogSubsystem::GetMissionTitle(int32 MissionId) const
{
    if(MissionId==FreeFlightAssignment::Id)return INVTEXT("Free Flight — Departure Clearance");
    if(DynamicMissionAI::IsDynamic(MissionId))if(auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>())return DynamicMissionAI::Title(Players->GetPlayerInfo(),MissionId);

    int32 CampaignId, OriginalId;
    if (AgentMissionRules::Decode(MissionId, CampaignId, OriginalId))
        if (auto* Active = Campaign::GetCampaign(); Active && Active->GetCampaignId() == CampaignId)
            if (const auto* Info = Active->GetMissionInfo(OriginalId)) return FText::FromString(UTF8_TO_TCHAR(Info->name.data()));
    if (const auto* Scenario = FindScenario(MissionId)) return FText::FromString(Scenario->MissionName);
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()))
        if (auto* Training = Game->GetAgentTrainingCampaign())
            if (const auto* Info = Training->GetMissionInfo(MissionId)) return FText::FromString(UTF8_TO_TCHAR(Info->name.data()));
    return FText::GetEmpty();
}

Campaign* UAgentCatalogSubsystem::GetRepository(int32 MissionId)
{
    int32 CampaignId, OriginalId;
    if (AgentMissionRules::Decode(MissionId, CampaignId, OriginalId))
    {
        auto* Active = Campaign::GetCampaign();
        return Active && Active->GetCampaignId() == CampaignId ? Active : nullptr;
    }
    if (MissionId >= AgentMissionRules::CampaignBase) return nullptr;
    if (MissionId >= Mission::RegionalAgentMissionOffset)
    {
        if (!RegionalRepository)
            RegionalRepository = MakeShared<Campaign>(Campaign::SINGLE_MISSIONS, "Regional Assignments", true);
        return RegionalRepository.Get();
    }
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance())) return Game->GetAgentTrainingCampaign();
    return nullptr;
}

Mission* UAgentCatalogSubsystem::GetMission(int32 MissionId)
{
    if(MissionId==FreeFlightAssignment::Id)
    {
        if(auto* Existing=RegionalMissions.Find(MissionId))return Existing->Get();
        FS_CampaignMission Row;FString Json;
        if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("GameData/Agents/free_flight.json"))) || !FJsonObjectConverter::JsonObjectStringToUStruct(Json,&Row,0,0))return nullptr;
        auto Model=MakeShared<Mission>(MissionId);
        Model->SetName(TCHAR_TO_UTF8(*Row.MissionName));
        Model->SetSituation(TCHAR_TO_UTF8(*Row.Sitrep));
        Model->SetObjective(TCHAR_TO_UTF8(*Row.Objective));
        return RegionalMissions.Add(MissionId,MoveTemp(Model)).Get();
    }
    if(DynamicMissionAI::IsDynamic(MissionId))
    {
        auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();FS_CampaignMission Row;
        if(!Players || !DynamicMissionAI::Preview(Players->GetPlayerInfo(),MissionId,Row))return nullptr;
        if(auto* Existing=RegionalMissions.Find(MissionId)){(*Existing)->SetObjective(TCHAR_TO_UTF8(*Row.Objective));(*Existing)->SetSituation(TCHAR_TO_UTF8(*Row.Sitrep));return Existing->Get();}
        auto Model=MakeShared<Mission>(MissionId);if(!Model->LoadFromCampaignMissionData(Row))return nullptr;Model->SetIdentity(MissionId);return RegionalMissions.Add(MissionId,MoveTemp(Model)).Get();
    }

    int32 CampaignId, OriginalId;
    if (AgentMissionRules::Decode(MissionId, CampaignId, OriginalId))
    {
        auto* Repository = GetRepository(MissionId);
        return Repository ? Repository->GetMission(OriginalId) : nullptr;
    }
    if (MissionId < Mission::RegionalAgentMissionOffset)
    {
        auto* Training = GetRepository(MissionId);
        return Training ? Training->GetMission(MissionId) : nullptr;
    }

    if (const auto* Scenario = FindScenario(MissionId))
    {
        if (auto* Existing = RegionalMissions.Find(MissionId)) return Existing->Get();
        auto Model = MakeShared<Mission>(MissionId);
        if (!Model->LoadFromCampaignMissionData(*Scenario)) return nullptr;
        Model->SetIdentity(MissionId);
        return RegionalMissions.Add(MissionId, MoveTemp(Model)).Get();
    }
    return nullptr;
}

FText UAgentCatalogSubsystem::GetAssignmentStatus(FName AgentId) const
{
    const auto* Agent = FindAgent(AgentId);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    if (!Agent || !Game || !Pilot) return FText::GetEmpty();
    if (Agent->Class != EAgentClass::Training)
    {
        TArray<FString> Status;
        Status.Add(TEXT("Request local work or review your campaign orders."));
        const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
        for (int32 OriginalId : Agent->MissionIds)
        {
            const FText Arc = Game->GetTrainingAgent().ArcStatus(AgentAI::PilotKey(*Pilot), Mission::RegionalAgentMissionOffset + OriginalId,
                [Players](int32 Id) { return Players && Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, Id); });
            if (!Arc.IsEmpty()) Status.AddUnique(Arc.ToString());
        }
        return FText::FromString(FString::Join(Status,TEXT("\n\n")));
    }
    return FText::Format(INVTEXT("{0} | Complete each lesson in sequence. Advanced command lessons require the appropriate rank and hull-command skill."),FText::FromString(FactionTrainingCatalog::Title(Agent->Empire)));
}

FText UAgentCatalogSubsystem::GetTrainingRequirement(int32 Id) const
{
    if(Id==FreeFlightAssignment::Id)
    {
        auto* Context=Cast<USSWGameInstance>(GetGameInstance());
        const auto* Character=PlayerCharacter::GetCurrentPlayer();
        if(!Character || Character->GetRank()<2)return INVTEXT("Requires Ensign rank or higher.");
        const auto* Progress=Context?Context->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
        if(!Progress || !FreeFlightAssignment::Unlocked(Character->GetRank(),Progress->GetPlayerInfo()))
            return INVTEXT("Complete T1–T4 before requesting independent flight.");
        const auto Queue=Context->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Character));
        if(Queue.ContainsByPredicate([](const AgentAI::FAssignment& Assignment){return Assignment.MissionId!=FreeFlightAssignment::Id;}))
            return INVTEXT("Complete or abandon your current assignment before requesting Free Flight.");
        if(!FStationServices::IsDocked())return INVTEXT("Dock at a friendly base to request departure clearance.");
        return INVTEXT("Accept and start this clearance, then use Launch. Fly your active craft in the live universe. Repeatable; no mission rewards.");
    }
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    if (!Game || !Pilot || !FTrainingTestRules::IsLesson(Id)) return INVTEXT("Unavailable");
    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Skills = Game->GetSubsystem<USkillsSubsystem>();
    const FName Key = AgentAI::PilotKey(*Pilot);
    const auto Completed = [&](int32 Lesson)
    {
        return Game->GetTrainingAgent().IsCompleted(Key, Lesson) ||
            (!Game->IsTrainingTestSession() && Players && Players->GetPlayerInfo().HasCompletedMission(1, Lesson));
    };
    if (Completed(Id)) return INVTEXT("COMPLETE");
    TArray<FString> Reasons;
    if (Id > 101 && !Completed(Id-1)) Reasons.Add(FString::Printf(TEXT("Complete T%d first"), Id-101));
    if (Pilot->GetRank() < FTrainingTestRules::RequiredRank(Id))
        Reasons.Add(MissionRankRules::Requirement(FTrainingTestRules::RequiredRank(Id)));
    const FName Skill = FTrainingTestRules::RequiredSkill(Id);
    if (!Skill.IsNone() && (!Skills || !Skills->HasSkill(Skill)))
    {
        const auto* Definition = Skills ? Skills->GetCatalog().Find(Skill) : nullptr;
        Reasons.Add(TEXT("Requires ") + (Definition ? Definition->DisplayName : Skill.ToString().Replace(TEXT("_"), TEXT(" "))));
    }
    if (!Reasons.IsEmpty()) return FText::FromString(FString::Join(Reasons, TEXT(" | ")));
    for (const auto& Assignment : Game->GetTrainingAgent().GetQueue(Key))
        if (Assignment.MissionId == Id) return Assignment.State == AgentAI::EState::InProgress ? INVTEXT("IN PROGRESS") : INVTEXT("ACCEPTED - START FROM QUEUE");
    if (!Game->GetTrainingAgent().GetQueue(Key).IsEmpty()) return INVTEXT("Finish the current assignment first");
    return INVTEXT("AVAILABLE");
}

FText UAgentCatalogSubsystem::GetCampaignTitle(int32 Id) const
{
    const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row = Data ? Data->GetCampaignByIndex1Based(Id) : nullptr;
    return Row ? FText::FromString(Row->Name) : FText::GetEmpty();
}

FText UAgentCatalogSubsystem::GetCampaignOrders(FName AgentId) const
{
    const auto* Agent = FindAgent(AgentId);
    const auto* Active = Campaign::GetCampaign();
    if (!Agent || !Active || !IsCampaignIssuer(AgentId, Active->GetCampaignId())) return FText::GetEmpty();
    return FText::FromString(FString::Printf(TEXT("%s\n\n%s\n\n%s"), UTF8_TO_TCHAR(Active->GetName()),
        UTF8_TO_TCHAR(Active->Situation()), UTF8_TO_TCHAR(Active->Orders())));
}

bool UAgentCatalogSubsystem::IsStoryUnlocked(const Campaign& Active, int32 MissionId) const
{
    const auto* Row=Active.FindStoryMissionData(MissionId);
    if (!Row) return true; // Existing generated/story mission gates stay unchanged.
    const auto* Game=Cast<USSWGameInstance>(GetGameInstance());
    const auto* Save=Game ? Game->CampaignSave : nullptr;
    return CampaignStoryRules::IsUnlocked(*Row,Active.GetCampaignId(),Save &&
        Save->CampaignIndex==Active.GetCampaignId() && !Save->CompletedMissionIds.Contains(MissionId) && Save->HasPlayedScene(Row->StoryUnlockScene));
}

bool UAgentCatalogSubsystem::IsCampaignIssuer(FName AgentId, int32 CampaignId) const
{
    const auto* Agent = FindAgent(AgentId);
    const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row = Data ? Data->GetCampaignByIndex1Based(CampaignId) : nullptr;
    return Agent && Row && Agent->CampaignIds.Contains(CampaignId) && IsCampaignOrigin(*Agent, *Row);
}

bool UAgentCatalogSubsystem::IsCampaignOrigin(const FS_AgentCatalogRow& Agent, const FS_Campaign& Row)
{
    return !Row.System.IsEmpty() && !Row.Region.IsEmpty() &&
        Agent.System.Equals(Row.System, ESearchCase::IgnoreCase) && Agent.Region.Equals(Row.Region, ESearchCase::IgnoreCase);
}

bool UAgentCatalogSubsystem::CanOfferCampaignMission(const FS_AgentCatalogRow& Agent, int32 CampaignId,
    const MissionInfo& Info, double CampaignTime, int32 Rank, const FString& GroupRegion, bool bCompleted, bool bActionSatisfied)
{
    // Query the campaign's live generated list without consuming exec_once or generating new missions.
    return Agent.CampaignIds.Contains(CampaignId) && AgentMissionRules::Encode(CampaignId, Info.id) != INDEX_NONE &&
        !Info.is_cutscene && Info.DisplayType == EMissionDisplayType::PlayerMission && Info.Source != EMissionSource::Cutscene &&
        (!Info.mission || !Info.mission->IsComplete()) && Info.exec_once >= 0 && !bCompleted && bActionSatisfied &&
        CampaignTime >= Info.start_after && CampaignTime <= Info.start_before && Rank >= Agent.MinimumRank &&
        Rank >= Info.min_rank && (Info.max_rank <= 0 || Rank <= Info.max_rank) &&
        (!Info.region.length() || GroupRegion.Equals(UTF8_TO_TCHAR(Info.region.data()), ESearchCase::IgnoreCase));
}

bool UAgentCatalogSubsystem::NeedsCampaignDeployment(int32 Id) const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || Id!=2)return false;
    if(CampaignPresentationRules::HasPendingFleetDeployment(Players->GetPlayerInfo(),Id))return true;
    auto* Simulation=Sim::GetSim();
    auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    auto* Base=Pilot?Pilot->GetCarrier():nullptr;
    return CampaignPresentationRules::NeedsFleetReturnRecovery(Players->GetPlayerInfo(),
        Simulation && Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight(),
        Base && FCStringAnsi::Stricmp(Base->GetName(),"Janek Station")==0 && Pilot->GetFlightPhase()<=EOPSMode::ALERT);
}

void UAgentCatalogSubsystem::CompleteCampaignDeploymentOnArrival()
{
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    if(!Players || !CampaignPresentationRules::HasPendingFleetDeployment(Players->GetPlayerInfo(),2))return;
    const auto* Data=GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row=Data?Data->GetCampaignByIndex1Based(2):nullptr;
    if(!Row)return;
    auto* Simulation=Sim::GetSim();
    auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    auto* Carrier=Pilot?Pilot->GetCarrier():nullptr;
    auto* Region=Carrier?Carrier->GetRegion():nullptr;
    if(!Region || Carrier->IsStatic() || Region!=Simulation->GetActiveRegion() || !Region->GetSystem() ||
        !Row->System.Equals(UTF8_TO_TCHAR(Region->GetSystem()->GetName()),ESearchCase::IgnoreCase) ||
        !Row->Region.Equals(UTF8_TO_TCHAR(Region->GetName()),ESearchCase::IgnoreCase))return;
    auto& Info=Players->GetMutablePlayerInfo();
    // Commit the destination world before marking the one-time deployment complete.
    // Otherwise a crash can pair a completed pilot flag with an older Janek checkpoint.
    Info.PendingFleetDeploymentCampaign=0;
    auto* State=GetGameInstance()->GetSubsystem<ULocalGameStateSubsystem>();
    if(!(State?State->SaveCheckpointNow():Players->SaveActivePlayerProgress()))
    {
        Info.PendingFleetDeploymentCampaign=2;
        Players->SaveActivePlayerProgress();
    }
    else
    {
        UE_LOG(LogTemp,Display,TEXT("[CampaignTransit] Meridian deployment completed and saved"));
        AgentAI::OnJournalChanged().Broadcast();
    }
}

bool UAgentCatalogSubsystem::IsAboardTrainingCarrier() const
{
    auto* Simulation=Sim::GetSim();auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    auto* Carrier=Pilot?Pilot->GetCarrier():nullptr;
    return Carrier && !Carrier->IsStatic() && !Carrier->IsDead() && Carrier->Design() &&
        FCStringAnsi::Stricmp(Carrier->Design()->name,"Orion")==0 && Pilot->GetFlightPhase()<=EOPSMode::ALERT;
}

int32 UAgentCatalogSubsystem::GetTrainingTransferStage() const
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Simulation=Sim::GetSim();auto* Pilot=Simulation?Simulation->GetPlayerShip():nullptr;
    auto* Base=Pilot?Pilot->GetCarrier():nullptr;
    if(!Players)return 0;
    return TrainingShuttleRules::TransferStage(Players->GetPlayerInfo(),
        Simulation && Simulation->GetMission() && Simulation->GetMission()->IsFreeFlight(),
        Pilot && Pilot->GetFlightPhase()<=EOPSMode::ALERT,IsAboardTrainingCarrier(),
        Base && FCStringAnsi::Stricmp(Base->GetName(),"Janek Station")==0);
}

bool UAgentCatalogSubsystem::TransferTrainingPilot(FText& Reason)
{
    auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Data=GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    auto* Simulation=Sim::GetSim();
    const int32 Stage=GetTrainingTransferStage();
    if(!Players || !Data || !Simulation || !Stage)
    {Reason=INVTEXT("Training shuttle orders are unavailable. Complete T2 for TDY to Janek Station, or T4 to return to Orion, then dock.");return false;}
    auto* Pilot=Simulation->GetPlayerShip();
    Ship* Destination=nullptr;
    if(Stage==1)
    {
        Simulation->EnsureTrainingSurfaceBase();
        Destination=Simulation->FindShip("Janek Station");
    }
    else
    {
        for(ListIter<SimRegion> Region=Simulation->GetRegions();++Region;)
            for(ListIter<Ship> Vessel=Region->GetShips();++Vessel;)
                if(!Vessel->IsStatic() && !Vessel->IsDead() && !Vessel->IsDying() && Vessel->GetIFF()==Pilot->GetIFF() &&
                    Vessel->Design() && FCStringAnsi::Stricmp(Vessel->Design()->name,"Orion")==0)Destination=Vessel.value();
        if(!Destination)
        {
            FS_CampaignMission Entry;
            if(!Data->ReadLooseMission(FPaths::ProjectContentDir()/TEXT("GameData/Campaigns/01/Scenes/FreeFlight.json"),Entry))
            {Reason=INVTEXT("Orion's return-transfer orders could not be loaded. Your position is unchanged.");return false;}
            const auto* Authored=Entry.Element.FindByPredicate([](const auto& E){return E.Design==TEXT("Orion");});
            auto* Design=Authored?ShipDesignRegistry::FindLegacy(FName(*Authored->Design)):nullptr;
            auto* Region=Authored?CampaignFleetTransit::EnsureRegion(*Simulation,Entry.MissionSystem,
                Authored->RegionName.IsEmpty()?Entry.MissionRegion:Authored->RegionName):nullptr;
            if(Region && Design)
            {
                Destination=Simulation->CreateShip(TCHAR_TO_UTF8(*Authored->Name),"",Design,Region->GetName(),Authored->Location,Pilot->GetIFF(),Authored->CommandAI);
                if(Destination)if(auto* Group=Simulation->CreateElement(TCHAR_TO_UTF8(*Authored->Name),Pilot->GetIFF()))Group->AddShip(Destination);
            }
        }
    }
    if(!Destination){Reason=INVTEXT("The training shuttle destination is unavailable. Remain docked and retry.");return false;}
    if(!CampaignFleetTransit::BoardCarrier(*Simulation,*Destination,Reason))return false;
    // Recover older Campaign 2 saves stranded at Janek without accepting anything new.
    if(Stage==2 && Players->GetPlayerInfo().Campaign==2)
        Players->GetMutablePlayerInfo().PendingFleetDeploymentCampaign=2;
    auto* State=GetGameInstance()->GetSubsystem<ULocalGameStateSubsystem>();
    if(!(State?State->SaveCheckpointNow():Players->SaveActivePlayerProgress()))
    {Reason=INVTEXT("Shuttle arrival completed, but the checkpoint could not be saved. Save before exiting.");return false;}
    Reason=Stage==1?INVTEXT("TDY to Janek Station. Report to the training agent for T3 and T4."):
        INVTEXT("Returned to Orion. Your TDY is complete. Review Campaign 2 orders with the agent when ready.");
    AgentAI::OnJournalChanged().Broadcast();
    return true;
}

bool UAgentCatalogSubsystem::DeployCampaignFleet(int32 Id,FText& Reason)
{
    const auto* Players=GetGameInstance()->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Data=GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row=Data?Data->GetCampaignByIndex1Based(Id):nullptr;
    auto* Simulation=Sim::GetSim();
    if(Id!=2 || !Players || Players->GetPlayerInfo().Campaign!=Id || !Row || !Simulation ||
        !Simulation->GetMission() || !Simulation->GetMission()->IsFreeFlight() || !IsAboardTrainingCarrier())
    {Reason=INVTEXT("Return to Orion, then accept Campaign 2 orders before deploying the fleet.");return false;}
    return CampaignFleetTransit::Start(*Simulation,Row->System,Row->Region,Reason);
}

FName UAgentCatalogSubsystem::GetPostTrainingCampaignAgent() const
{
    auto* Game=Cast<USSWGameInstance>(GetGameInstance());
    const auto* Players=Game?Game->GetSubsystem<UStarshatterPlayerSubsystem>():nullptr;
    const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    if(!Game || Game->IsTrainingTestSession() || !Players || !Pilot || !IsAboardTrainingCarrier())return NAME_None;
    if(!CampaignPresentationRules::NeedsTrainingHandoff(Players->GetPlayerInfo(),
        [&](int32 Id){return Game->GetTrainingAgent().IsCompleted(AgentAI::PilotKey(*Pilot),Id);}))return NAME_None;
    TArray<FName> Issuers;
    for(const auto& Pair:Catalog)
        if(CanContact(Pair.Key) && IsCampaignIssuer(Pair.Key,2))Issuers.Add(Pair.Key);
    Issuers.Sort(FNameLexicalLess());
    return Issuers.IsEmpty()?NAME_None:Issuers[0];
}

bool UAgentCatalogSubsystem::CanActivateCampaign(FName AgentId, int32 Id) const
{
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Agent = FindAgent(AgentId);
    if (!Game || Game->IsTrainingTestSession() || !Agent || !CanContact(AgentId) || !Agent->CampaignIds.Contains(Id) || Id < 2) return false;
    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row = Data ? Data->GetCampaignByIndex1Based(Id) : nullptr;
    auto* Simulation = Sim::GetSim();
    if (Simulation && Simulation->GetMission() && (!Simulation->GetMission()->IsFreeFlight() || !FStationServices::IsDocked())) return false;
    if(Id==2 && !IsAboardTrainingCarrier())return false;
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    if(Id==2 && Players && Players->GetPlayerInfo().Campaign==1 &&
        !Players->GetPlayerInfo().HasCompletedMission(1,104) &&
        (!Pilot || !Game->GetTrainingAgent().IsCompleted(AgentAI::PilotKey(*Pilot),104)))return false;
    return Players && Pilot && Pilot->GetRank() >= Agent->MinimumRank && Row && Row->bAvailable && IsCampaignOrigin(*Agent, *Row) &&
        CampaignPresentationRules::CanSelectCampaign(Players->GetPlayerInfo()) &&
        CampaignPresentationRules::IsCampaignUnlocked(Id, Players->GetPlayerInfo()) &&
        (!Campaign::GetCampaign() || Campaign::GetCampaign()->GetCampaignId() != Id) &&
        Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot)).IsEmpty();
}

bool UAgentCatalogSubsystem::ActivateCampaign(FName AgentId, int32 Id)
{
    if (!CanActivateCampaign(AgentId, Id)) return false;
    auto* Game = CastChecked<USSWGameInstance>(GetGameInstance());
    auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row = Data->GetCampaignByIndex1Based(Id);
    auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!Row || !Players) return false;
    const FS_PlayerGameInfo PreviousPlayer = Players->GetPlayerInfo();
    auto& PendingPlayer = Players->GetMutablePlayerInfo();
    PendingPlayer.Campaign = Id;
    PendingPlayer.CampaignRowName = Row->RowName;
    PendingPlayer.PendingFleetDeploymentCampaign = Id==2 ? Id : 0;
    if (!Players->SaveActivePlayerProgress())
    {
        Players->GetMutablePlayerInfo() = PreviousPlayer;
        UE_LOG(LogTemp, Warning, TEXT("[Agents] Campaign acceptance could not save; prior operation retained."));
        return false;
    }
    if (!Campaign::SelectFromData(*Row))
    {
        Players->GetMutablePlayerInfo() = PreviousPlayer;
        if (!Players->SaveActivePlayerProgress())
            UE_LOG(LogTemp, Error, TEXT("[Agents] Could not restore saved campaign after activation failure."));
        return false;
    }
    Campaign::GetCampaign()->Start();
    Data->SetActiveCampaign(*Row);
    Data->CampaignIndex = Id - 1;
    Data->SelectedCampaignRowName = Row->RowName;
    Data->BuildCombatRosterFromDataTables();
    Game->SelectedCampaignIndex = Id;
    Game->SelectedCampaignRowName = Row->RowName;
    Game->SelectedCampaignDisplayName = Row->Name;
    Game->LoadOrCreateSelectedCampaignSave();
    if (auto* Timer = Game->GetSubsystem<UTimerSubsystem>()) Timer->SetCampaignSave(Game->CampaignSave);
    UE_LOG(LogTemp, Display, TEXT("[Agents] Campaign %d introduced by %s"), Id, *AgentId.ToString());
    return true;
}


bool UAgentCatalogSubsystem::AssignTrainingOperation(FName AgentId, bool bRequireAcceptedMission)
{
    auto* Game=Cast<USSWGameInstance>(GetGameInstance());
    const auto* Agent=FindAgent(AgentId);
    const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    if(!Game || !Agent || Agent->Class!=EAgentClass::Training || !Pilot)return false;
    const auto Queue=Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot));
    if(bRequireAcceptedMission && !Queue.ContainsByPredicate([](const AgentAI::FAssignment& Item){return Item.MissionId>=101 && Item.MissionId<=107 && (Item.State==AgentAI::EState::Accepted || Item.State==AgentAI::EState::InProgress);}))return false;
    if(Game->IsTrainingTestSession() && bRequireAcceptedMission)
    {
        UE_LOG(LogTemp,Display,TEXT("[Agents] Operation Live Fire accepted in test session; career unchanged"));
        return true;
    }
    auto* Players=Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    auto* Data=Game->GetSubsystem<UStarshatterGameDataSubsystem>();
    if(!Players || !Data)return false;
    // A refresher lesson must not replace an existing story campaign.
    if(Players->GetPlayerInfo().Campaign>Campaign::TRAINING_CAMPAIGN)return bRequireAcceptedMission;
    const auto* Row=Data->GetCampaignByIndex1Based(Campaign::TRAINING_CAMPAIGN);
    FString TrainingError;if(Row)Row=FactionTrainingCatalog::Resolve(*Row,Pilot->GetEmpire(),TrainingError);
    if(!Row || Agent->Empire!=Pilot->GetEmpire())return false;
    const FS_PlayerGameInfo Before=Players->GetPlayerInfo();
    auto& Info=Players->GetMutablePlayerInfo(); Info.Campaign=Campaign::TRAINING_CAMPAIGN; Info.CampaignRowName=Row->RowName;
    // Save the durable assignment before changing runtime campaign ownership.
    if (!Players->SaveActivePlayerProgress())
    {
        Players->GetMutablePlayerInfo()=Before;
        return false;
    }
    if(!Campaign::GetCampaign() || Campaign::GetCampaign()->GetCampaignId()!=Campaign::TRAINING_CAMPAIGN || FString(UTF8_TO_TCHAR(Campaign::GetCampaign()->GetName()))!=Row->Name)
    {
        if(!Campaign::SelectFromData(*Row))
        {
            Players->GetMutablePlayerInfo()=Before;
            if(!Players->SaveActivePlayerProgress()) UE_LOG(LogTemp,Error,TEXT("[Agents] Training activation failed; prior assignment could not be resaved."));
            return false;
        }
        Campaign::GetCampaign()->Start();
    }
    Data->SetActiveCampaign(*Row);Data->CampaignIndex=Campaign::TRAINING_CAMPAIGN-1;
    Data->SelectedCampaignRowName=Row->RowName;Data->BuildCombatRosterFromDataTables();
    Game->SelectedCampaignIndex=Campaign::TRAINING_CAMPAIGN;Game->SelectedCampaignRowName=Row->RowName;Game->SelectedCampaignDisplayName=Row->Name;
    Game->LoadOrCreateSelectedCampaignSave();
    if(auto* Timer=Game->GetSubsystem<UTimerSubsystem>())Timer->SetCampaignSave(Game->CampaignSave);
    UE_LOG(LogTemp,Display,TEXT("[Agents] Pilot assigned to Operation Live Fire; saved=1"));
    return true;
}

FText UAgentCatalogSubsystem::GetCurrentOperationName() const
{
    auto* Game=Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot=PlayerCharacter::GetCurrentPlayer();
    if(Game && Game->IsTrainingTestSession() && Pilot)
    {
        const auto Queue=Game->GetTrainingAgent().GetQueue(AgentAI::PilotKey(*Pilot));
        if(Queue.ContainsByPredicate([](const AgentAI::FAssignment& Item){return Item.MissionId>=101 && Item.MissionId<=107;}))
            return INVTEXT("Operation Live Fire (Test)");
    }
    const auto* Active=Campaign::GetCampaign();
    return Active?FText::FromString(UTF8_TO_TCHAR(Active->GetName())):INVTEXT("Unassigned");
}
