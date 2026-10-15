#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CampaignSlotSave.generated.h"

// Mirrors the legacy CampaignSlotSave text format so saves stay compatible.
USTRUCT(BlueprintType)
struct FCampaignCombatantSave
{
    GENERATED_BODY()
    UPROPERTY() FString Name;
    UPROPERTY() int32 IFF = 0;
    UPROPERTY() int32 Score = 0;
};

USTRUCT(BlueprintType)
struct FCampaignActionSave
{
    GENERATED_BODY()
    UPROPERTY() int32 Id = 0;
    UPROPERTY() int32 Status = 0;
    UPROPERTY() int32 Count = 0;
    UPROPERTY() int32 After = 0;
};

USTRUCT(BlueprintType)
struct FCampaignEventSave
{
    GENERATED_BODY()
    UPROPERTY() FString Type;
    UPROPERTY() int32 Time = 0;
    UPROPERTY() int32 Team = 0;
    UPROPERTY() int32 Points = 0;
    UPROPERTY() FString Source;
    UPROPERTY() FString Region;
    UPROPERTY() FString Title;
    UPROPERTY() FString File;
    UPROPERTY() FString Image;
    UPROPERTY() FString Scene;
    UPROPERTY() FString Info;
};

UCLASS()
class STARSHATTERWARS_API UCampaignSlotSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() FString CampaignName;
    UPROPERTY() int32 GroupIFF = 0;
    UPROPERTY() int32 GroupType = 0;
    UPROPERTY() int32 GroupId = 0;
    UPROPERTY() FString UnitName;
    UPROPERTY() int32 Status = 0;
    UPROPERTY() double BaseTime = 0;
    UPROPERTY() double Time = 0;
    UPROPERTY() FString Sitrep;
    UPROPERTY() FString Orders;
    UPROPERTY() TArray<FCampaignCombatantSave> Combatants;
    UPROPERTY() TArray<FCampaignActionSave> Actions;
    UPROPERTY() TArray<FCampaignEventSave> Events;

    // Slot is per campaign: "Campaign_<name>".
    static FString SlotFor(const FString& CampaignName);
    static bool SaveCampaign(class Campaign* C);
    static UCampaignSlotSave* LoadCampaign(const FString& CampaignName);
    static bool DeleteCampaign(const FString& CampaignName);
    // Restore saved state onto a loaded campaign.
    bool ApplyTo(class Campaign* C) const;
};
