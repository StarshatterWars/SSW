#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameStructs.h"
#include "StarshatterPlayerCharacter.generated.h"

class UStarshatterPlayerSubsystem;

UCLASS(BlueprintType)
class STARSHATTERWARS_API UStarshatterPlayerCharacter : public UObject
{
    GENERATED_BODY()

public:
    // Subsystem assigns this after NewObject<>
    void Initialize(UStarshatterPlayerSubsystem* InOwnerSubsystem);

    // Full mirror sync
    void FromPlayerInfo(const FS_PlayerGameInfo& InInfo);
    void ToPlayerInfo(FS_PlayerGameInfo& OutInfo) const;

    UFUNCTION(BlueprintCallable, Category = "Starshatter|Player")
    bool Commit(bool bForceSave = true);

    void LoadFromPlayerInfo(const FS_PlayerGameInfo& Info);

private:
    UStarshatterPlayerSubsystem* OwnerSubsystem = nullptr;

public:

    // ------------------------------------------------------------
    // Identity / profile
    // ------------------------------------------------------------
    UPROPERTY() int32 Id = 0;
    UPROPERTY() FString Name;
    UPROPERTY() FString Callsign;

    UPROPERTY() FString Squadron;
    UPROPERTY() FString Signature;
    UPROPERTY() int32 Avatar = -1;

    // ------------------------------------------------------------
    // Campaign / progression routing
    // ------------------------------------------------------------
    UPROPERTY() int32 Campaign = 0;
    UPROPERTY() FName CampaignRowName = NAME_None;
    UPROPERTY() int32 Mission = -1;

    // ------------------------------------------------------------
    // Rank / faction
    // ------------------------------------------------------------
    UPROPERTY() int32 Rank = 0;
    UPROPERTY() int32 Empire = 0;

    // ------------------------------------------------------------
    // Visual / HUD / controls prefs
    // ------------------------------------------------------------
    UPROPERTY() int32 ShipColor = 0;
    UPROPERTY() int32 HudMode = 0;
    UPROPERTY() int32 GunMode = 0;
    UPROPERTY() int32 HudColor = 0;
    UPROPERTY() int32 FlightModel = 0;
    UPROPERTY() int32 LandingMode = 0;

    UPROPERTY() bool FlyingStart = false;
    UPROPERTY() bool GridMode = false;
    UPROPERTY() bool TrainingMode = false;
    UPROPERTY() bool GunSightMode = false;

    UPROPERTY() int32 AILevel = 0;
    UPROPERTY() int32 ForceFeedbackLevel = 0;

    // ------------------------------------------------------------
    // Time
    // ------------------------------------------------------------
    UPROPERTY() int64 CreateTime = 0;
    UPROPERTY() int64 GameTime = 0;
    UPROPERTY() int64 CampaignTime = 0;
    UPROPERTY() int64 FlightTime = 0;

    // ------------------------------------------------------------
    // Career / logbook stats
    // ------------------------------------------------------------
    UPROPERTY() int32 PlayerKills = 0;
    UPROPERTY() int32 PlayerWins = 0;
    UPROPERTY() int32 PlayerLosses = 0;
    UPROPERTY() int32 PlayerDeaths = 0;
    UPROPERTY() int32 PlayerMissions = 0;
    UPROPERTY() int32 PlayerPoints = 0;
    UPROPERTY() int32 PlayerLevel = 0;
    UPROPERTY() int32 PlayerExperience = 0;

    UPROPERTY() FString PlayerStatus;
    UPROPERTY() FString PlayerShip;
    UPROPERTY() FString PlayerRegion;
    UPROPERTY() FString PlayerSystem;

    // ------------------------------------------------------------
    // OOB / unit selection
    // ------------------------------------------------------------
    UPROPERTY() int32 PlayerSquadron = -1;
    UPROPERTY() int32 PlayerWing = -1;
    UPROPERTY() int32 PlayerDesronGroup = -1;
    UPROPERTY() int32 PlayerBattleGroup = -1;
    UPROPERTY() int32 PlayerCarrier = -1;
    UPROPERTY() int32 PlayerFleet = -1;
    UPROPERTY() int32 PlayerForce = 1;

    // ------------------------------------------------------------
    // Campaign completion
    // ------------------------------------------------------------
    UPROPERTY() int64 CampaignCompleteMask = 0;
    UPROPERTY() TArray<uint8> CampaignComplete;

    // ------------------------------------------------------------
    // Training
    // ------------------------------------------------------------
    UPROPERTY() int32 HighestTrainingMission = 0;
    UPROPERTY() int64 TrainingMask = 0;
    UPROPERTY() int32 Trained = 0;

    // ------------------------------------------------------------
    // Awards / medals
    // ------------------------------------------------------------
    UPROPERTY() int32 MedalsMask = 0;

    // ------------------------------------------------------------
    // Chat / MFD
    // ------------------------------------------------------------
    UPROPERTY() TArray<FString> ChatMacros; // 10
    UPROPERTY() TArray<int32> MfdModes;     // 3
};
