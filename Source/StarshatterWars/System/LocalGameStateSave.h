#pragma once
#include "GameFramework/SaveGame.h"
#include "GameStructs.h"
#include "LocalGameStateSave.generated.h"
USTRUCT()
struct FLocalNavState
{
 GENERATED_BODY()
 UPROPERTY(SaveGame) FString Region;
 UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
 UPROPERTY(SaveGame) FString Target;
 UPROPERTY(SaveGame) int32 Action=0;
 UPROPERTY(SaveGame) int32 Status=0;
 UPROPERTY(SaveGame) int32 Formation=0;
 UPROPERTY(SaveGame) int32 Priority=0;
 UPROPERTY(SaveGame) int32 Speed=0;
 UPROPERTY(SaveGame) int32 Emcon=0;
 UPROPERTY(SaveGame) int32 WeaponsFree=0;
 UPROPERTY(SaveGame) int32 Farcast=0;
 UPROPERTY(SaveGame) double Hold=0;
};
USTRUCT()
struct FLocalShipState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString Name;
    UPROPERTY(SaveGame) FString Design;
    UPROPERTY(SaveGame) FString Region;
    UPROPERTY(SaveGame) FString System;
    UPROPERTY(SaveGame) FString Element;
    UPROPERTY(SaveGame) FString Carrier;
    UPROPERTY(SaveGame) FGuid CargoId;
    UPROPERTY(SaveGame) int32 IFF=0;
    UPROPERTY(SaveGame) int32 Deck=0;
    UPROPERTY(SaveGame) int32 FlightPhase=0;
    UPROPERTY(SaveGame) bool bDestroyed=false;
    UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
    UPROPERTY(SaveGame) FVector Velocity=FVector::ZeroVector;
    UPROPERTY(SaveGame) FVector Right=FVector::RightVector;
    UPROPERTY(SaveGame) FVector Up=FVector::UpVector;
    UPROPERTY(SaveGame) FVector Forward=FVector::ForwardVector;
    UPROPERTY(SaveGame) double Throttle=0;
    UPROPERTY(SaveGame) int32 Shield=0;
    UPROPERTY(SaveGame) FPlayerHangarShip Condition;
    UPROPERTY(SaveGame) TArray<int32> Ammo;
    UPROPERTY(SaveGame) bool bMagazineIssue = false;
    UPROPERTY(SaveGame) TArray<double> Fuel;
    UPROPERTY(SaveGame) TArray<int32> Navigation;
    UPROPERTY(SaveGame) TArray<FLocalNavState> FlightPlan;
    UPROPERTY(SaveGame) bool bHasFlightPlan=false;
    UPROPERTY(SaveGame) TArray<int32> Objectives;
};
USTRUCT()
struct FLocalAsteroidState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString Region;
    UPROPERTY(SaveGame) int32 Index=0;
    UPROPERTY(SaveGame) int32 ContactId=0;
    UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
    UPROPERTY(SaveGame) FSSWAsteroidResource Resource;
    UPROPERTY(SaveGame) double InitialTonnes=0;
    UPROPERTY(SaveGame) double Integrity=0;
};
USTRUCT()
struct FLocalMiningRunState
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString Ship;
    UPROPERTY(SaveGame) double Time=0;
    UPROPERTY(SaveGame) double NextPulse=0;
    UPROPERTY(SaveGame) bool bDelivering=false;
    UPROPERTY(SaveGame) double LoadedTonnes=0;
    UPROPERTY(SaveGame) FGuid Reservation;
    UPROPERTY(SaveGame) int32 DepositId=INDEX_NONE;
};
UCLASS()
class ULocalStateArea : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=1;
    UPROPERTY(SaveGame) FGuid Generation;
    UPROPERTY(SaveGame) FString ProfileSlot;
};
UCLASS()
class ULocalPilotState : public ULocalStateArea
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FS_PlayerGameInfo Pilot;
};
UCLASS()
class ULocalEconomyState : public ULocalStateArea
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FEconomyState Economy;
};
UCLASS()
class ULocalSkillState : public ULocalStateArea
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) TMap<FName,FSkillProgress> Skills;
    UPROPERTY(SaveGame) TArray<FSkillTrainingEntry> Queue;
};
UCLASS()
class ULocalUniverseState : public ULocalStateArea
{
    GENERATED_BODY()
public:
    // Flight-plan positions are authored X/Y horizontal, Z altitude.
    // Zero identifies checkpoints written before the coordinate-restore fix.
    UPROPERTY(SaveGame) int32 NavigationCoordinatesVersion=0;
    UPROPERTY(SaveGame) int32 MissionId=INDEX_NONE;
    UPROPERTY(SaveGame) int32 AgentAssignmentId=INDEX_NONE;
    UPROPERTY(SaveGame) FString MissionName;
    UPROPERTY(SaveGame) bool bMissionComplete=false;
    UPROPERTY(SaveGame) double MissionElapsed=0;
    UPROPERTY(SaveGame) TArray<FLocalShipState> Ships;
    UPROPERTY(SaveGame) TArray<FLocalAsteroidState> Asteroids;
    UPROPERTY(SaveGame) TArray<int32> EventIds;
    UPROPERTY(SaveGame) TArray<int32> EventStatuses;
    UPROPERTY(SaveGame) TArray<FLocalMiningRunState> MiningRuns;
};
UCLASS()
class ULocalStateManifest : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 Version=1;
    UPROPERTY(SaveGame) FGuid Generation;
    UPROPERTY(SaveGame) FString ProfileSlot;
    UPROPERTY(SaveGame) int64 PlayerRevision=0;
    UPROPERTY(SaveGame) int64 SavedUtc=0;
    UPROPERTY(SaveGame) TArray<FString> Slots;
};
class FLocalWorldStateCodec
{
public:
    static void Capture(class Sim&,ULocalUniverseState&);
    static bool Restore(class Sim&,const ULocalUniverseState&);
};
