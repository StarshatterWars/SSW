/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      Player / Logbook (Legacy-compatible facade)
    FILE:           PlayerCharacter.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    PlayerCharacter is a legacy-compatible non-UObject façade that:
    - Keeps the legacy static roster using List<>
    - Stores player state using UE types exclusively
    - Persists via UStarshatterPlayerSubsystem (no player.cfg)

    NOTES / FIXES APPLIED
    =====================
    - Removed legacy AwardInfo dependency from this façade (AwardInfo is separate now).
    - AwardName/AwardDesc return FString (registry returns TCHAR*).
    - Signature cleanup: GetMissionPoints returns int32 consistently.
    - EarnedAward signature matches header/cpp and no longer depends on AwardInfo*.
    - WorldContext save pointer remains, but all access is null-safe.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"

#include "List.h"        // legacy container (kept per request)
#include "GameStructs.h" // FS_PlayerGameInfo, etc.

class ShipStats; // legacy forward
class USound;    // UE forward
class UObject;

// +-------------------------------------------------------------------+

class PlayerCharacter
{
public:
    static const char* TYPENAME() { return "PlayerCharacter"; }

    PlayerCharacter();
    explicit PlayerCharacter(const FString& InPlayerName);
    virtual ~PlayerCharacter();

    // Legacy equality operator (name compare)
    int operator==(const PlayerCharacter& Other) const
    {
        return PlayerName.Equals(Other.PlayerName, ESearchCase::IgnoreCase);
    }

    // ------------------------------------------------------------------
    // Identity / strings (UE types)
    // ------------------------------------------------------------------
    int32 GetIdentity() const { return PlayerId; }
    int32 Identity()    const { return PlayerId; } // legacy alias

    const FString& Name()      const { return PlayerName; }
    const FString& Password()  const { return PlayerPassword; }
    const FString& Squadron()  const { return PlayerSquadron; }
    const FString& Signature() const { return PlayerSignature; }

    void SetEmpire(EEMPIRE_NAME InEmpire);
    EEMPIRE_NAME GetEmpire() const { return PlayerEmpire; };

    void SetCallsign(const FString& InCallsign);
    const FString& GetCallsign() const { return PlayerCallsign; };

    const FString& ChatMacro(int32 Index) const;

    int32 CreateDate() const { return CreateDateUtc; }

    // ------------------------------------------------------------------
    // Rank / medals / points (legacy-compatible)
    // ------------------------------------------------------------------
    int32 GetRank() const;      // derived or cached
    int32 Medal(int32 N) const; // nth medal bit in 16-bit field
    int32 Points() const { return PlayerPoints; }
    int32 Medals() const { return MedalsMask; }

    // Stats
    int32 FlightTime() const { return FlightTimeSeconds; }
    int32 Missions()   const { return MissionCount; }
    int32 Kills()      const { return KillCount; }
    int32 Deaths()     const { return DeathCount; }
    int32 Losses()     const { return LossCount; }

    // Campaign/training (modernized storage)
    int64 Campaigns() const { return CampaignCompleteMask; }
    int32 Trained()   const { return HighestTrainingMission; }

    // ------------------------------------------------------------------
    // Gameplay options (legacy names kept, UE-backed)
    // ------------------------------------------------------------------
    int32 FlightModel()  const { return FlightModelMode; }
    int32 FlyingStart()  const { return bFlyingStart ? 1 : 0; }
    int32 LandingModel() const { return LandingMode; }
    int32 AILevel()      const { return AiDifficulty; }
    int32 HUDMode()      const { return HudMode; }
    int32 HUDColor()     const { return HudColor; }
    int32 FriendlyFire() const { return ForceFeedbackLevel; }
    int32 GridMode()     const { return bGridMode ? 1 : 0; }
    int32 Gunsight()     const { return bGunSight ? 1 : 0; }

    // ------------------------------------------------------------------
    // Awards (registry-backed presentation)
    // ------------------------------------------------------------------
    bool    ShowAward() const;
    FString AwardName() const;
    FString AwardDesc() const;
    USound* AwardSound() const;

    bool CanCommand(int32 ShipClassMask) const;

    void ClearShowAward();

    // ------------------------------------------------------------------
    // Mutators (keep legacy signatures but UE implementation)
    // ------------------------------------------------------------------
    void SetName(const char* InNameAnsi);
    void SetName(const FString& InName);

    void SetPassword(const char* InPassAnsi);
    void SetPassword(const FString& InPass);

    void SetSquadron(const char* InSquadAnsi);
    void SetSquadron(const FString& InSquad);

    void SetSignature(const char* InSigAnsi);
    void SetSignature(const FString& InSig);

    void SetChatMacro(int32 Index, const char* InMacroAnsi);
    void SetChatMacro(int32 Index, const FString& InMacro);

    void SetCreateDate(int32 InUtcSeconds);
    void SetRank(int32 RankId);
    void SetPoints(int32 InPoints);
    void SetMedals(int32 InMask);

    void SetCampaigns(int64 InMask);
    void SetTrained(int32 InHighestTrainingMission);

    void SetFlightTime(int32 InSeconds);
    void SetMissions(int32 InCount);
    void SetKills(int32 InCount);
    void SetDeaths(int32 InCount);
    void SetLosses(int32 InCount);

    // Accumulators
    void AddFlightTime(int32 InSeconds);
    void AddPoints(int32 InPoints);
    void AddMedal(int32 MedalBit);
    void AddMissions(int32 InCount);
    void AddKills(int32 InCount);
    void AddDeaths(int32 InCount);
    void AddLosses(int32 InCount);

    // Campaign/training helpers (legacy names retained)
    bool HasTrained(int32 MissionId1Based) const;
    bool HasCompletedCampaign(int32 CampaignId) const;
    void SetCampaignComplete(int32 CampaignId);

    // Options
    void SetFlightModel(int32 InMode);
    void SetFlyingStart(int32 InOnOff);
    void SetLandingModel(int32 InMode);
    void SetAILevel(int32 InLevel);
    void SetHUDMode(int32 InMode);
    void SetHUDColor(int32 InColorIndex);
    void SetFriendlyFire(int32 InLevel);
    void SetGridMode(int32 InOnOff);
    void SetGunsight(int32 InOnOff);

    // ------------------------------------------------------------------
    // Legacy static rank/medal helpers (stubs but compile-safe)
    // ------------------------------------------------------------------
    static const char* RankName(int32 RankId);
    static const char* RankAbrv(int32 RankId);
    static int32       RankFromName(const char* InNameAnsi);
    static int32       RankFromName(const FString& InName);

    static const char* RankDescription(int32 RankId);
    static const char* MedalName(int32 MedalBit);
    static const char* MedalDescription(int32 MedalBit);

    static int32 CommandRankRequired(int32 ShipClassMask);

    // ------------------------------------------------------------------
    // Roster + persistence (List<> kept)
    // ------------------------------------------------------------------
    static List<PlayerCharacter>& GetRoster();
    static PlayerCharacter* GetCurrentPlayer();
    static void                   SelectPlayer(PlayerCharacter* InPlayer);
    static void                   SetCurrentPlayer(PlayerCharacter* InPlayer);
    static PlayerCharacter* EnsureCurrentPlayer();

    static PlayerCharacter* Create(const char* InNameAnsi);
    static void                   Destroy(PlayerCharacter* InPlayer);
    static PlayerCharacter* Find(const char* InNameAnsi);
    static void                   Initialize();                 // safe default
    static void                   Initialize(UObject* WorldContext);
    static void                   Close();

    bool                          ConfigExists() const { return bHadSubsystemSave; }

    static void                   Load();                       // no-op safe
    static void                   Load(UObject* WorldContext);  // loads from subsystem
    static void                   Save();                       // saves to subsystem if context set
    static bool                   SaveToSubsystem(UObject* WorldContext);

    static void                   SetWorldContext(UObject* WorldContext);

    static PlayerCharacter* CreateDefault();
    static void                   AddToRoster(PlayerCharacter* InPlayer);
    static void                   RemoveFromRoster(PlayerCharacter* InPlayer);

    // ------------------------------------------------------------------
    // Conversion to/from subsystem struct
    // ------------------------------------------------------------------
    void FromPlayerInfo(const FS_PlayerGameInfo& InInfo);
    void ToPlayerInfo(FS_PlayerGameInfo& OutInfo) const;

    int32 GetMissionPoints(ShipStats* Stats, uint32 StartTimeMs);
    void  ProcessStats(ShipStats* Stats, uint32 StartTimeMs);

    // Registry-driven medal/rank eligibility (placeholder)
    bool  EarnedAward(int32 AwardId, bool bIsRank, ShipStats* InShipStats);

private:
    void CreateUniqueID();

private:
    // UE identity/profile
    int32   PlayerId = 0;
    EEMPIRE_NAME   PlayerEmpire = EEMPIRE_NAME::Terellian;
    FString PlayerName;
    FString PlayerCallsign;
    FString PlayerPassword;
    FString PlayerSquadron;
    FString PlayerSignature;

    // Chat macros (10)
    TArray<FString> ChatMacros;

    // MFD modes (keep numeric for legacy UI until fully ported)
    TArray<int32> MfdModes;

    // Stats
    int32 CreateDateUtc = 0;
    int32 PlayerPoints = 0;
    int32 MedalsMask = 0; // bitmask
    int32 FlightTimeSeconds = 0;
    int32 MissionCount = 0;
    int32 KillCount = 0;
    int32 DeathCount = 0;
    int32 LossCount = 0;

    // Campaign/training modern storage
    int64 CampaignCompleteMask = 0; // up to 64 campaigns
    int64 TrainingMask = 0;         // up to 64 trainings
    int32 HighestTrainingMission = 0;

    // Cached/explicit rank id (optional)
    int32 CachedRankId = 0;

    // Gameplay options
    int32 FlightModelMode = 0;
    bool  bFlyingStart = false;
    int32 LandingMode = 0;
    int32 AiDifficulty = 1;
    int32 HudMode = 0;
    int32 HudColor = 1;
    int32 ForceFeedbackLevel = 4;
    bool  bGridMode = true;
    bool  bGunSight = false;

    // Dirty tracking
    bool bDirty = false;

    // Presentation-only rank tracking (legacy field used by some call sites)
    int32 Rank = 0;

    // Pending award presentation
    int32 PendingAwardId = 0;
    bool  bPendingAwardIsRank = false;

    // “ConfigExists” legacy behavior now means “had a SaveGame on load”
    bool bHadSubsystemSave = false;

private:
    // Shared static world context for Save()/Load() convenience
    static UObject* GWorldContextForSave;
};
