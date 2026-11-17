#pragma once

#include "Engine/DataTable.h"
#include "GameStructs.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AgentCatalogSubsystem.generated.h"

class Campaign;
class Mission;

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

    bool ServesEmpire(EEMPIRE_NAME PilotEmpire) const
    {
        return Empire != EEMPIRE_NAME::NONE && Empire != EEMPIRE_NAME::Unknown && Empire == PilotEmpire;
    }
};

UCLASS()
class STARSHATTERWARS_API UAgentCatalogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    void LoadCatalog();
    const UDataTable* GetCatalog() const { return Catalog; }
    const FS_AgentCatalogRow* FindAgent(FName Id) const;
    bool CanContact(FName Id) const;
    TArray<int32> GetEligibleMissions(FName AgentId) const;
    bool CanAccept(int32 MissionId, FName AgentId) const;
    FText GetMissionTitle(int32 MissionId) const;
    Mission* GetMission(int32 MissionId);
    Campaign* GetRepository(int32 MissionId);

private:
    const FS_CampaignMission* FindScenario(int32 Id) const;

    UPROPERTY(Transient) TObjectPtr<UDataTable> Catalog;
    TSharedPtr<Campaign> RegionalRepository;
    TMap<int32, TSharedPtr<Mission>> RegionalMissions;
};
