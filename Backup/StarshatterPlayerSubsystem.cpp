/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterPlayerSubsystem
    FILE:           StarshatterPlayerSubsystem.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    Player profile persistence with SaveVersion and migration.

    Load behavior:
    - Records whether a save existed (bHadExistingSave)
    - Loads if present, migrates if needed
    - If absent, leaves defaults in memory and expects FirstRun to save

    Save behavior:
    - Writes UPlayerSaveGame { SaveVersion, PlayerInfo } to slot

    AWARDS (PORT NOTE)
    ==================
    Legacy award UI queried PlayerCharacter for:
      - ShowAward()
      - AwardName()
      - AwardDesc()
      - ClearShowAward()

    Unreal port routes awards through UStarshatterPlayerSubsystem:
      - Transient award state stored on subsystem:
          bShowAward / PendingAwardType / PendingAwardId
      - Title/body resolved via UAwardInfoRegistry (ranks + medals tables)
=============================================================================*/

#include "StarshatterPlayerSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include "AwardInfoRegistry.h"
#include "PlayerProgression.h"

// ------------------------------------------------------------
// Static cache defaults
// ------------------------------------------------------------

int32 UStarshatterPlayerSubsystem::CachedAILevel = 1;
bool  UStarshatterPlayerSubsystem::CachedGunSightMode = false;

// ------------------------------------------------------------
// Subsystem lifecycle
// ------------------------------------------------------------

void UStarshatterPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bLoaded = false;
    bDirty = false;
    bHadExistingSave = false;

    // Award state (transient)
    bShowAward = false;
    PendingAwardType = EPendingAwardType::None;
    PendingAwardId = 0;

    // PlayerInfo remains default-constructed (FS_PlayerGameInfo ctor).
}

void UStarshatterPlayerSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ------------------------------------------------------------
// Accessors
// ------------------------------------------------------------

UStarshatterPlayerSubsystem* UStarshatterPlayerSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
        return nullptr;

    const UWorld* World = WorldContextObject->GetWorld();
    return Get(World);
}

UStarshatterPlayerSubsystem* UStarshatterPlayerSubsystem::Get(const UWorld* World)
{
    if (!World)
        return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return nullptr;

    return GI->GetSubsystem<UStarshatterPlayerSubsystem>();
}

bool UStarshatterPlayerSubsystem::IsInitialized(const UObject* WorldContextObject)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->HasLoaded();

    return false;
}

bool UStarshatterPlayerSubsystem::IsInitialized(const UWorld* World)
{
    if (UStarshatterPlayerSubsystem* SS = Get(World))
        return SS->HasLoaded();

    return false;
}

void UStarshatterPlayerSubsystem::SetSaveSlot(const FString& InSlotName, int32 InUserIndex)
{
    SlotName = InSlotName;
    UserIndex = InUserIndex;
}

// ------------------------------------------------------------
// Boot entry point
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::LoadFromBoot()
{
    const bool bOk = LoadPlayer();
    OnPlayerSaveLoaded.Broadcast(bOk);
    return bOk;
}

bool UStarshatterPlayerSubsystem::DoesSaveExistNow() const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

// ------------------------------------------------------------
// Load / Save
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::LoadPlayer()
{
    // FIRST-RUN flag: did a save exist before we loaded?
    bHadExistingSave = UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);

    if (bHadExistingSave)
    {
        FS_PlayerGameInfo LoadedInfo;
        int32 LoadedVersion = 0;

        if (!LoadGameInternal(SlotName, UserIndex, LoadedInfo, LoadedVersion))
        {
            UE_LOG(LogTemp, Error,
                TEXT("[PlayerSubsystem] Failed to load save slot. Slot=%s UserIndex=%d"),
                *SlotName, UserIndex);

            bLoaded = false;
            return false;
        }

        // Version handling
        if (LoadedVersion < CURRENT_SAVE_VERSION)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[PlayerSubsystem] Migrating save from v%d to v%d..."),
                LoadedVersion, CURRENT_SAVE_VERSION);

            if (!MigratePlayerSave(LoadedVersion, CURRENT_SAVE_VERSION, LoadedInfo))
            {
                UE_LOG(LogTemp, Error, TEXT("[PlayerSubsystem] Migration failed."));
                bLoaded = false;
                return false;
            }

            PlayerInfo = LoadedInfo;

            // Keep any rank-side systems in sync (if you use them):
            PlayerProgression::SetCurrentRankId(PlayerInfo.Rank);

            bLoaded = true;
            bDirty = true;

            // Repair slot immediately after migration
            SavePlayer(true);

            UE_LOG(LogTemp, Log, TEXT("[PlayerSubsystem] Migration complete; slot repaired."));
        }
        else if (LoadedVersion > CURRENT_SAVE_VERSION)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("[PlayerSubsystem] SaveVersion v%d is newer than build supports (v%d). Loading anyway."),
                LoadedVersion, CURRENT_SAVE_VERSION);

            PlayerInfo = LoadedInfo;
            bLoaded = true;
            bDirty = false;
        }
        else
        {
            PlayerInfo = LoadedInfo;
            bLoaded = true;
            bDirty = false;

            // Keep rank-side systems in sync (if you use them):
            PlayerProgression::SetCurrentRankId(PlayerInfo.Rank);
        }

        // Refresh cached scalars for legacy callsites:
        CachedAILevel = (PlayerInfo.AILevel > 0) ? PlayerInfo.AILevel : 1;
        CachedGunSightMode = PlayerInfo.GunSightMode;

        UE_LOG(LogTemp, Log,
            TEXT("[PlayerSubsystem] Player loaded: %s (SaveVersion=%d)"),
            *PlayerInfo.Name, FMath::Max(LoadedVersion, CURRENT_SAVE_VERSION));

        return true;
    }

    // No save exists: keep default-constructed PlayerInfo in memory.
    // DO NOT auto-create a save here; FirstRun is expected to do that.
    bLoaded = true;
    bDirty = true;

    CachedAILevel = (PlayerInfo.AILevel > 0) ? PlayerInfo.AILevel : 1;
    CachedGunSightMode = PlayerInfo.GunSightMode;

    UE_LOG(LogTemp, Log, TEXT("[PlayerSubsystem] No player save found. FirstRun required."));
    return true;
}

bool UStarshatterPlayerSubsystem::SavePlayer(bool bForce)
{
    if (!bForce && !bDirty)
        return true;

    const bool bOk = SaveGameInternal(SlotName, UserIndex, PlayerInfo, CURRENT_SAVE_VERSION);
    if (bOk)
        bDirty = false;

    return bOk;
}

const FS_PlayerGameInfo& UStarshatterPlayerSubsystem::GetPlayerInfo() const
{
    return PlayerInfo;
}

FS_PlayerGameInfo& UStarshatterPlayerSubsystem::GetMutablePlayerInfo()
{
    bDirty = true;
    return PlayerInfo;
}

bool UStarshatterPlayerSubsystem::SaveGameInternal(
    const FString& InSlotName,
    int32 InUserIndex,
    const FS_PlayerGameInfo& InPlayerData,
    int32 InSaveVersion)
{
    UPlayerSaveGame* SaveInstance =
        Cast<UPlayerSaveGame>(UGameplayStatics::CreateSaveGameObject(UPlayerSaveGame::StaticClass()));

    if (!SaveInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[PlayerSubsystem] CreateSaveGameObject failed."));
        return false;
    }

    SaveInstance->SaveVersion = InSaveVersion;
    SaveInstance->PlayerInfo = InPlayerData;

    const bool bOk = UGameplayStatics::SaveGameToSlot(SaveInstance, InSlotName, InUserIndex);
    if (!bOk)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlayerSubsystem] SaveGameToSlot failed. Slot=%s UserIndex=%d"),
            *InSlotName, InUserIndex);
    }

    return bOk;
}

bool UStarshatterPlayerSubsystem::LoadGameInternal(
    const FString& InSlotName,
    int32 InUserIndex,
    FS_PlayerGameInfo& OutPlayerData,
    int32& OutSaveVersion)
{
    UPlayerSaveGame* LoadedGame =
        Cast<UPlayerSaveGame>(UGameplayStatics::LoadGameFromSlot(InSlotName, InUserIndex));

    if (!LoadedGame)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[PlayerSubsystem] LoadGameFromSlot returned null/wrong class. Slot=%s UserIndex=%d"),
            *InSlotName, InUserIndex);
        return false;
    }

    OutSaveVersion = LoadedGame->SaveVersion;
    OutPlayerData = LoadedGame->PlayerInfo;
    return true;
}

bool UStarshatterPlayerSubsystem::MigratePlayerSave(int32 FromVersion, int32 ToVersion, FS_PlayerGameInfo& InOutPlayerInfo)
{
    // Stepwise migration framework
    if (FromVersion <= 0)
        FromVersion = 1;

    while (FromVersion < ToVersion)
    {
        switch (FromVersion)
        {
        case 1:
            // v1 -> v2 example (not active yet)
            // Implement transforms once CURRENT_SAVE_VERSION becomes 2.
            FromVersion = 2;
            break;

        default:
            UE_LOG(LogTemp, Error,
                TEXT("[PlayerSubsystem] No migration path from v%d to v%d."),
                FromVersion, ToVersion);
            return false;
        }
    }

    return true;
}

void UStarshatterPlayerSubsystem::MarkDirty()
{
    bDirty = true;
}

// ------------------------------------------------------------
// Safe getters
// ------------------------------------------------------------

int32 UStarshatterPlayerSubsystem::GetPlayerIdSafe(const UObject* WorldContextObject, int32 DefaultId)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->GetPlayerInfo().Id;

    return DefaultId;
}

int32 UStarshatterPlayerSubsystem::GetRankIdSafe(const UObject* WorldContextObject, int32 DefaultRankId)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->GetPlayerInfo().Rank;

    return DefaultRankId;
}

FString UStarshatterPlayerSubsystem::GetPlayerNameSafe(const UObject* WorldContextObject)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->GetPlayerInfo().Name;

    return FString();
}

FString UStarshatterPlayerSubsystem::GetRankNameSafe(const UObject* WorldContextObject, int32 RankId)
{
    (void)WorldContextObject;
    return UAwardInfoRegistry::RankName(RankId);
}

FString UStarshatterPlayerSubsystem::GetRankDescSafe(const UObject* WorldContextObject, int32 RankId)
{
    (void)WorldContextObject;
    return UAwardInfoRegistry::RankDescription(RankId);
}

// ------------------------------------------------------------
// Training helpers
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::HasTrained(int32 TrainingMissionId) const
{
    // Treat TrainingMissionId as 1-based mission id (legacy-style).
    if (TrainingMissionId <= 0 || TrainingMissionId > 64)
        return false;

    const int32 BitIndex = TrainingMissionId - 1;
    const int64 Bit = (1ll << BitIndex);
    return (PlayerInfo.TrainingMask & Bit) != 0;
}

void UStarshatterPlayerSubsystem::SetTrained(int32 TrainingMissionId, bool bTrained)
{
    if (TrainingMissionId <= 0 || TrainingMissionId > 64)
        return;

    const int32 BitIndex = TrainingMissionId - 1;
    const int64 Bit = (1ll << BitIndex);

    if (bTrained)
    {
        PlayerInfo.TrainingMask |= Bit;
        PlayerInfo.HighestTrainingMission = FMath::Max(PlayerInfo.HighestTrainingMission, TrainingMissionId);
        PlayerInfo.Trained = FMath::Max(PlayerInfo.Trained, TrainingMissionId);
    }
    else
    {
        PlayerInfo.TrainingMask &= ~Bit;
        // Intentionally do not recompute HighestTrainingMission here.
    }

    bDirty = true;
}

bool UStarshatterPlayerSubsystem::HasTrainedSafe(const UObject* WorldContextObject, int32 TrainingMissionId)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->HasTrained(TrainingMissionId);

    return false;
}

// Legacy wrappers:
bool UStarshatterPlayerSubsystem::HasTrainedMission(const UObject* WorldContextObject, int32 MissionId)
{
    return HasTrainedSafe(WorldContextObject, MissionId);
}

void UStarshatterPlayerSubsystem::MarkTrainedMission(const UObject* WorldContextObject, int32 MissionId)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
    {
        SS->SetTrained(MissionId, true);
        SS->SavePlayer(false);
    }
}

// ------------------------------------------------------------
// Campaign completion (bit index 0..63)
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::HasCompletedCampaign(int32 CampaignBitIndex) const
{
    if (CampaignBitIndex < 0 || CampaignBitIndex >= 64)
        return false;

    const int64 Bit = (1ll << CampaignBitIndex);
    return (PlayerInfo.CampaignCompleteMask & Bit) != 0;
}

void UStarshatterPlayerSubsystem::SetCampaignComplete(int32 CampaignBitIndex, bool bComplete)
{
    if (CampaignBitIndex < 0 || CampaignBitIndex >= 64)
        return;

    const int64 Bit = (1ll << CampaignBitIndex);

    if (bComplete) PlayerInfo.CampaignCompleteMask |= Bit;
    else           PlayerInfo.CampaignCompleteMask &= ~Bit;

    bDirty = true;
}

bool UStarshatterPlayerSubsystem::HasCompletedCampaignSafe(const UObject* WorldContextObject, int32 CampaignBitIndex)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->HasCompletedCampaign(CampaignBitIndex);

    return false;
}

void UStarshatterPlayerSubsystem::SetCampaignCompleteSafe(
    const UObject* WorldContextObject,
    int32 CampaignBitIndex,
    bool bComplete,
    bool bSave)
{
    UStarshatterPlayerSubsystem* SS = Get(WorldContextObject);
    if (!SS)
        return;

    SS->SetCampaignComplete(CampaignBitIndex, bComplete);

    if (bSave)
        SS->SavePlayer(true);
}

// Legacy wrappers:
bool UStarshatterPlayerSubsystem::IsCampaignComplete(const UObject* WorldContextObject, int32 CampaignIndex0Based)
{
    return HasCompletedCampaignSafe(WorldContextObject, CampaignIndex0Based);
}

void UStarshatterPlayerSubsystem::SetCampaignComplete(const UObject* WorldContextObject, int32 CampaignIndex0Based, bool bComplete)
{
    SetCampaignCompleteSafe(WorldContextObject, CampaignIndex0Based, bComplete, true);
}

// ------------------------------------------------------------
// Command eligibility
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::CanCommand(int32 CmdClass) const
{
    const int32 RankId = PlayerInfo.Rank;
    const FRankInfo* RankRow = UAwardInfoRegistry::FindRank(RankId);
    if (!RankRow)
        return false;

    const int32 Mask = RankRow->GrantedShipClasses;

    if (CmdClass < 0 || CmdClass >= 31)
        return false;

    const int32 Bit = (1 << CmdClass);
    return (Mask & Bit) != 0;
}

bool UStarshatterPlayerSubsystem::CanCommandSafe(const UObject* WorldContextObject, int32 CmdClass)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->CanCommand(CmdClass);

    return false;
}

// ------------------------------------------------------------
// Legacy-style wrappers (kept for callsite drop-in)
// ------------------------------------------------------------

int32 UStarshatterPlayerSubsystem::GetPlayerId(const UObject* WorldContextObject)
{
    return GetPlayerIdSafe(WorldContextObject, 0);
}

FString UStarshatterPlayerSubsystem::GetPlayerName(const UObject* WorldContextObject)
{
    return GetPlayerNameSafe(WorldContextObject);
}

int32 UStarshatterPlayerSubsystem::GetPlayerRankId(const UObject* WorldContextObject)
{
    return GetRankIdSafe(WorldContextObject, 0);
}

// ------------------------------------------------------------
// Cached scalar helpers
// ------------------------------------------------------------

int32 UStarshatterPlayerSubsystem::GetCachedAILevel(int32 DefaultValue)
{
    return (CachedAILevel > 0) ? CachedAILevel : DefaultValue;
}

bool UStarshatterPlayerSubsystem::GetCachedGunSightMode(bool DefaultValue)
{
    // CachedGunSightMode is updated on LoadPlayer() and should remain stable.
    // If you need "default until loaded", add an explicit init flag.
    (void)DefaultValue;
    return CachedGunSightMode;
}

// ------------------------------------------------------------
// Awards (instance)
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::GetShowAward() const
{
    return bShowAward;
}

int32 UStarshatterPlayerSubsystem::GetPendingAwardType() const
{
    return static_cast<int32>(PendingAwardType);
}

EPendingAwardType UStarshatterPlayerSubsystem::GetPendingAwardTypeEnum() const
{
    return PendingAwardType;
}

int32 UStarshatterPlayerSubsystem::GetPendingAwardId() const
{
    return PendingAwardId;
}

FString UStarshatterPlayerSubsystem::GetCachedAwardTitle() const
{
    if (!bShowAward || PendingAwardType == EPendingAwardType::None || PendingAwardId <= 0)
        return FString();

    switch (PendingAwardType)
    {
    case EPendingAwardType::Rank:  return UAwardInfoRegistry::RankName(PendingAwardId);
    case EPendingAwardType::Medal: return UAwardInfoRegistry::MedalName(PendingAwardId);
    default: break;
    }

    return FString();
}

FString UStarshatterPlayerSubsystem::GetCachedAwardBody() const
{
    if (!bShowAward || PendingAwardType == EPendingAwardType::None || PendingAwardId <= 0)
        return FString();

    switch (PendingAwardType)
    {
    case EPendingAwardType::Rank:
    {
        const FString AwardText = UAwardInfoRegistry::RankAwardText(PendingAwardId);
        return !AwardText.IsEmpty() ? AwardText : UAwardInfoRegistry::RankDescription(PendingAwardId);
    }
    case EPendingAwardType::Medal:
    {
        const FString AwardText = UAwardInfoRegistry::MedalAwardText(PendingAwardId);
        return !AwardText.IsEmpty() ? AwardText : UAwardInfoRegistry::MedalDescription(PendingAwardId);
    }
    default:
        break;
    }

    return FString();
}

void UStarshatterPlayerSubsystem::SetPendingAward(
    const UObject* WorldContextObject,
    EPendingAwardType InType,
    int32 InId,
    bool bShow,
    bool bSave)
{
    UStarshatterPlayerSubsystem* SS = Get(WorldContextObject);
    if (!SS)
        return;

    SS->PendingAwardType = InType;
    SS->PendingAwardId = InId;
    SS->bShowAward = bShow;

    SS->bDirty = true;

    if (bSave)
    {
        SS->SavePlayer(false);
    }
}

void UStarshatterPlayerSubsystem::ClearPendingAward(bool bSave)
{
    if (!bShowAward && PendingAwardType == EPendingAwardType::None && PendingAwardId == 0)
        return;

    bShowAward = false;
    PendingAwardType = EPendingAwardType::None;
    PendingAwardId = 0;

    bDirty = true;
    SaveIfRequested(bSave);
}

void UStarshatterPlayerSubsystem::SaveIfRequested(bool bSave)
{
    if (bSave)
        SavePlayer(false);
}

// ------------------------------------------------------------
// Awards (static wrappers for UI callsites)
// ------------------------------------------------------------

bool UStarshatterPlayerSubsystem::GetCachedShowAward(const UObject* WorldContextObject, bool DefaultValue)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        return SS->GetShowAward();

    return DefaultValue;
}

FString UStarshatterPlayerSubsystem::GetCachedAwardTitle(const UObject* WorldContextObject, const FString& DefaultValue)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
    {
        const FString Title = SS->GetCachedAwardTitle();
        return Title.IsEmpty() ? DefaultValue : Title;
    }

    return DefaultValue;
}

FString UStarshatterPlayerSubsystem::GetCachedAwardBody(const UObject* WorldContextObject, const FString& DefaultValue)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
    {
        const FString Body = SS->GetCachedAwardBody();
        return Body.IsEmpty() ? DefaultValue : Body;
    }

    return DefaultValue;
}

void UStarshatterPlayerSubsystem::ClearPendingAwardSafe(const UObject* WorldContextObject, bool bSave)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        SS->ClearPendingAward(bSave);
}

void UStarshatterPlayerSubsystem::ClearPendingAward(const UObject* WorldContextObject, bool bSave)
{
    if (UStarshatterPlayerSubsystem* SS = Get(WorldContextObject))
        SS->ClearPendingAward(bSave);
}

int32 UStarshatterPlayerSubsystem::GetAILevel() const
{
    return PlayerInfo.AILevel;
}

void UStarshatterPlayerSubsystem::ProcessStats(ShipStats* Stats, uint32 StartTimeMs)
{
    if (!Stats)
        return;
}

void UStarshatterPlayerSubsystem::SetTrained(int32 TrainingIdentity)
{
    // TODO: Store in PlayerSave (e.g., a set/bitfield of completed trainings)
}

void UStarshatterPlayerSubsystem::SavePlayer()
{
}

void UStarshatterPlayerSubsystem::SetAILevel(int32 InLevel)
{
    AILevel = InLevel;
}

int32 UStarshatterPlayerSubsystem::GetGridMode() const
{
    return GridMode;
}

void UStarshatterPlayerSubsystem::SetGridMode(int32 InMode)
{
    GridMode = InMode;
}

