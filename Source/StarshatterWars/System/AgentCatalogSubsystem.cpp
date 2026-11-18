#include "AgentCatalogSubsystem.h"
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
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        UE_LOG(LogTemp, Warning, TEXT("Agent catalog missing: %s"), *Path);
        return;
    }

    auto* Imported = NewObject<UDataTable>(this);
    Imported->RowStruct = FS_AgentCatalogRow::StaticStruct();
    const auto Problems = Imported->CreateTableFromJSONString(Json);
    for (const auto& Problem : Problems) UE_LOG(LogTemp, Warning, TEXT("%s: %s"), *Path, *Problem);
    if (!Problems.IsEmpty() || Imported->GetRowMap().IsEmpty()) return;

    for (const auto& Pair : Imported->GetRowMap())
    {
        const auto& Agent = *reinterpret_cast<const FS_AgentCatalogRow*>(Pair.Value);
        if (Agent.DisplayName.IsEmpty() || Agent.System.IsEmpty() || Agent.Planet.IsEmpty() ||
            Agent.Region.IsEmpty() || Agent.Station.IsEmpty() || Agent.Division.IsEmpty() ||
            Agent.MinimumRank < 0 || !Agent.ServesEmpire(Agent.Empire) || Agent.Level < 1 || Agent.Level > 5)
        {
            UE_LOG(LogTemp, Warning, TEXT("Invalid agent definition: %s"), *Pair.Key.ToString());
            return;
        }
    }
    Catalog = Imported;
    UE_LOG(LogTemp, Display, TEXT("Imported %d agents from %s"), Catalog->GetRowMap().Num(), *Path);
}

const FS_AgentCatalogRow* UAgentCatalogSubsystem::FindAgent(FName Id) const
{
    return Catalog ? Catalog->FindRow<FS_AgentCatalogRow>(Id, TEXT("Agent catalog"), false) : nullptr;
}

bool UAgentCatalogSubsystem::CanContact(FName Id) const
{
    const auto* Agent = FindAgent(Id);
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    const auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    if (Game && Game->IsTrainingTestSession() && Id != TEXT("JanekFlightTraining")) return false;
    return Agent && Pilot && Agent->ServesEmpire(Pilot->GetEmpire());
}

TArray<int32> UAgentCatalogSubsystem::GetEligibleMissions(FName AgentId) const
{
    TArray<int32> Eligible;
    const auto* Agent = FindAgent(AgentId);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    if (!Agent || !Game || !CanContact(AgentId) || !MissionRankRules::Unlocked(Agent->MinimumRank)) return Eligible;

    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
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

    // Orders and dynamic assignments remain owned by the active SSW campaign.
    if (!Game->IsTrainingTestSession())
        if (auto* Active = Campaign::GetCampaign(); Active && Active->IsActive() && IsCampaignIssuer(AgentId, Active->GetCampaignId()))
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

    if (const auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>())
        for (const auto& Scenario : Data->GetSingleMissionScenarios())
        {
            if (Scenario.Id <= 0 || Scenario.Id >= 1000000 - Mission::RegionalAgentMissionOffset || Scenario.StoryCampaignId > 0 ||
                !Agent->MissionIds.Contains(Scenario.Id) ||
                !AgentMissionRules::LocalSystem(Agent->System, Scenario.MissionSystem) ||
                !MissionRankRules::Unlocked(MissionRankRules::Required(Scenario))) continue;

            const int32 Id = Mission::RegionalAgentMissionOffset + Scenario.Id;
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
    if(!Game || !Game->IsTrainingTestSession() || !Pilot || !Agent || Agent->Class!=EAgentClass::Training ||
        !Agent->MissionIds.Contains(MissionId) || !FTrainingTestRules::IsLesson(MissionId) || !CanContact(AgentId))return false;
    const FName Key=AgentAI::PilotKey(*Pilot);
    const FName Skill=FTrainingTestRules::RequiredSkill(MissionId);
    const auto* Skills=Game->GetSubsystem<USkillsSubsystem>();
    return Pilot->GetRank()>=FTrainingTestRules::RequiredRank(MissionId) && (Skill.IsNone() || (Skills && Skills->HasSkill(Skill))) &&
        Game->GetTrainingAgent().IsCompleted(Key,MissionId) && Game->GetTrainingAgent().GetQueue(Key).IsEmpty();
}

bool UAgentCatalogSubsystem::CanAccept(int32 MissionId, FName AgentId) const
{
    if (!CanContact(AgentId)) return false;
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
    const auto* Agent = FindAgent(AgentId);
    if (!Game || !Pilot || !Agent || Pilot->GetRank() < Agent->MinimumRank) return false;
    if (FindScenario(MissionId))
    {
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
        if (FTrainingTestRules::IsLesson(MissionId)) return !Queue[0].bTestReplay || Game->IsTrainingTestSession();
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
    return INVTEXT("OPERATION LIVE FIRE | Complete each lesson in sequence. Advanced command lessons require the appropriate rank and hull-command skill.");
}

FText UAgentCatalogSubsystem::GetTrainingRequirement(int32 Id) const
{
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

bool UAgentCatalogSubsystem::CanActivateCampaign(FName AgentId, int32 Id) const
{
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    const auto* Agent = FindAgent(AgentId);
    if (!Game || Game->IsTrainingTestSession() || !Agent || !CanContact(AgentId) || !Agent->CampaignIds.Contains(Id) || Id < 2) return false;
    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    const auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>();
    const auto* Row = Data ? Data->GetCampaignByIndex1Based(Id) : nullptr;
    auto* Simulation = Sim::GetSim();
    if (Simulation && Simulation->GetMission() && (!Simulation->GetMission()->IsFreeFlight() || !Simulation->HasPlayerDocked())) return false;
    const auto* Pilot = PlayerCharacter::GetCurrentPlayer();
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
    if(!Row)return false;
    const FS_PlayerGameInfo Before=Players->GetPlayerInfo();
    auto& Info=Players->GetMutablePlayerInfo(); Info.Campaign=Campaign::TRAINING_CAMPAIGN; Info.CampaignRowName=Row->RowName;
    // Save the durable assignment before changing runtime campaign ownership.
    if (!Players->SaveActivePlayerProgress())
    {
        Players->GetMutablePlayerInfo()=Before;
        return false;
    }
    if(!Campaign::GetCampaign() || Campaign::GetCampaign()->GetCampaignId()!=Campaign::TRAINING_CAMPAIGN)
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
