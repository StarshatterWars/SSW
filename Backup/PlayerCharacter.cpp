/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           PlayerCharacter.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    PlayerCharacter (Unreal)

    Thin runtime adapter over UStarshatterPlayerSubsystem + FS_PlayerGameInfo.
    All persistence is handled by the subsystem (SaveGame).
=============================================================================*/

#include "PlayerCharacter.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "StarshatterPlayerSubsystem.h"

PlayerCharacter* PlayerCharacter::GCurrentPlayer = nullptr;

PlayerCharacter::PlayerCharacter()
    : BoundSubsystem(nullptr)
{
}

PlayerCharacter::PlayerCharacter(UStarshatterPlayerSubsystem* InSubsystem)
    : BoundSubsystem(InSubsystem)
{
}

PlayerCharacter* PlayerCharacter::GetCurrentPlayer()
{
    return GCurrentPlayer;
}

void PlayerCharacter::SetCurrentPlayer(PlayerCharacter* NewPlayer)
{
    GCurrentPlayer = NewPlayer;
}

UStarshatterPlayerSubsystem* PlayerCharacter::GetPlayerSubsystem(UObject* WorldContext)
{
    if (!WorldContext) return nullptr;

    UWorld* World = WorldContext->GetWorld();
    if (!World) return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;

    return GI->GetSubsystem<UStarshatterPlayerSubsystem>();
}

PlayerCharacter* PlayerCharacter::EnsureCurrentPlayer(UObject* WorldContext)
{
    if (GCurrentPlayer && GCurrentPlayer->BoundSubsystem)
        return GCurrentPlayer;

    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem(WorldContext);
    if (!PlayerSS)
        return nullptr;

    // Ensure a wrapper exists bound to the subsystem:
    if (!GCurrentPlayer)
        GCurrentPlayer = new PlayerCharacter(PlayerSS);
    else
        GCurrentPlayer->BoundSubsystem = PlayerSS;

    // At this point PlayerSS has already run LoadFromBoot() in your boot flow.
    // If not, you can force-load here (optional):
    // if (!PlayerSS->HasLoaded()) PlayerSS->LoadFromBoot();

    return GCurrentPlayer;
}

bool PlayerCharacter::Save(UObject* WorldContext, bool bForce)
{
    UStarshatterPlayerSubsystem* PlayerSS = GetPlayerSubsystem(WorldContext);
    if (!PlayerSS)
        return false;

    return PlayerSS->SavePlayer(bForce);
}

const FS_PlayerGameInfo* PlayerCharacter::GetInfoConst() const
{
    return BoundSubsystem ? &BoundSubsystem->GetPlayerInfo() : nullptr;
}

FS_PlayerGameInfo* PlayerCharacter::GetInfoMutable()
{
    return BoundSubsystem ? &BoundSubsystem->GetMutablePlayerInfo() : nullptr;
}

// -----------------------------------------------------------------------------
// Identity
// -----------------------------------------------------------------------------
FString PlayerCharacter::GetName() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->Name : FString();
}

void PlayerCharacter::SetName(const FString& InName)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->Name = InName;
        BoundSubsystem->MarkDirty();
    }
}

FString PlayerCharacter::GetSignature() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->Signature : FString();
}

void PlayerCharacter::SetSignature(const FString& InSignature)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->Signature = InSignature;
        BoundSubsystem->MarkDirty();
    }
}

FString PlayerCharacter::GetNickname() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->Nickname : FString();
}

void PlayerCharacter::SetNickname(const FString& InNickname)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->Nickname = InNickname;
        BoundSubsystem->MarkDirty();
    }
}

int32 PlayerCharacter::GetId() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->Id : 0;
}

void PlayerCharacter::SetId(int32 InId)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->Id = InId;
        BoundSubsystem->MarkDirty();
    }
}

// -----------------------------------------------------------------------------
// Stats / progression
// -----------------------------------------------------------------------------
int32 PlayerCharacter::GetPlayerPoints() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->PlayerPoints : 0;
}

void PlayerCharacter::AddPoints(int32 Delta)
{
    if (Delta <= 0) return;
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->PlayerPoints += Delta;
        BoundSubsystem->MarkDirty();
    }
}

int32 PlayerCharacter::GetPlayerMissions() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->PlayerMissions : 0;
}

void PlayerCharacter::AddMissions(int32 Delta)
{
    if (Delta <= 0) return;
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->PlayerMissions += Delta;
        BoundSubsystem->MarkDirty();
    }
}

int32 PlayerCharacter::GetPlayerKills() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->PlayerKills : 0;
}

void PlayerCharacter::AddKills(int32 Delta)
{
    if (Delta <= 0) return;
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->PlayerKills += Delta;
        BoundSubsystem->MarkDirty();
    }
}

int32 PlayerCharacter::GetPlayerLosses() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->PlayerLosses : 0;
}

void PlayerCharacter::AddLosses(int32 Delta)
{
    if (Delta <= 0) return;
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->PlayerLosses += Delta;
        BoundSubsystem->MarkDirty();
    }
}

int32 PlayerCharacter::GetRankId() const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->Rank : 0;
}

void PlayerCharacter::SetRankId(int32 RankId)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->Rank = RankId;
        BoundSubsystem->MarkDirty();
    }
}

bool PlayerCharacter::HasTrainedMission(int32 MissionId1Based) const
{
    const FS_PlayerGameInfo* Info = GetInfoConst();
    return Info ? Info->HasTrainedMission(MissionId1Based) : false;
}

void PlayerCharacter::MarkTrainedMission(int32 MissionId1Based)
{
    if (FS_PlayerGameInfo* Info = GetInfoMutable())
    {
        Info->MarkTrainedMission(MissionId1Based);
        BoundSubsystem->MarkDirty();
    }
}
