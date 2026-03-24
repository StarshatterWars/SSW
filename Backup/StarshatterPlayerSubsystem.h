/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterPlayerSubsystem
    FILE:           StarshatterPlayerSubsystem.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UStarshatterPlayerSubsystem

    Unreal-native player profile persistence manager.

    Responsibilities:
    - Owns authoritative FS_PlayerGameInfo instance (PlayerInfo)
    - Loads player save during Boot (LoadFromBoot)
    - Writes player save on demand (SavePlayer)
    - Tracks dirty state to avoid unnecessary writes
    - Stores SaveVersion in UPlayerSaveGame and supports forward migration

    FIRST-RUN FLOW
    ==============
    Boot calls LoadFromBoot(). This subsystem records whether a save existed:
      - HadExistingSaveOnLoad() == true  -> normal menu flow
      - HadExistingSaveOnLoad() == false -> show FirstRun dialog

    IMPORTANT:
    - When no save exists, subsystem does NOT auto-create a save by default.
      FirstRun dialog is expected to create the save explicitly.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GameStructs.h"
#include "PlayerSaveGame.h"

#include "StarshatterPlayerSubsystem.generated.h"

class ShipStats;

// ------------------------------------------------------------
// Events
// ------------------------------------------------------------
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerSaveLoaded, bool, bSuccess);

// ------------------------------------------------------------
// Pending award typing (UI routing)
// ------------------------------------------------------------
UENUM(BlueprintType)
enum class EPendingAwardType : uint8
{
    None  UMETA(DisplayName = "None"),
    Rank  UMETA(DisplayName = "Rank"),
    Medal UMETA(DisplayName = "Medal"),
};

UCLASS()
class STARSHATTERWARS_API UStarshatterPlayerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------
    // Subsystem lifecycle
    // ------------------------------------------------------------------
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    static UStarshatterPlayerSubsystem* Get(const UObject* WorldContextObject);
    static UStarshatterPlayerSubsystem* Get(const UWorld* World);

    // ------------------------------------------------------------------
    // Boot entry point
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool LoadFromBoot();

    // ------------------------------------------------------------------
    // Slot configuration
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    void SetSaveSlot(const FString& InSlotName, int32 InUserIndex);

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    FString GetSlotName() const { return SlotName; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    int32 GetUserIndex() const { return UserIndex; }

    // ------------------------------------------------------------------
    // State / flags
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool HasLoaded() const { return bLoaded; }

    // Did a save exist at the moment LoadPlayer() ran?
    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool HadExistingSaveOnLoad() const { return bHadExistingSave; }

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    bool IsInitialized() const { return bLoaded; }

    static bool IsInitialized(const UObject* WorldContextObject);
    static bool IsInitialized(const UWorld* World);

    // Cached scalar read-through:
    int32 GetAILevel() const;

    void SetAILevel(int32 InLevel);

    int32 GetGridMode() const;

    void SetGridMode(int32 InMode);


    // ------------------------------------------------------------------
    // Player data access
    // ------------------------------------------------------------------
    const FS_PlayerGameInfo& GetPlayerInfo() const;
    FS_PlayerGameInfo& GetMutablePlayerInfo();

    UFUNCTION(BlueprintPure, Category = "Starshatter|PlayerSave")
    FS_PlayerGameInfo GetPlayerInfoCopy() const { return PlayerInfo; }

    bool DoesSaveExistNow() const;

    // ------------------------------------------------------------------
    // Save / Load
    // ------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool LoadPlayer();

    UFUNCTION(BlueprintCallable, Category = "Starshatter|PlayerSave")
    bool SavePlayer(bool bForce = false);

    // ------------------------------------------------------------------
    // Events
    // ------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Starshatter|PlayerSave")
    FOnPlayerSaveLoaded OnPlayerSaveLoaded;

    // ------------------------------------------------------------------
    // Safe getters (static convenience)
    // ------------------------------------------------------------------
    static int32   GetPlayerIdSafe(const UObject* WorldContextObject, int32 DefaultId = 0);
    static int32   GetRankIdSafe(const UObject* WorldContextObject, int32 DefaultRankId = 0);
    static FString GetPlayerNameSafe(const UObject* WorldContextObject);

    static FString GetRankNameSafe(const UObject* WorldContextObject, int32 RankId);
    static FString GetRankDescSafe(const UObject* WorldContextObject, int32 RankId);

    // ------------------------------------------------------------------
    // Training
    // ------------------------------------------------------------------
    bool HasTrained(int32 TrainingMissionId) const;
    void SetTrained(int32 TrainingMissionId, bool bTrained);
    static bool HasTrainedSafe(const UObject* WorldContextObject, int32 TrainingMissionId);

    // ------------------------------------------------------------------
    // Campaign completion
    // ------------------------------------------------------------------
    bool HasCompletedCampaign(int32 CampaignBitIndex) const;
    void SetCampaignComplete(int32 CampaignBitIndex, bool bComplete);

    static bool HasCompletedCampaignSafe(const UObject* WorldContextObject, int32 CampaignBitIndex);
    static void SetCampaignCompleteSafe(
        const UObject* WorldContextObject,
        int32 CampaignBitIndex,
        bool bComplete,
        bool bSave = true
    );

    // ------------------------------------------------------------------
    // Command eligibility (rank -> permissions)
    // ------------------------------------------------------------------
    bool CanCommand(int32 CmdClass) const;
    static bool CanCommandSafe(const UObject* WorldContextObject, int32 CmdClass);

    // ------------------------------------------------------------------
    // Legacy-style wrappers (drop-in callsites)
    // NOTE: Keep these names distinct from real instance methods.
    // ------------------------------------------------------------------
    static int32   GetPlayerId(const UObject* WorldContextObject);
    static FString GetPlayerName(const UObject* WorldContextObject);
    static int32   GetPlayerRankId(const UObject* WorldContextObject);

    static bool HasTrainedMission(const UObject* WorldContextObject, int32 MissionId);
    static void MarkTrainedMission(const UObject* WorldContextObject, int32 MissionId);

    static bool IsCampaignComplete(const UObject* WorldContextObject, int32 CampaignIndex0Based);
    static void SetCampaignComplete(const UObject* WorldContextObject, int32 CampaignIndex0Based, bool bComplete);

    // ------------------------------------------------------------------
    // Cached settings helpers (used by legacy ports)
    // ------------------------------------------------------------------
    static int32 GetCachedAILevel(int32 DefaultValue = 1);
    static bool  GetCachedGunSightMode(bool DefaultValue = false);

    // ------------------------------------------------------------------
    // Awards (pending award state lives in PlayerInfo)
    // ------------------------------------------------------------------
    bool  GetShowAward() const;
    int32 GetPendingAwardType() const;     // EPendingAwardType as int32 for older code
    int32 GetPendingAwardId() const;

    EPendingAwardType GetPendingAwardTypeEnum() const;

    FString GetCachedAwardTitle() const;
    FString GetCachedAwardBody() const;

    void SetPendingAward(const UObject* WorldContextObject, EPendingAwardType InType, int32 InId, bool bShow, bool bSave);

    static bool    GetCachedShowAward(const UObject* WorldContextObject, bool DefaultValue = false);
    static FString GetCachedAwardTitle(const UObject* WorldContextObject, const FString& DefaultValue = TEXT(""));
    static FString GetCachedAwardBody(const UObject* WorldContextObject, const FString& DefaultValue = TEXT(""));

    void ClearPendingAward(bool bSave = true);
    static void ClearPendingAwardSafe(const UObject* WorldContextObject, bool bSave = true);
    static void ClearPendingAward(const UObject* WorldContextObject, bool bSave = true);

    // ------------------------------------------------------------------
    // Cached identity shortcuts
    // ------------------------------------------------------------------
    static FString GetCachedPlayerName(const FString& DefaultValue = TEXT(""));
    static int32   GetCachedRankId(int32 DefaultValue = 0);

    // ------------------------------------------------------------------
    // Mission-end stats (replacement for PlayerCharacter::ProcessStats)
    // ------------------------------------------------------------------
    void ProcessStats(ShipStats* Stats, uint32 StartTimeMs);

    void SetTrained(int32 TrainingIdentity);

    void SavePlayer();

    // ------------------------------------------------------------------
    // Utility
    // ------------------------------------------------------------------
    void MarkDirty();

private:
    // ------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------
    bool SaveGameInternal(
        const FString& InSlotName,
        int32 InUserIndex,
        const FS_PlayerGameInfo& InPlayerData,
        int32 InSaveVersion
    );

    bool LoadGameInternal(
        const FString& InSlotName,
        int32 InUserIndex,
        FS_PlayerGameInfo& OutPlayerData,
        int32& OutSaveVersion
    );

    bool MigratePlayerSave(int32 FromVersion, int32 ToVersion, FS_PlayerGameInfo& InOutPlayerInfo);
    void SaveIfRequested(bool bSave);

private:
    // ------------------------------------------------------------------
    // Persistent settings
    // ------------------------------------------------------------------
    FString SlotName = TEXT("PlayerSave");
    int32   UserIndex = 0;
    int32   AILevel = 1;
    int32   GridMode = 0;

    // ------------------------------------------------------------------
    // Authoritative in-memory player state
    // ------------------------------------------------------------------
    FS_PlayerGameInfo PlayerInfo;

    // ------------------------------------------------------------------
    // Flags
    // ------------------------------------------------------------------
    bool bLoaded = false;
    bool bDirty = false;

    // First-run detection:
    bool bHadExistingSave = false;

    // Cached scalar values (read-through from PlayerInfo):
    static int32 CachedAILevel;
    static bool  CachedGunSightMode;

private:
    // ------------------------------------------------------------------
    // Awards (transient UI cache)
    // ------------------------------------------------------------------
    UPROPERTY(Transient)
    bool bShowAward = false;

    UPROPERTY(Transient)
    EPendingAwardType PendingAwardType = EPendingAwardType::None;

    UPROPERTY(Transient)
    int32 PendingAwardId = 0;

private:
    // Increment when the save schema changes
    static constexpr int32 CURRENT_SAVE_VERSION = 1;
};
