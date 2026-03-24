/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           PlayerCharacter.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    PlayerCharacter (Unreal)

    Runtime wrapper around the authoritative player profile stored in:
      UStarshatterPlayerSubsystem::PlayerInfo (FS_PlayerGameInfo)

    This class no longer:
      - parses player.cfg
      - writes player.cfg
      - encodes/decodes encrypted stat blobs
      - loads award tables

    Persistence is handled exclusively by UStarshatterPlayerSubsystem (SaveGame).
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"

class UObject;
class UStarshatterPlayerSubsystem;

class STARSHATTERWARS_API PlayerCharacter
{
public:
    PlayerCharacter();
    explicit PlayerCharacter(UStarshatterPlayerSubsystem* InSubsystem);

    // -----------------------------------------------------------------
    // Current player wrapper
    // -----------------------------------------------------------------
    static PlayerCharacter* GetCurrentPlayer();
    static PlayerCharacter* EnsureCurrentPlayer(UObject* WorldContext);
    static void             SetCurrentPlayer(PlayerCharacter* NewPlayer);

    // -----------------------------------------------------------------
    // Subsystem access + persistence
    // -----------------------------------------------------------------
    static UStarshatterPlayerSubsystem* GetPlayerSubsystem(UObject* WorldContext);

    // Replaces legacy PlayerCharacter::Save() that wrote player.cfg
    static bool Save(UObject* WorldContext, bool bForce = false);

    // -----------------------------------------------------------------
    // Direct FS_PlayerGameInfo access
    // -----------------------------------------------------------------
    const FS_PlayerGameInfo* GetInfoConst() const;
    FS_PlayerGameInfo* GetInfoMutable();

    // -----------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------
    int32   GetId() const;
    void    SetId(int32 InId);

    FString GetName() const;
    void    SetName(const FString& InName);

    FString GetNickname() const;
    void    SetNickname(const FString& InNickname);

    FString GetSignature() const;
    void    SetSignature(const FString& InSignature);

    // -----------------------------------------------------------------
    // Core stats / progression (examples; extend as needed)
    // -----------------------------------------------------------------
    int32 GetRankId() const;       // FS_PlayerGameInfo::Rank
    void  SetRankId(int32 RankId);

    int32 GetPlayerPoints() const;
    void  AddPoints(int32 Delta);

    int32 GetPlayerMissions() const;
    void  AddMissions(int32 Delta);

    int32 GetPlayerKills() const;
    void  AddKills(int32 Delta);

    int32 GetPlayerLosses() const;
    void  AddLosses(int32 Delta);

    // Training helpers map to FS_PlayerGameInfo helper methods
    bool HasTrainedMission(int32 MissionId1Based) const;
    void MarkTrainedMission(int32 MissionId1Based);

private:
    // Not owning: GI owns subsystem. Wrapper just references it.
    UStarshatterPlayerSubsystem* BoundSubsystem = nullptr;

private:
    static PlayerCharacter* GCurrentPlayer;
};
