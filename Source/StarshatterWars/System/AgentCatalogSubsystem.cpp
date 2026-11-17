#include "AgentCatalogSubsystem.h"

#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"
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
    return Agent && Pilot && Agent->ServesEmpire(Pilot->GetEmpire());
}

TArray<int32> UAgentCatalogSubsystem::GetEligibleMissions(FName AgentId) const
{
    TArray<int32> Eligible;
    const auto* Agent = FindAgent(AgentId);
    auto* Game = Cast<USSWGameInstance>(GetGameInstance());
    if (!Agent || !Game || !CanContact(AgentId) || !MissionRankRules::Unlocked(Agent->MinimumRank)) return Eligible;

    const auto* Players = Game->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (Agent->Class == EAgentClass::Training)
    {
        auto* Training = Game->GetAgentTrainingCampaign();
        if (!Training) return Eligible;
        ListIter<MissionInfo> It = Training->GetMissionList();
        while (++It)
        {
            const auto* Info = It.value();
            if (Info && Info->id > 0 && Info->DisplayType != EMissionDisplayType::Hidden &&
                Info->DisplayType != EMissionDisplayType::CutsceneOnly && !Training->IsCutsceneMission(Info->id) &&
                Training->IsMissionRankUnlocked(Info->id) &&
                (!Players || !Players->GetPlayerInfo().HasCompletedMission(Training->GetCampaignId(), Info->id)))
                Eligible.Add(Info->id);
        }
        return Eligible;
    }

    if (const auto* Data = Game->GetSubsystem<UStarshatterGameDataSubsystem>())
        for (const auto& Scenario : Data->GetSingleMissionScenarios())
        {
            if (Scenario.Id <= 0 || Scenario.Id >= 1000000 - Mission::RegionalAgentMissionOffset || Scenario.StoryCampaignId > 0 ||
                !Agent->MissionIds.Contains(Scenario.Id) ||
                !Agent->System.Equals(Scenario.MissionSystem, ESearchCase::IgnoreCase) ||
                !Agent->Region.Equals(Scenario.MissionRegion, ESearchCase::IgnoreCase) ||
                !MissionRankRules::Unlocked(MissionRankRules::Required(Scenario))) continue;

            const int32 Id = Mission::RegionalAgentMissionOffset + Scenario.Id;
            if (!Players || !Players->GetPlayerInfo().HasCompletedMission(Campaign::SINGLE_MISSIONS, Id))
                Eligible.AddUnique(Id);
        }
    return Eligible;
}

bool UAgentCatalogSubsystem::CanAccept(int32 MissionId, FName AgentId) const
{
    return GetEligibleMissions(AgentId).Contains(MissionId);
}

const FS_CampaignMission* UAgentCatalogSubsystem::FindScenario(int32 Id) const
{
    if (Id < Mission::RegionalAgentMissionOffset) return nullptr;
    const auto* Data = GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>();
    return Data ? Data->FindSingleMissionScenario(Id - Mission::RegionalAgentMissionOffset) : nullptr;
}

FText UAgentCatalogSubsystem::GetMissionTitle(int32 MissionId) const
{
    if (const auto* Scenario = FindScenario(MissionId)) return FText::FromString(Scenario->MissionName);
    if (auto* Game = Cast<USSWGameInstance>(GetGameInstance()))
        if (auto* Training = Game->GetAgentTrainingCampaign())
            if (const auto* Info = Training->GetMissionInfo(MissionId)) return FText::FromString(UTF8_TO_TCHAR(Info->name.data()));
    return FText::GetEmpty();
}

Campaign* UAgentCatalogSubsystem::GetRepository(int32 MissionId)
{
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
