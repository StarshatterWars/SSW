/*=============================================================================
	Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025–2026.

	SUBSYSTEM:    StarshatterShipDesignSubsystem (Unreal Engine)
	FILE:         StarshatterShipDesignSubsystem.h / .cpp
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	Authoritative ship design ingestion and normalization subsystem.

	This subsystem is responsible for:
	  - Parsing legacy Starshatter SHIP .def files
	  - Normalizing ship systems into Unreal-native data structures
	  - Resolving legacy semantics (scale, EMCON, decoy/probe, muzzle spaces)
	  - Writing fully-resolved FShipDesign rows into a UDataTable

	The subsystem is intentionally WRITE-ON-INGEST:
	  - It runs only when ship data is added, modified, or re-imported
	  - The resulting DataTable is the runtime source of truth
	  - No ship design parsing occurs during normal gameplay

	DESIGN PHILOSOPHY
	=================
	This class preserves legacy Starshatter behavior exactly while
	decoupling parsing from runtime simulation.

	Ship systems are parsed as-authored (power, drives, weapons, shields,
	flight decks, etc.) and stored without enforcing modularity or slotting.
	Future refactors may introduce component-based or slot-driven designs,
	but this subsystem intentionally mirrors the legacy data model to ensure
	correctness and backward compatibility.

	IMPORTANT CONSTRAINTS
	=====================
	  - This subsystem MUST NOT allocate or spawn runtime simulation objects
	  - This subsystem MUST NOT mutate ship designs after ingestion
	  - All validation and fix-ups occur at import time only
	  - Runtime code consumes FShipDesign data verbatim

	LEGACY COMPATIBILITY
	====================
	The parser supports:
	  - Legacy SHIP file syntax and semantics
	  - Backward-compatible explosion/death spiral definitions
	  - Decoy and probe weapon resolution by WeaponType
	  - Implicit system defaults used by original Starshatter

	Any behavior that differs from legacy must be treated as a bug.

=============================================================================*/


#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "DataLoader.h"
#include "ParseUtil.h"
#include "Random.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"
#include "List.h"
#include "GameLoader.h"

#include "GameStructs_System.h"
#include "GameStructs.h"

#include "StarshatterShipDesignSubsystem.generated.h"

UCLASS()
class STARSHATTERWARS_API UStarshatterShipDesignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// ---------- lifecycle ----------
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ---------- load entry points ----------
	// Scans directories and parses all design defs.
	void LoadShipDesigns();
	void LoadAll(bool bFull = false);
	void LoadShipDesign(const char* InFilename);

	void SetProjectPath();
	FString GetProjectPath();

	// ---------- accessors ----------
	// Authoritative in-memory cache:
	const TMap<FName, FShipDesign>& GetDesignsByName() const { return DesignsByName; }
	const FShipDesign* FindDesign(const FName Name) const { return DesignsByName.Find(Name); }

	bool                 bClearTables;


private:
	// ---------- current-ship scratch ----------
	void BeginDesignParse(const char* Fn);
	void FinalizeDesignParse();

	// “Ship root” parser calls these:
	void ParsePower(TermStruct* val, const char* fn);
	void ParseDrive(TermStruct* val, const char* fn);
	void ParseQuantumDrive(TermStruct* Val, const char* Fn);
	void ParseFarcaster(TermStruct* Val, const char* Fn);
	void ParseThruster(TermStruct* Val, const char* Fn);
	void ParseNavlight(TermStruct* Val, const char* Fn);
	void ParseFlightDeck(TermStruct* Val, const char* Fn);
	void ParseLandingGear(TermStruct* Val, const char* Fn);
	void ParseWeapon(TermStruct* Val, const char* Fn);
	void ParseHardPoint(TermStruct* Val, const char* Fn);
	void ParseLoadout(TermStruct* val, const char* fn);
	void ParseSensor(TermStruct* Val, const char* Fn);
	void ParseNavsys(TermStruct* Val, const char* Fn);
	void ParseComputer(TermStruct* Val, const char* Fn);
	void ParseShield(TermStruct* Val, const char* Fn);
	void ParseSquadron(TermStruct* Val, const char* Fn);
	void ParseDeathSpiral(TermStruct* Val, const char* Fn);
	void ParseMap(TermStruct* Val, const char* Fn);
	void ParseSkin(TermStruct* Val, const char* Fn);

	FExplosion ParseExplosion(TermStruct* Val, const char* Fn);
	FDebris ParseDebris(TermStruct* Val, const char* Fn);

	void ResolveWeaponsForCurrentShip();
	void ValidateLoadoutsForCurrentShip() const;

	
private:
	// ---------- authoritative outputs ----------
	UPROPERTY()
	TMap<FName, FShipDesign> DesignsByName;

	UPROPERTY()
	FName CurrentShipName;

	UPROPERTY()
	float CurrentShipScale = 1.0f;

	// Paths
	FString ProjectPath;
	FString FilePath;

	UPROPERTY()
	bool bAnglesAreDegrees = false;

	// Per-ship parsed results (scratch while building CurrentShipDesign):
	UPROPERTY()
	FShipDesign CurrentDesign;

	TArray<FShipPower> NewShipPowerArray;
	TArray<FShipDrive> NewShipDriveArray;
	TArray<FShipQuantum> NewShipQuantumArray;
	TArray<FShipFarcaster> NewShipFarcasterArray;
	TArray<FShipThruster> NewShipThrusterArray;
	TArray<FThrusterPort> NewThrusterPortArray;
	TArray<FShipNavLight> NewShipNavLightArray;
	TArray<FShipFlightDeck> NewShipFlightDeckArray;
	TArray<FShipLandingGear> NewShipLandingGearArray;
	TArray<FShipWeapon> NewShipWeaponArray;
	TArray<FShipHardPoint> NewShipHardPointArray;
	TArray<FShipLoadout> NewShipLoadoutArray;
	TArray<FShipSensor> NewShipSensorArray;
	TArray<FShipNavSystem> NewShipNavSystemArray;
	TArray<FShipComputer> NewShipComputerArray;
	TArray<FShipShield> NewShipShieldArray;
	TArray<FShipSquadron> NewShipSquadronArray;
	TArray<FShipDeathSpiral> NewShipDeathSpiralArray;
	TArray<FShipMapSprite> NewShipMapSpriteArray;
	TArray<FShipSkin> NewShipSkinArray;

	// Ship-level special selection:
	UPROPERTY() FString CurrentShipDecoyWeaponType;
	UPROPERTY() FString CurrentShipSourceFile;
	UPROPERTY() FString CurrentShipProbeWeaponType;
	UPROPERTY() int32   CurrentShipDecoyWeaponIndex = INDEX_NONE;
	UPROPERTY() int32   CurrentShipProbeWeaponIndex = INDEX_NONE;

	UPROPERTY() TMap<FString, FWeaponDesignMeta> WeaponMetaByType;

	UDataTable* ShipDesignDataTable;
};

