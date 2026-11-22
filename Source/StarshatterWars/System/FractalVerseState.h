#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameStructs_economy.h"
#include "GameStructs_traffic.h"
#include "GameStructs_insurance.h"
#include "FractalVerseState.generated.h"

USTRUCT()
struct FFractalVerseTransfer
{
    GENERATED_BODY()
    UPROPERTY() FGuid Request;
    UPROPERTY() int64 ExpectedRevision = 0;
    UPROPERTY() FName Source;
    UPROPERTY() FName Destination;
    UPROPERTY() FName Item;
    UPROPERTY() double Quantity = 0;
};

// Client intent only. Position, velocity, hull and ship identity are never accepted.
USTRUCT()
struct FFractalVerseFlightInput
{
    GENERATED_BODY()
    UPROPERTY() int64 Sequence = 0;
    UPROPERTY() FVector Rotation = FVector::ZeroVector;
    UPROPERTY() FVector Translation = FVector::ZeroVector;
    UPROPERTY() float Throttle = 0;
    UPROPERTY() bool bAugmenter = false;
    UPROPERTY() bool bPrimary = false;
};

USTRUCT()
struct FFractalVerseShipView
{
    GENERATED_BODY()
    UPROPERTY() uint32 ObjectId = 0;
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() FVector Velocity = FVector::ZeroVector;
    UPROPERTY() double Integrity = 0;
    UPROPERTY() double Throttle = 0;
};

USTRUCT()
struct FFractalVerseReceipt
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FName Principal;
    UPROPERTY(SaveGame) FString Fingerprint;
    UPROPERTY(SaveGame) int64 Revision = 0;
};

// Account records and wallet keys belong to the server, never a client save upload.
USTRUCT()
struct FFractalVersePilot
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString Callsign = TEXT("Cadet");
    UPROPERTY(SaveGame) FName HomeSystem = TEXT("Janus");
    UPROPERTY(SaveGame) FName HomeRegion = TEXT("Janek");
    UPROPERTY(SaveGame) FString CareerJson;
    UPROPERTY(SaveGame) FString LegacyDigest;
    UPROPERTY(SaveGame) FString LegacyBaseSlot;
    UPROPERTY(SaveGame) TMap<FName,FName> InventoryAliases;
    UPROPERTY(SaveGame) TMap<FName,FName> ContractAliases;

};

USTRUCT()
struct FFractalVersePilotView
{
    GENERATED_BODY()
    UPROPERTY() bool bAuthenticated = false;
    UPROPERTY() FString Callsign;
    UPROPERTY() FName HomeSystem;
    UPROPERTY() FName HomeRegion;
    UPROPERTY() int64 Credits = 0;
    UPROPERTY() int64 Revision = 0;
};

// Persistent hull identity and physical state, never supplied by a client.
USTRUCT()
struct FFractalVersePlayerShip
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FPlayerHangarShip Hull;
    UPROPERTY(SaveGame) FName System;
    UPROPERTY(SaveGame) FName Region;
    UPROPERTY(SaveGame) int32 IFF=1;
    UPROPERTY(SaveGame) FVector Position=FVector::ZeroVector;
    UPROPERTY(SaveGame) FVector Velocity=FVector::ZeroVector;
    UPROPERTY(SaveGame) FVector Heading=FVector::ForwardVector;
    UPROPERTY(SaveGame) FVector Up=FVector::UpVector;
    UPROPERTY(SaveGame) bool bDestroyed=false;
};

// World-owned data. This is deliberately not embedded in a player save.
USTRUCT()
struct FFractalVerseWorld
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) int32 Schema = 1;
    UPROPERTY(SaveGame) FGuid Universe;
    UPROPERTY(SaveGame) int64 Revision = 0;
    UPROPERTY(SaveGame) int64 UniverseBaseUtc = 0;
    UPROPERTY(SaveGame) int64 UniverseSeconds = 0;
    UPROPERTY(SaveGame) int64 LastServerUtc = 0;
    UPROPERTY(SaveGame) TMap<FName, FFractalVersePilot> Pilots;
    UPROPERTY(SaveGame) TMap<FString,FName> MigrationSources;
    UPROPERTY(SaveGame) TMap<FName,FFractalVersePlayerShip> PlayerShips;
    UPROPERTY(SaveGame) FEconomyState Economy;
    UPROPERTY(SaveGame) FGalaxyTrafficState Traffic;
    UPROPERTY(SaveGame) TMap<FGuid, FFractalVerseReceipt> Receipts;
};

USTRUCT()
struct FFractalVersePeer
{
    GENERATED_BODY()
    UPROPERTY() FString Name;
    UPROPERTY() float PingMs = 0;
};

USTRUCT()
struct FFractalVerseStatus
{
    GENERATED_BODY()
    static constexpr int32 ClientLimit = 8;
    UPROPERTY() int32 Protocol = 1;
    UPROPERTY() FGuid Universe;
    UPROPERTY() int64 Revision = 0;
    UPROPERTY() int64 UniverseBaseUtc = 0;
    UPROPERTY() int64 UniverseSeconds = 0;
    UPROPERTY() int64 ServerUtc = 0;
    UPROPERTY() int32 MaximumClients = ClientLimit;
    UPROPERTY() double UptimeSeconds = 0;
    UPROPERTY() TArray<FFractalVersePeer> Players;
    UPROPERTY() int32 ConnectedClients = 0;
    UPROPERTY() double EconomySeconds = 0;
    UPROPERTY() int32 Inventories = 0;
    UPROPERTY() int32 Journeys = 0;
    UPROPERTY() int32 Jobs = 0;
    UPROPERTY() bool bPersistenceReady = false;
};

UCLASS()
class UFractalVerseCheckpoint : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FFractalVerseWorld State;
};
