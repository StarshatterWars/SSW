/*
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    FILE:           PlayerCharacter.cpp
    AUTHOR:         Carlos Bott
*/

#include "PlayerCharacter.h"

#include "AwardInfoRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Game.h"
#include "Kismet/GameplayStatics.h"
#include "SimEvent.h"
#include "StarshatterPlayerSubsystem.h"

// ------------------------------------------------------------------
// Static globals (legacy style)
// ------------------------------------------------------------------

static List<PlayerCharacter>  GPlayerRoster;
static PlayerCharacter* GSelectedPlayer = nullptr;

UObject* PlayerCharacter::GWorldContextForSave = nullptr;

// ------------------------------------------------------------------
// Ctors
// ------------------------------------------------------------------

PlayerCharacter::PlayerCharacter()
{
    ChatMacros.SetNum(10);
    for (int32 i = 0; i < ChatMacros.Num(); ++i)
        ChatMacros[i] = TEXT("");

    // legacy uses 3 MFDs in many places
    MfdModes.SetNum(3);
    for (int32 i = 0; i < MfdModes.Num(); ++i)
        MfdModes[i] = -1;
}

PlayerCharacter::PlayerCharacter(const FString& InPlayerName)
    : PlayerCharacter()
{
    PlayerName = InPlayerName;
}

PlayerCharacter::~PlayerCharacter()
{
}

// ------------------------------------------------------------------
// Accessors
// ------------------------------------------------------------------

void PlayerCharacter::SetEmpire(EEMPIRE_NAME InEmpire)
{
    if (PlayerEmpire != InEmpire)
    {
        PlayerEmpire = InEmpire;
        bDirty = true;
    }
}

void PlayerCharacter::SetCallsign(const FString& InCallsign)
{
    const FString Trimmed = InCallsign.TrimStartAndEnd();

    if (!PlayerCallsign.Equals(Trimmed, ESearchCase::CaseSensitive))
    {
        PlayerCallsign = Trimmed;
        bDirty = true;
    }
}

const FString& PlayerCharacter::ChatMacro(int32 Index) const
{
    if (ChatMacros.IsValidIndex(Index))
        return ChatMacros[Index];

    static const FString Empty = TEXT("");
    return Empty;
}

int32 PlayerCharacter::GetRank() const
{
    // If you have real rank table logic, hook it here.
    // For now: use cached value.
    return CachedRankId;
}

int32 PlayerCharacter::Medal(int32 N) const
{
    if (N < 0)
        return 0;

    // legacy enumerated medals by scanning high->low 16 bits
    for (int32 i = 0; i < 16; ++i)
    {
        const int32 Selector = 1 << (15 - i);
        if (MedalsMask & Selector)
        {
            if (N == 0)
                return Selector;
            --N;
        }
    }

    return 0;
}

// ------------------------------------------------------------------
// Awards (registry-backed presentation)
// ------------------------------------------------------------------

FString PlayerCharacter::AwardName() const
{
    if (PendingAwardId == 0)
        return FString();

    const TCHAR* T = bPendingAwardIsRank
        ? UAwardInfoRegistry::RankName(PendingAwardId)
        : UAwardInfoRegistry::MedalName(PendingAwardId);

    return FString(T ? T : TEXT(""));
}

FString PlayerCharacter::AwardDesc() const
{
    if (PendingAwardId == 0)
        return FString();

    const TCHAR* T = bPendingAwardIsRank
        ? UAwardInfoRegistry::RankDescription(PendingAwardId)
        : UAwardInfoRegistry::MedalDescription(PendingAwardId);

    return FString(T ? T : TEXT(""));
}

USound* PlayerCharacter::AwardSound() const
{
    return nullptr;
}

bool PlayerCharacter::CanCommand(int32 /*ShipClassMask*/) const
{
    return true;
}

bool PlayerCharacter::ShowAward() const
{
    return PendingAwardId != 0;
}

void PlayerCharacter::ClearShowAward()
{
    PendingAwardId = 0;
    bPendingAwardIsRank = false;
}

// ------------------------------------------------------------------
// Mutators (UE-backed)
// ------------------------------------------------------------------

void PlayerCharacter::SetName(const char* InNameAnsi)
{
    SetName(InNameAnsi ? UTF8_TO_TCHAR(InNameAnsi) : TEXT(""));
}

void PlayerCharacter::SetName(const FString& InName)
{
    const FString Trimmed = InName.TrimStartAndEnd();
    if (!Trimmed.IsEmpty())
    {
        PlayerName = Trimmed;
        bDirty = true;
    }
}

void PlayerCharacter::SetPassword(const char* InPassAnsi)
{
    SetPassword(InPassAnsi ? UTF8_TO_TCHAR(InPassAnsi) : TEXT(""));
}

void PlayerCharacter::SetPassword(const FString& InPass)
{
    PlayerPassword = InPass.Left(16);
    bDirty = true;
}

void PlayerCharacter::SetSquadron(const char* InSquadAnsi)
{
    SetSquadron(InSquadAnsi ? UTF8_TO_TCHAR(InSquadAnsi) : TEXT(""));
}

void PlayerCharacter::SetSquadron(const FString& InSquad)
{
    PlayerSquadron = InSquad;
    bDirty = true;
}

void PlayerCharacter::SetSignature(const char* InSigAnsi)
{
    SetSignature(InSigAnsi ? UTF8_TO_TCHAR(InSigAnsi) : TEXT(""));
}

void PlayerCharacter::SetSignature(const FString& InSig)
{
    PlayerSignature = InSig;
    bDirty = true;
}

void PlayerCharacter::SetChatMacro(int32 Index, const char* InMacroAnsi)
{
    SetChatMacro(Index, InMacroAnsi ? UTF8_TO_TCHAR(InMacroAnsi) : TEXT(""));
}

void PlayerCharacter::SetChatMacro(int32 Index, const FString& InMacro)
{
    if (ChatMacros.IsValidIndex(Index))
    {
        ChatMacros[Index] = InMacro;
        bDirty = true;
    }
}

void PlayerCharacter::SetCreateDate(int32 InUtcSeconds)
{
    CreateDateUtc = FMath::Max(0, InUtcSeconds);
    bDirty = true;
}

void PlayerCharacter::SetRank(int32 RankId)
{
    CachedRankId = FMath::Max(0, RankId);
    bDirty = true;
}

void PlayerCharacter::SetPoints(int32 InPoints)
{
    PlayerPoints = FMath::Max(0, InPoints);
    bDirty = true;
}

void PlayerCharacter::SetMedals(int32 InMask)
{
    MedalsMask = InMask;
    bDirty = true;
}

void PlayerCharacter::SetCampaigns(int64 InMask)
{
    CampaignCompleteMask = InMask;
    bDirty = true;
}

void PlayerCharacter::SetTrained(int32 InHighestTrainingMission)
{
    HighestTrainingMission = FMath::Max(0, InHighestTrainingMission);
    if (HighestTrainingMission > 0 && HighestTrainingMission <= 64)
        TrainingMask |= (int64(1) << (HighestTrainingMission - 1));

    bDirty = true;
}

void PlayerCharacter::SetFlightTime(int32 InSeconds)
{
    FlightTimeSeconds = FMath::Max(0, InSeconds);
    bDirty = true;
}

void PlayerCharacter::SetMissions(int32 InCount)
{
    MissionCount = FMath::Max(0, InCount);
    bDirty = true;
}

void PlayerCharacter::SetKills(int32 InCount)
{
    KillCount = FMath::Max(0, InCount);
    bDirty = true;
}

void PlayerCharacter::SetDeaths(int32 InCount)
{
    DeathCount = FMath::Max(0, InCount);
    bDirty = true;
}

void PlayerCharacter::SetLosses(int32 InCount)
{
    LossCount = FMath::Max(0, InCount);
    bDirty = true;
}

// Accumulators
void PlayerCharacter::AddFlightTime(int32 InSeconds)
{
    if (InSeconds > 0)
    {
        FlightTimeSeconds += InSeconds;
        bDirty = true;
    }
}

void PlayerCharacter::AddPoints(int32 InPoints)
{
    // NOTE: legacy typically only adds positive points
    if (InPoints != 0)
    {
        PlayerPoints = FMath::Max(0, PlayerPoints + InPoints);
        bDirty = true;
    }
}

void PlayerCharacter::AddMedal(int32 MedalBit)
{
    if (MedalBit != 0)
    {
        MedalsMask |= MedalBit;
        bDirty = true;
    }
}

void PlayerCharacter::AddMissions(int32 InCount)
{
    if (InCount > 0)
    {
        MissionCount += InCount;
        bDirty = true;
    }
}

void PlayerCharacter::AddKills(int32 InCount)
{
    if (InCount > 0)
    {
        KillCount += InCount;
        bDirty = true;
    }
}

void PlayerCharacter::AddDeaths(int32 InCount)
{
    if (InCount > 0)
    {
        DeathCount += InCount;
        bDirty = true;
    }
}

void PlayerCharacter::AddLosses(int32 InCount)
{
    if (InCount > 0)
    {
        LossCount += InCount;
        bDirty = true;
    }
}

// ------------------------------------------------------------------
// Campaign/training helpers
// ------------------------------------------------------------------

bool PlayerCharacter::HasTrained(int32 MissionId1Based) const
{
    if (MissionId1Based <= 0 || MissionId1Based > 64)
        return false;

    const int64 Bit = (int64(1) << (MissionId1Based - 1));
    return (TrainingMask & Bit) != 0;
}

bool PlayerCharacter::HasCompletedCampaign(int32 CampaignId) const
{
    if (CampaignId < 0 || CampaignId >= 64)
        return false;

    const int64 Bit = (int64(1) << CampaignId);
    return (CampaignCompleteMask & Bit) != 0;
}

void PlayerCharacter::SetCampaignComplete(int32 CampaignId)
{
    if (CampaignId < 0 || CampaignId >= 64)
        return;

    CampaignCompleteMask |= (int64(1) << CampaignId);
    bDirty = true;

    Save(); // legacy behavior
}

// ------------------------------------------------------------------
// Options
// ------------------------------------------------------------------

void PlayerCharacter::SetFlightModel(int32 InMode) { FlightModelMode = InMode; bDirty = true; }
void PlayerCharacter::SetFlyingStart(int32 InOnOff) { bFlyingStart = (InOnOff != 0); bDirty = true; }
void PlayerCharacter::SetLandingModel(int32 InMode) { LandingMode = InMode; bDirty = true; }
void PlayerCharacter::SetAILevel(int32 InLevel) { AiDifficulty = InLevel; bDirty = true; }
void PlayerCharacter::SetHUDMode(int32 InMode) { HudMode = InMode; bDirty = true; }
void PlayerCharacter::SetHUDColor(int32 InColorIndex) { HudColor = InColorIndex; bDirty = true; }
void PlayerCharacter::SetFriendlyFire(int32 InLevel) { ForceFeedbackLevel = InLevel; bDirty = true; }
void PlayerCharacter::SetGridMode(int32 InOnOff) { bGridMode = (InOnOff != 0); bDirty = true; }
void PlayerCharacter::SetGunsight(int32 InOnOff) { bGunSight = (InOnOff != 0); bDirty = true; }

// ------------------------------------------------------------------
// Legacy static rank/medal helpers (compile-safe stubs)
// ------------------------------------------------------------------

const char* PlayerCharacter::RankName(int32 RankId)
{
    switch (RankId)
    {
    case 0:  return "Conscript";
    case 1:  return "Ensign";
    case 2:  return "Lieutenant";
    case 3:  return "Commander";
    case 4:  return "Captain";
    default: return "Officer";
    }
}

const char* PlayerCharacter::RankAbrv(int32 RankId)
{
    switch (RankId)
    {
    case 0:  return "CNS";
    case 1:  return "ENS";
    case 2:  return "LT";
    case 3:  return "CDR";
    case 4:  return "CPT";
    default: return "OFF";
    }
}

int32 PlayerCharacter::RankFromName(const char* InNameAnsi)
{
    return RankFromName(InNameAnsi ? FString(UTF8_TO_TCHAR(InNameAnsi)) : FString());
}

int32 PlayerCharacter::RankFromName(const FString& InName)
{
    const FString N = InName.TrimStartAndEnd();

    if (N.Equals(TEXT("Conscript"), ESearchCase::IgnoreCase)) return 0;
    if (N.Equals(TEXT("Ensign"), ESearchCase::IgnoreCase)) return 1;
    if (N.Equals(TEXT("Lieutenant"), ESearchCase::IgnoreCase)) return 2;
    if (N.Equals(TEXT("Commander"), ESearchCase::IgnoreCase)) return 3;
    if (N.Equals(TEXT("Captain"), ESearchCase::IgnoreCase)) return 4;

    return 0;
}

const char* PlayerCharacter::RankDescription(int32 /*RankId*/) { return ""; }
const char* PlayerCharacter::MedalName(int32 /*MedalBit*/) { return ""; }
const char* PlayerCharacter::MedalDescription(int32 /*MedalBit*/) { return ""; }
int32       PlayerCharacter::CommandRankRequired(int32 /*Mask*/) { return 0; }

// ------------------------------------------------------------------
// Roster
// ------------------------------------------------------------------

List<PlayerCharacter>& PlayerCharacter::GetRoster()
{
    return GPlayerRoster;
}

PlayerCharacter* PlayerCharacter::GetCurrentPlayer()
{
    return GSelectedPlayer;
}

void PlayerCharacter::SetCurrentPlayer(PlayerCharacter* InPlayer)
{
    if (GSelectedPlayer == InPlayer)
        return;

    GSelectedPlayer = InPlayer;
}

void PlayerCharacter::SelectPlayer(PlayerCharacter* InPlayer)
{
    if (!InPlayer)
        return;

    if (GPlayerRoster.contains(InPlayer))
    {
        GSelectedPlayer = InPlayer;
        Save();
    }
}

PlayerCharacter* PlayerCharacter::Find(const char* InNameAnsi)
{
    if (!InNameAnsi || !*InNameAnsi)
        return nullptr;

    const FString Desired = UTF8_TO_TCHAR(InNameAnsi);

    for (int32 i = 0; i < GPlayerRoster.size(); ++i)
    {
        PlayerCharacter* P = GPlayerRoster.at(i);
        if (P && P->Name().Equals(Desired, ESearchCase::IgnoreCase))
            return P;
    }

    return nullptr;
}

PlayerCharacter* PlayerCharacter::Create(const char* InNameAnsi)
{
    if (!InNameAnsi || !*InNameAnsi)
        return nullptr;

    if (Find(InNameAnsi))
        return nullptr;

    PlayerCharacter* NewPlayer = new PlayerCharacter(FString(UTF8_TO_TCHAR(InNameAnsi)));
    GPlayerRoster.append(NewPlayer);
    NewPlayer->CreateUniqueID();

    return NewPlayer;
}

void PlayerCharacter::Destroy(PlayerCharacter* InPlayer)
{
    if (!InPlayer)
        return;

    GPlayerRoster.remove(InPlayer);

    if (InPlayer == GSelectedPlayer)
        GSelectedPlayer = (GPlayerRoster.size() > 0) ? GPlayerRoster.at(0) : nullptr;

    delete InPlayer;
    Save();
}

PlayerCharacter* PlayerCharacter::CreateDefault()
{
    PlayerCharacter* P = new PlayerCharacter(TEXT("NEW PILOT"));
    P->CreateUniqueID();
    return P;
}

void PlayerCharacter::AddToRoster(PlayerCharacter* InPlayer)
{
    if (!InPlayer)
        return;

    if (!GPlayerRoster.contains(InPlayer))
    {
        GPlayerRoster.append(InPlayer);
        InPlayer->CreateUniqueID();
        Save();
    }
}

void PlayerCharacter::RemoveFromRoster(PlayerCharacter* InPlayer)
{
    if (!InPlayer)
        return;

    GPlayerRoster.remove(InPlayer);

    if (InPlayer == GSelectedPlayer)
        GSelectedPlayer = (GPlayerRoster.size() > 0) ? GPlayerRoster.at(0) : nullptr;

    delete InPlayer;
    Save();
}

// ------------------------------------------------------------------
// World context + lifecycle
// ------------------------------------------------------------------

void PlayerCharacter::SetWorldContext(UObject* WorldContext)
{
    GWorldContextForSave = WorldContext;
}

void PlayerCharacter::Initialize()
{
    // Safe default: ensure there is at least one player in memory.
    if (!GSelectedPlayer)
    {
        if (!GPlayerRoster.size())
        {
            PlayerCharacter* P = CreateDefault();
            GPlayerRoster.append(P);
            GSelectedPlayer = P;
        }
        else
        {
            GSelectedPlayer = GPlayerRoster.at(0);
        }
    }
}

void PlayerCharacter::Initialize(UObject* WorldContext)
{
    SetWorldContext(WorldContext);
    Load(WorldContext);

    if (!GSelectedPlayer)
    {
        if (!GPlayerRoster.size())
        {
            PlayerCharacter* P = CreateDefault();
            GPlayerRoster.append(P);
            GSelectedPlayer = P;
            Save();
        }
        else
        {
            GSelectedPlayer = GPlayerRoster.at(0);
        }
    }
}

void PlayerCharacter::Close()
{
    for (int32 i = 0; i < GPlayerRoster.size(); ++i)
        delete GPlayerRoster.at(i);

    GPlayerRoster.destroy();
    GSelectedPlayer = nullptr;
    GWorldContextForSave = nullptr;
}

// ------------------------------------------------------------------
// Persistence (NO player.cfg)
// ------------------------------------------------------------------

void PlayerCharacter::Load()
{
    Initialize();
}

void PlayerCharacter::Load(UObject* WorldContext)
{
    if (!WorldContext)
    {
        Load();
        return;
    }

    UWorld* World = WorldContext->GetWorld();
    if (!World)
    {
        Load();
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        Load();
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
    {
        Load();
        return;
    }

    // Clear roster
    for (int32 i = 0; i < GPlayerRoster.size(); ++i)
        delete GPlayerRoster.at(i);

    GPlayerRoster.destroy();
    GSelectedPlayer = nullptr;

    // Ensure subsystem has loaded
    PlayerSS->LoadFromBoot();

    // Create one player from saved PlayerInfo (expand to multi-profile later)
    const FS_PlayerGameInfo Info = PlayerSS->GetPlayerInfoCopy();

    PlayerCharacter* P = new PlayerCharacter();
    P->FromPlayerInfo(Info);
    P->bHadSubsystemSave = PlayerSS->HadExistingSaveOnLoad();

    GPlayerRoster.append(P);
    GSelectedPlayer = P;
}

void PlayerCharacter::Save()
{
    if (!GWorldContextForSave)
        return;

    SaveToSubsystem(GWorldContextForSave);
}

bool PlayerCharacter::SaveToSubsystem(UObject* WorldContext)
{
    if (!WorldContext)
        return false;

    UWorld* World = WorldContext->GetWorld();
    if (!World)
        return false;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
        return false;

    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();
    if (!PlayerSS)
        return false;

    PlayerCharacter* P = GetCurrentPlayer();
    if (!P)
        return false;

    FS_PlayerGameInfo& Info = PlayerSS->GetMutablePlayerInfo();
    P->ToPlayerInfo(Info);

    // Persist (force because legacy often expects immediate write)
    const bool bOk = PlayerSS->SavePlayer(true);
    if (bOk)
        P->bDirty = false;

    return bOk;
}

// ------------------------------------------------------------------
// Struct conversion
// ------------------------------------------------------------------

void PlayerCharacter::FromPlayerInfo(const FS_PlayerGameInfo& InInfo)
{
    PlayerId = InInfo.Id;
    PlayerName = InInfo.Name;
    PlayerSignature = InInfo.Signature;

    // map “squadron/callsign” -> Nickname for now
    PlayerSquadron = InInfo.Squadron;
    PlayerCallsign = InInfo.Callsign;

    CachedRankId = InInfo.Rank;

    FlightModelMode = InInfo.FlightModel;
    LandingMode = InInfo.LandingMode;
    bFlyingStart = InInfo.FlyingStart;
    AiDifficulty = InInfo.AILevel;
    HudMode = InInfo.HudMode;
    HudColor = InInfo.HudColor;
    ForceFeedbackLevel = InInfo.ForceFeedbackLevel;
    bGridMode = InInfo.GridMode;
    bGunSight = InInfo.GunSightMode;

    CreateDateUtc = (int32)InInfo.CreateTime;
    PlayerPoints = InInfo.PlayerPoints;
    MedalsMask = InInfo.MedalsMask;
    FlightTimeSeconds = (int32)InInfo.FlightTime;
    MissionCount = InInfo.PlayerMissions;
    KillCount = InInfo.PlayerKills;
    DeathCount = InInfo.PlayerDeaths;
    LossCount = InInfo.PlayerLosses;

    CampaignCompleteMask = InInfo.CampaignCompleteMask;
    TrainingMask = InInfo.TrainingMask;
    HighestTrainingMission = InInfo.HighestTrainingMission;

    ChatMacros = InInfo.ChatMacros;
    if (ChatMacros.Num() != 10)
    {
        ChatMacros.SetNum(10);
        for (int32 i = 0; i < 10; ++i)
            if (ChatMacros[i].IsEmpty()) ChatMacros[i] = TEXT("");
    }

    MfdModes = InInfo.MfdModes;
    if (MfdModes.Num() != 3)
    {
        MfdModes.SetNum(3);
        for (int32 i = 0; i < 3; ++i)
            if (MfdModes[i] == 0) MfdModes[i] = -1;
    }

    bDirty = false;
}

void PlayerCharacter::ToPlayerInfo(FS_PlayerGameInfo& OutInfo) const
{
    OutInfo.Id = PlayerId;
    OutInfo.Name = PlayerName;
    OutInfo.Signature = PlayerSignature;
    OutInfo.Callsign = PlayerCallsign;
    OutInfo.Squadron = PlayerSquadron;

    OutInfo.Rank = CachedRankId;

    OutInfo.FlightModel = FlightModelMode;
    OutInfo.LandingMode = LandingMode;
    OutInfo.FlyingStart = bFlyingStart;
    OutInfo.AILevel = AiDifficulty;
    OutInfo.HudMode = HudMode;
    OutInfo.HudColor = HudColor;
    OutInfo.ForceFeedbackLevel = ForceFeedbackLevel;
    OutInfo.GridMode = bGridMode;
    OutInfo.GunSightMode = bGunSight;

    OutInfo.CreateTime = CreateDateUtc;
    OutInfo.PlayerPoints = PlayerPoints;
    OutInfo.MedalsMask = MedalsMask;
    OutInfo.FlightTime = FlightTimeSeconds;
    OutInfo.PlayerMissions = MissionCount;
    OutInfo.PlayerKills = KillCount;
    OutInfo.PlayerDeaths = DeathCount;
    OutInfo.PlayerLosses = LossCount;

    OutInfo.CampaignCompleteMask = CampaignCompleteMask;
    OutInfo.TrainingMask = TrainingMask;
    OutInfo.HighestTrainingMission = HighestTrainingMission;
    OutInfo.Trained = HighestTrainingMission; // legacy mirror

    OutInfo.ChatMacros = ChatMacros;
    OutInfo.MfdModes = MfdModes;
}

// ------------------------------------------------------------------
// Unique ID (legacy roster-based)
// ------------------------------------------------------------------

void PlayerCharacter::CreateUniqueID()
{
    int32 MaxId = 0;

    for (int32 i = 0; i < GPlayerRoster.size(); ++i)
    {
        PlayerCharacter* P = GPlayerRoster.at(i);
        if (P && P != this)
            MaxId = FMath::Max(MaxId, P->PlayerId);
    }

    if (PlayerId <= 0)
        PlayerId = MaxId + 1;

    if (PlayerId < 1)
        PlayerId = 1;
}

PlayerCharacter* PlayerCharacter::EnsureCurrentPlayer()
{
    if (GSelectedPlayer)
        return GSelectedPlayer;

    if (GPlayerRoster.size() > 0)
    {
        GSelectedPlayer = GPlayerRoster.at(0);
        return GSelectedPlayer;
    }

    PlayerCharacter* P = CreateDefault();
    GPlayerRoster.append(P);
    GSelectedPlayer = P;
    return P;
}

// ------------------------------------------------------------------
// Mission stats (legacy-ish, compile-safe)
// ------------------------------------------------------------------

int32 PlayerCharacter::GetMissionPoints(ShipStats* Stats, uint32 StartTimeMs)
{
    int32 MissionPoints = 0;

    if (Stats)
    {
        MissionPoints = Stats->GetPoints();

        const int32 FlightTimeSecondsLocal = (Game::GameTime() - StartTimeMs) / 1000;

        // If player survived the mission, award experience based on time in action
        if (!Stats->GetDeaths() && !Stats->GetColls())
        {
            int32 MinutesInAction = FlightTimeSecondsLocal / 60;
            MinutesInAction /= 10;
            MinutesInAction *= 10;

            MissionPoints += MinutesInAction;

            if (Stats->HasEvent(SimEvent::DOCK))
                MissionPoints += 100;
        }
        else
        {
            // Keep behavior: deaths/colls reduce points, never below 0
            MissionPoints -= 250;
        }

        MissionPoints = FMath::Max(0, MissionPoints);
    }

    return MissionPoints;
}

void PlayerCharacter::ProcessStats(ShipStats* Stats, uint32 StartTimeMs)
{
    if (!Stats)
        return;

    const int32 OldRankId = GetRank();
    const int32 EarnedMissionPoints = GetMissionPoints(Stats, StartTimeMs);

    AddPoints(EarnedMissionPoints);
    AddPoints(Stats->GetCommandPoints());
    AddKills(Stats->GetGunKills());
    AddKills(Stats->GetMissileKills());
    AddLosses(Stats->GetDeaths());
    AddLosses(Stats->GetColls());
    AddMissions(1);
    AddFlightTime((Game::GameTime() - StartTimeMs) / 1000);

    Rank = GetRank();

    // Promotion presentation only (no medal evaluation here yet)
    if (OldRankId != Rank)
    {
        PendingAwardId = Rank;
        bPendingAwardIsRank = true;
    }

    Save();
}

bool PlayerCharacter::EarnedAward(int32 /*AwardId*/, bool /*bIsRank*/, ShipStats* /*InShipStats*/)
{
    // Registry-driven award eligibility not implemented yet.
    return false;
}
