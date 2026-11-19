#pragma once

#include "Engine/DataTable.h"
#include "GameStructs.h"
#include "AgentDirectoryRules.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentCatalogSubsystem.generated.h"

class Campaign;
class Mission;
class MissionInfo;

UENUM(BlueprintType)
enum class EAgentClass : uint8
{
    Training,
    Security,
    Diplomatic
};

USTRUCT(BlueprintType)
struct FS_AgentCatalogRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Division;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString System;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Planet;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Region;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Station;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EAgentClass Class = EAgentClass::Training;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EEMPIRE_NAME Empire = EEMPIRE_NAME::NONE;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumRank = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Level = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Description;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> MissionIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<EMissionType> MissionTypes;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> CampaignIds;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> DynamicPools;

    bool ServesEmpire(EEMPIRE_NAME PilotEmpire) const
    {
        return PilotEmpire != EEMPIRE_NAME::NONE && PilotEmpire != EEMPIRE_NAME::Unknown &&
            Empire != EEMPIRE_NAME::NONE && Empire != EEMPIRE_NAME::Unknown &&
            (Empire == EEMPIRE_NAME::Neutral || Empire == PilotEmpire);
    }
};

UCLASS()
class STARSHATTERWARS_API UAgentCatalogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void LoadCatalog();
    bool PrepareIndependentOffer(FName Agent,FText& Reason);
    bool StartIndependentAssignment(int32 Id,FText& Reason);
    const TMap<FName, FS_AgentCatalogRow>& GetCatalog() const { return Catalog; }
    const FS_AgentCatalogRow* FindAgent(FName Id) const;
    bool CanContact(FName Id) const;
    TArray<FName> GetDirectorySystems(FName Origin, const TSet<FName>& Neighbors, EAgentDirectoryScope Scope) const;
    TArray<FName> GetSystemAgents(FName System) const;
    TArray<int32> GetEligibleMissions(FName AgentId) const;
    bool CanAccept(int32 MissionId, FName AgentId) const;
    bool CanReplayTrainingTest(int32 MissionId,FName AgentId) const;
    FText GetMissionTitle(int32 MissionId) const;
    Mission* GetMission(int32 MissionId);
    Campaign* GetRepository(int32 MissionId);
    FText GetAssignmentStatus(FName AgentId) const;
    FText GetTrainingRequirement(int32 MissionId) const;
    FText GetCampaignOrders(FName AgentId) const;
    FText GetCampaignTitle(int32 CampaignId) const;
    bool CanActivateCampaign(FName AgentId, int32 CampaignId) const;
    static bool IsCampaignOrigin(const FS_AgentCatalogRow& Agent, const FS_Campaign& Campaign);
    static bool CanOfferCampaignMission(const FS_AgentCatalogRow& Agent, int32 CampaignId, const MissionInfo& Info,
        double CampaignTime, int32 Rank, const FString& GroupRegion, bool bCompleted, bool bActionSatisfied);
    bool ActivateCampaign(FName AgentId, int32 CampaignId);
    bool AssignTrainingOperation(FName AgentId, bool bRequireAcceptedMission = true);
    FText GetCurrentOperationName() const;

private:
    bool IsCampaignIssuer(FName AgentId, int32 CampaignId) const;
    bool IsStoryUnlocked(const Campaign& Active, int32 MissionId) const;
    const FS_CampaignMission* FindScenario(int32 Id) const;

    UPROPERTY(Transient) TMap<FName, FS_AgentCatalogRow> Catalog;
    TSharedPtr<Campaign> RegionalRepository;
    TMap<int32, TSharedPtr<Mission>> RegionalMissions;
};
