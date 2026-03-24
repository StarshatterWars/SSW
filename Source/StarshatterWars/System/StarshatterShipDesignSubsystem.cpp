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

#include "StarshatterShipDesignSubsystem.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#include "Engine/DataTable.h"
#include "FormattingUtils.h"
#include "StarshatterAssetRegistrySubsystem.h"

static EShipCategory DeriveShipCategoryFromClass(const FString& InShipClass)
{
	const FString C = InShipClass.TrimStartAndEnd().ToLower();

	// Station-like:
	if (C.Contains(TEXT("station")) || C.Contains(TEXT("base")) || C.Contains(TEXT("farcaster")))
		return EShipCategory::Station;

	// Fighter-like:
	if (C.Contains(TEXT("fighter")) || C.Contains(TEXT("interceptor")) || C.Contains(TEXT("strike")) || C.Contains(TEXT("lca")) || C.Contains(TEXT("attack")))
		return EShipCategory::Fighter;

	if (C.Contains(TEXT("building")) || C.Contains(TEXT("factory")))
		return EShipCategory::Building;
	if (C.Contains(TEXT("sam")) || C.Contains(TEXT("mine")) || C.Contains(TEXT("comsat")) || C.Contains(TEXT("drone")))
		return EShipCategory::Platform;
	if (C.Contains(TEXT("hulk")))
		return EShipCategory::Hulk;
	if (C.Contains(TEXT("freighter")) || C.Contains(TEXT("courier")) || C.Contains(TEXT("transport")) || C.Contains(TEXT("cargo")))
		return EShipCategory::Transport;
	// Capital ships / big hulls:
	if (C.Contains(TEXT("cruiser")) || C.Contains(TEXT("destroyer")) || C.Contains(TEXT("carrier")) || 
		C.Contains(TEXT("dreadnought")) || C.Contains(TEXT("battleship")) || C.Contains(TEXT("frigate")) ||
		C.Contains(TEXT("corvette")))
		return EShipCategory::CapitalShip;

	return EShipCategory::Unknown;
}

void UStarshatterShipDesignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("[SHIP DESIGN] Initialize()"));

	/*------------------------------------------------------------------
	Resolve Ship Design DataTable via Asset Registry
------------------------------------------------------------------*/

	UStarshatterAssetRegistrySubsystem* Assets =
		GetGameInstance()->GetSubsystem<UStarshatterAssetRegistrySubsystem>();

	if (!Assets)
	{
		UE_LOG(LogTemp, Error, TEXT("[SHIPDESIGN] Asset Registry missing"));
		return;
	}

	ShipDesignDataTable =
		Assets->GetDataTable(TEXT("Data.ShipDesignTable"), true);

	if (!ShipDesignDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[SHIPDESIGN] ShipDesignTable not found"));
		return;
	}

	if (bClearTables)
	{
		ShipDesignDataTable->EmptyTable();
	}
}

void UStarshatterShipDesignSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[SHIPDESIGN] Deinitialize()"));
	DesignsByName.Empty();
	Super::Deinitialize();
}

void UStarshatterShipDesignSubsystem::SetProjectPath()
{
	ProjectPath = FPaths::ProjectDir();
	ProjectPath.Append(TEXT("GameData/"));

	UE_LOG(LogTemp, Log, TEXT("Setting [SHIPDESIGN] Directory %s"), *ProjectPath);
}

FString UStarshatterShipDesignSubsystem::GetProjectPath()
{
	return ProjectPath;
}

void UStarshatterShipDesignSubsystem::BeginDesignParse(const char* Fn)
{
	CurrentShipSourceFile = FString(ANSI_TO_TCHAR(Fn));

	CurrentShipScale = 1.0f;
	bAnglesAreDegrees = false;

	CurrentShipName = NAME_None;
	CurrentDesign = FShipDesign{};

	NewShipPowerArray.Empty();
	NewShipWeaponArray.Empty();
	NewShipHardPointArray.Empty();
	NewShipFlightDeckArray.Empty();
	NewShipDeathSpiralArray.Empty();

	CurrentShipDecoyWeaponType.Reset();
	CurrentShipProbeWeaponType.Reset();
	CurrentShipDecoyWeaponIndex = INDEX_NONE;
	CurrentShipProbeWeaponIndex = INDEX_NONE;
}

void UStarshatterShipDesignSubsystem::FinalizeDesignParse()
{
	
}

void UStarshatterShipDesignSubsystem::LoadAll(bool bLoaded)
{
	UE_LOG(LogTemp, Log, TEXT("[SHIPDESIGN] LoadAll()"));
	if (!bLoaded)
		return;

	LoadShipDesigns();
	bLoaded = true;
}

void UStarshatterShipDesignSubsystem::LoadShipDesigns()
{
	UE_LOG(LogTemp, Log, TEXT("[SHIPDESIGN] LoadShipDesigns()"));

	// Content/GameData/Ships/
	ProjectPath = FPaths::ProjectContentDir();
	ProjectPath /= TEXT("GameData/Ships/");

	TArray<FString> Files;
	const FString Wildcard = ProjectPath / TEXT("*.def");

	IFileManager::Get().FindFiles(Files, *Wildcard, /*Files=*/true, /*Directories=*/false);

	for (const FString& File : Files)
	{
		const FString FullPath = ProjectPath / File;

		// Legacy API wants const char*; convert for the call only (do NOT store the pointer).
		const FTCHARToUTF8 Utf8Path(*FullPath);
		LoadShipDesign(Utf8Path.Get());
	}
}

void UStarshatterShipDesignSubsystem::LoadShipDesign(const char* InFilename)
{
	//BeginDesignParse();
	if (!InFilename || !*InFilename)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadShipDesign: null/empty filename"));
		return;
	}

	const FString ShipFilePath = ANSI_TO_TCHAR(InFilename);
	UE_LOG(LogTemp, Log, TEXT("Loading Ship Design Data: %s"), *ShipFilePath);

	if (!FPaths::FileExists(ShipFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadShipDesign: file not found: %s"), *ShipFilePath);
		return;
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *ShipFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("LoadShipDesign: failed to read: %s"), *ShipFilePath);
		return;
	}
	Bytes.Add(0);

	const FTCHARToUTF8 Utf8Path(*ShipFilePath);
	const char* fn = Utf8Path.Get();

	Parser ParserObj(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* TermPtr = ParserObj.ParseTerm();

	if (!TermPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadShipDesign: could not parse: %s"), *ShipFilePath);
		return;
	}

	// Header check:
	{
		TermText* FileType = TermPtr->isText();
		if (!FileType || FileType->value() != "SHIP")
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid Ship File: %s"), *ShipFilePath);
			delete TermPtr;
			return;
		}
	}

	delete TermPtr;
	TermPtr = nullptr;

	// ------------------------------------------------------------
	// Locals
	// ------------------------------------------------------------
	Text LocalShipName = "";
	Text LocalDisplayName = "";
	Text LocalDescription = "";
	Text LocalAbrv = "";

	Text LocalModelName = "";

	Text LocalShipClass = "";
	Text LocalShipEmpire = "";
	Text LocalCockpitName = "";
	Text LocalBeautyName = "";
	Text LocalHudIconName = "";

	int32 LocalPcs = 3;
	int32 LocalAcs = 1;
	float LocalDetec = 250.0e3f;
	float LocalScale = 1.0f;
	float LocalExplosionScale = 0.0f;
	float LocalMass = 0.0f;

	int32 LocalShipType = 0;

	float LocalVlimit = 8e3f;
	float LocalAgility = 2e2f;
	float LocalAirFactor = 0.1f;
	float LocalRollRate = 0.0f;
	float LocalPitchRate = 0.0f;
	float LocalYawRate = 0.0f;
	float LocalTransX = 0.0f;
	float LocalTransY = 0.0f;
	float LocalTransZ = 0.0f;
	float LocalTurnBank = (float)(PI / 8);

	float LocalCockpitScale = 1.0f;
	float LocalAutoRoll = 0.0f;

	float LocalCL = 0.0f;
	float LocalCD = 0.0f;
	float LocalStall = 0.0f;

	float LocalPrepTime = 30.0f;
	float LocalAvoidTime = 0.0f;
	float LocalAvoidFighter = 0.0f;
	float LocalAvoidStrike = 0.0f;
	float LocalAvoidTarget = 0.0f;
	float LocalCommitRange = 0.0f;

	float LocalSplashRadius = -1.0f;
	float LocalScuttle = 5e3f;
	float LocalRepairSpeed = 1.0f;

	int32 LocalRepairTeams = 2;

	float LocalEFactor[3] = { 0.1f, 0.3f, 1.0f };

	bool bLocalRepairAuto = true;
	bool bLocalSecret = false;
	bool bLocalRepairScreen = true;
	bool bLocalWepScreen = true;
	bool bLocalDegrees = false;

	FVector LocalOffLoc = FVector::ZeroVector;
	FVector LocalSpin = FVector::ZeroVector;
	FVector LocalBeautyCam = FVector::ZeroVector;
	FVector LocalChaseVec = FVector(0.f, -100.f, 20.f);
	FVector LocalBridgeVec = FVector::ZeroVector;

	EShipEmpire LocalEmpire = EShipEmpire::Terellian;

	bool bLocalHidden = false; 

	FShipDesign NewShipDesign;
	NewShipPowerArray.Empty();
	NewShipDriveArray.Empty();
	NewShipQuantumArray.Empty();
	NewShipFarcasterArray.Empty();
	NewShipThrusterArray.Empty();
	NewShipNavLightArray.Empty();
	NewShipFlightDeckArray.Empty();
	NewShipLandingGearArray.Empty();
	NewShipWeaponArray.Empty();
	NewShipHardPointArray.Empty();
	NewShipLoadoutArray.Empty();
	NewShipSensorArray.Empty();
	NewShipNavSystemArray.Empty();
	NewShipComputerArray.Empty();
	NewShipShieldArray.Empty();
	NewShipSquadronArray.Empty();
	NewShipDeathSpiralArray.Empty();
	NewShipMapSpriteArray.Empty();
	NewShipSkinArray.Empty();

	// ------------------------------------------------------------
	// Parse terms
	// ------------------------------------------------------------
	while ((TermPtr = ParserObj.ParseTerm()) != nullptr)
	{
		TermDef* Def = TermPtr->isDef();
		if (!Def)
		{
			delete TermPtr;
			TermPtr = nullptr;
			continue;
		}

		const Text& Key = Def->name()->value();

		if (Key == "name")
		{
			GetDefText(LocalShipName, Def, fn);
			NewShipDesign.ShipName = FString(LocalShipName);
		}
		else if (Key == "display_name")
		{
			GetDefText(LocalDisplayName, Def, fn);
			NewShipDesign.DisplayName = FString(LocalDisplayName);
		}
		else if (Key == "class")
		{
			GetDefText(LocalShipClass, Def, fn);
			NewShipDesign.ShipClass = FString(LocalShipClass);

			LocalShipType = UFormattingUtils::GetDesignClassFromName(LocalShipClass);

			if (LocalShipType <= (int32)CLASSIFICATION::LCA)
			{
				bLocalRepairAuto = false;
				bLocalRepairScreen = false;
				bLocalWepScreen = false;
			}

			NewShipDesign.Category = DeriveShipCategoryFromClass(NewShipDesign.ShipClass);
			NewShipDesign.ShipType = LocalShipType;
			NewShipDesign.RepairAuto = bLocalRepairAuto;
			NewShipDesign.RepairScreen = bLocalRepairScreen;
			NewShipDesign.WepScreen = bLocalWepScreen;
			NewShipDesign.Hidden = bLocalHidden;
		}
		else if (Key == "empire")
		{
			GetDefText(LocalShipEmpire, Def, fn);
			NewShipDesign.ShipEmpire = UFormattingUtils::GetShipEmpireFromString(FString(LocalShipEmpire));
		}
		else if (Key == "description")
		{
			GetDefText(LocalDescription, Def, fn);
			NewShipDesign.Description = FString(LocalDescription);
		}
		else if (Key == "abrv")
		{
			GetDefText(LocalAbrv, Def, fn);
			NewShipDesign.Abrv = FString(LocalAbrv);
		}
		else if (Key == "model")
		{
			GetDefText(LocalModelName, Def, fn);
			NewShipDesign.Model = FString(LocalModelName);
		}
		else if (Key == "secret")
		{
			GetDefBool(bLocalSecret, Def, fn);
			NewShipDesign.Secret = bLocalSecret;
		}
		else if (Key == "pcs")
		{
			GetDefNumber(LocalPcs, Def, fn);
			NewShipDesign.PCS = LocalPcs;
		}
		else if (Key == "acs")
		{
			GetDefNumber(LocalAcs, Def, fn);
			NewShipDesign.ACS = LocalAcs;
		}
		else if (Key == "detec")
		{
			GetDefNumber(LocalDetec, Def, fn);
			NewShipDesign.Detet = LocalDetec;
		}
		else if (Key == "scale")
		{
			GetDefNumber(LocalScale, Def, fn);
			NewShipDesign.Scale = LocalScale;
		}
		else if (Key == "explosion_scale")
		{
			GetDefNumber(LocalExplosionScale, Def, fn);
			NewShipDesign.ExplosionScale = LocalExplosionScale;
		}
		else if (Key == "mass")
		{
			GetDefNumber(LocalMass, Def, fn);
			NewShipDesign.Mass = LocalMass;
		}
		else if (Key == "vlimit")
		{
			GetDefNumber(LocalVlimit, Def, fn);
			NewShipDesign.Vlimit = LocalVlimit;
		}
		else if (Key == "agility")
		{
			GetDefNumber(LocalAgility, Def, fn);
			NewShipDesign.Agility = LocalAgility;
		}
		else if (Key == "air_factor")
		{
			GetDefNumber(LocalAirFactor, Def, fn);
			NewShipDesign.AirFactor = LocalAirFactor;
		}
		else if (Key == "roll_rate")
		{
			GetDefNumber(LocalRollRate, Def, fn);
			NewShipDesign.RollRate = LocalRollRate;
		}
		else if (Key == "pitch_rate")
		{
			GetDefNumber(LocalPitchRate, Def, fn);
			NewShipDesign.PitchRate = LocalPitchRate;
		}
		else if (Key == "yaw_rate")
		{
			GetDefNumber(LocalYawRate, Def, fn);
			NewShipDesign.YawRate = LocalYawRate;
		}
		else if (Key == "trans_x")
		{
			GetDefNumber(LocalTransX, Def, fn);
			NewShipDesign.Trans.X = LocalTransX;
		}
		else if (Key == "trans_y")
		{
			GetDefNumber(LocalTransY, Def, fn);
			NewShipDesign.Trans.Y = LocalTransY;
		}
		else if (Key == "trans_z")
		{
			GetDefNumber(LocalTransZ, Def, fn);
			NewShipDesign.Trans.Z = LocalTransZ;
		}
		else if (Key == "turn_bank")
		{
			GetDefNumber(LocalTurnBank, Def, fn);
			NewShipDesign.TurnBank = LocalTurnBank;
		}
		else if (Key == "cockpit_scale")
		{
			GetDefNumber(LocalCockpitScale, Def, fn);
			NewShipDesign.CockpitScale = LocalCockpitScale;
		}
		else if (Key == "auto_roll")
		{
			GetDefNumber(LocalAutoRoll, Def, fn);
			NewShipDesign.AutoRoll = LocalAutoRoll;
		}
		else if (Key == "CL")
		{
			GetDefNumber(LocalCL, Def, fn);
			NewShipDesign.CL = LocalCL;
		}
		else if (Key == "CD")
		{
			GetDefNumber(LocalCD, Def, fn);
			NewShipDesign.CD = LocalCD;
		}
		else if (Key == "stall")
		{
			GetDefNumber(LocalStall, Def, fn);
			NewShipDesign.Stall = LocalStall;
		}
		else if (Key == "prep_time")
		{
			GetDefNumber(LocalPrepTime, Def, fn);
			NewShipDesign.PrepTime = LocalPrepTime;
		}
		else if (Key == "avoid_time")
		{
			GetDefNumber(LocalAvoidTime, Def, fn);
			NewShipDesign.AvoidTime = LocalAvoidTime;
		}
		else if (Key == "avoid_fighter")
		{
			GetDefNumber(LocalAvoidFighter, Def, fn);
			NewShipDesign.AvoidFighter = LocalAvoidFighter;
		}
		else if (Key == "avoid_strike")
		{
			GetDefNumber(LocalAvoidStrike, Def, fn);
			NewShipDesign.AvoidStrike = LocalAvoidStrike;
		}
		else if (Key == "avoid_target")
		{
			GetDefNumber(LocalAvoidTarget, Def, fn);
			NewShipDesign.AvoidTarget = LocalAvoidTarget;
		}
		else if (Key == "commit_range")
		{
			GetDefNumber(LocalCommitRange, Def, fn);
			NewShipDesign.CommitRange = LocalCommitRange;
		}
		else if (Key == "splash_radius")
		{
			GetDefNumber(LocalSplashRadius, Def, fn);
			NewShipDesign.SplashRadius = LocalSplashRadius;
		}
		else if (Key == "scuttle")
		{
			GetDefNumber(LocalScuttle, Def, fn);
			NewShipDesign.Scuttle = LocalScuttle;
		}
		else if (Key == "repair_speed")
		{
			GetDefNumber(LocalRepairSpeed, Def, fn);
			NewShipDesign.RepairSpeed = LocalRepairSpeed;
		}
		else if (Key == "repair_teams")
		{
			GetDefNumber(LocalRepairTeams, Def, fn);
			NewShipDesign.RepairTeams = LocalRepairTeams;
		}
		else if (Key == "cockpit_model")
		{
			GetDefText(LocalCockpitName, Def, fn);
			NewShipDesign.CockpitName = FString(LocalCockpitName);
		}
		else if (Key == "spin")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			LocalSpin = V;
			NewShipDesign.Spin = LocalSpin;
		}
		else if (Key == "offset_0")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			NewShipDesign.Offset[0] = V;
		}
		else if (Key == "offset_1")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			NewShipDesign.Offset[1] = V;
		}
		else if (Key == "offset_2")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			NewShipDesign.Offset[2] = V;
		}
		else if (Key == "offset_3")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			NewShipDesign.Offset[3] = V;
		}
		else if (Key == "beauty")
		{
			if (Def->term() && Def->term()->isArray())
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, Def, fn);
				LocalBeautyCam = V;

				if (bLocalDegrees)
				{
					LocalBeautyCam.X *= (float)DEGREES;
					LocalBeautyCam.Y *= (float)DEGREES;
				}

				NewShipDesign.BeautyCam = LocalBeautyCam;
			}
			else
			{
				GetDefText(LocalBeautyName, Def, fn);
				NewShipDesign.BeautyName = FString(LocalBeautyName); // IMPORTANT
			}
		}
		else if (Key == "hud_icon")
		{
			GetDefText(LocalHudIconName, Def, fn);
			NewShipDesign.HudIconName = FString(LocalHudIconName);
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(LocalEFactor[0], Def, fn);
			NewShipDesign.EFactor[0] = LocalEFactor[0];
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(LocalEFactor[1], Def, fn);
			NewShipDesign.EFactor[1] = LocalEFactor[1];
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(LocalEFactor[2], Def, fn);
			NewShipDesign.EFactor[2] = LocalEFactor[2];
		}
		else if (Key == "chase")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			LocalChaseVec = V * LocalScale;
			NewShipDesign.ChaseVec = LocalChaseVec;
		}
		else if (Key == "bridge")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, fn);
			LocalBridgeVec = V * LocalScale;
			NewShipDesign.BridgeVec = LocalBridgeVec;
		}
		else if (Key == "power")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: power source struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParsePower(Def->term()->isStruct(), fn);
				NewShipDesign.Power = NewShipPowerArray;
			}
		}
		else if (Key == "main_drive" || Key == "drive")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: drive struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseDrive(Def->term()->isStruct(), fn);
				NewShipDesign.Drive = NewShipDriveArray;
			}
		}
		else if (Key == "quantum" || Key == "quantum_drive")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: quantum struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseQuantumDrive(Def->term()->isStruct(), fn);
				NewShipDesign.Quantum = NewShipQuantumArray;
			}
		}
		else if (Key == "sender" || Key == "farcaster")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: farcaster struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseFarcaster(Def->term()->isStruct(), fn);
				NewShipDesign.Farcaster = NewShipFarcasterArray;
			}
		}
		else if (Key == "thruster")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: thruster struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseThruster(Def->term()->isStruct(), fn);
				NewShipDesign.Thruster = NewShipThrusterArray;
			}
		}
		else if (Key == "navlight")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: navlight struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseNavlight(Def->term()->isStruct(), fn);
				NewShipDesign.Navlight = NewShipNavLightArray;
			}
		}
		else if (Key == "flightdeck")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: flightdeck struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseFlightDeck(Def->term()->isStruct(), fn);
				NewShipDesign.FlightDeck = NewShipFlightDeckArray;
			}
		}
		else if (Key == "gear")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: landing gear struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseLandingGear(Def->term()->isStruct(), fn);
				NewShipDesign.LandingGear = NewShipLandingGearArray;
			}
		}
		else if (Key == "weapon")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: weapon struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseWeapon(Def->term()->isStruct(), fn);
				NewShipDesign.Weapon = NewShipWeaponArray;
			}
		}
		else if (Key == "decoy")
		{
			Text Buf;
			if (GetDefText(Buf, Def, fn))
			{
				CurrentShipDecoyWeaponType = FString(Buf);
			}
		}
		else if (Key == "probe")
		{
			Text Buf;
			if (GetDefText(Buf, Def, fn))
			{
				CurrentShipProbeWeaponType = FString(Buf);
			}
		}
		else if (Key == "hardpoint")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: hardpoint struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseHardPoint(Def->term()->isStruct(), fn);
				NewShipDesign.Hardpoint = NewShipHardPointArray;
			}
		}
		else if (Key == "loadout")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: loadout struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseLoadout(Def->term()->isStruct(), fn);
				NewShipDesign.Loadout = NewShipLoadoutArray;
			}
		}
		else if (Key == "sensor")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: sensor struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseSensor(Def->term()->isStruct(), fn);
				NewShipDesign.Sensor = NewShipSensorArray;
			}
		}
		else if (Key == "nav")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: nav struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseNavsys(Def->term()->isStruct(), fn);
				NewShipDesign.NavSys = NewShipNavSystemArray;
			}
		}
		else if (Key == "computer")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: computer struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseComputer(Def->term()->isStruct(), fn);
				NewShipDesign.Computer = NewShipComputerArray;
			}
		}
		else if (Key == "shield")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: shield struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseShield(Def->term()->isStruct(), fn);
				NewShipDesign.Shield = NewShipShieldArray;
			}
		}
		else if (Key == "squadron")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: squadron struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseSquadron(Def->term()->isStruct(), fn);
				NewShipDesign.Squadron = NewShipSquadronArray;
			}
		}
		else if (Key == "death_spiral")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: death spiral struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseDeathSpiral(Def->term()->isStruct(), fn);
				NewShipDesign.DeathSpiral = NewShipDeathSpiralArray;
			}
		}
		else if (Key == "map")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: map struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseMap(Def->term()->isStruct(), fn);
				NewShipDesign.Map = NewShipMapSpriteArray;
			}
		}
		else if (Key == "skin")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: skin struct missing in '%s'"), *ShipFilePath);
			}
			else
			{
				ParseSkin(Def->term()->isStruct(), fn);
				NewShipDesign.Skin = NewShipSkinArray;
			}
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("WARNING: unknown ship parameter '%s' in '%s'"),
				*FString(ANSI_TO_TCHAR(Key.data())), *ShipFilePath);
		}

		delete TermPtr;
		TermPtr = nullptr;
	}

	// ------------------------------------------------------------
	// Add row ONCE after parse
	// ------------------------------------------------------------
	if (!ShipDesignDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("LoadShipDesign: ShipDesignDataTable is null"));
		return;
	}

	if (!LocalShipName.length())
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadShipDesign: missing 'name' in '%s'"), *ShipFilePath);
		return;
	}

	const FString ShipName = NewShipDesign.ShipName.TrimStartAndEnd();
	if (ShipName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadShipDesign: missing 'name' in '%s'"), *ShipFilePath);
		return;
	}
	const FName CleanRowName(*ShipName);

	// ------------------------------------------------------------------
	// DataTable UPSERT (same pattern as systems)
	// ------------------------------------------------------------------
	if (ShipDesignDataTable)
	{
		if (FShipDesign* Existing =
			ShipDesignDataTable->FindRow<FShipDesign>(
				CleanRowName,
				TEXT("LoadShipDesign"),
				/*bWarnIfRowMissing=*/false))
		{
			*Existing = NewShipDesign;   // overwrite in place
		}
		else
		{
			ShipDesignDataTable->AddRow(CleanRowName, NewShipDesign);
		}

		UE_LOG(LogTemp, Log,
			TEXT("[SHIPDESIGN] Upsert OK: %s"),
			*CleanRowName.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[SHIPDESIGN] ShipDesignDataTable is null"));
	}
}

// +--------------------------------------------------------------------+

static EPowerSource PowerTypeFromText(const Text& TypeName)
{
	if (!TypeName.length())
		return EPowerSource::NONE;

	const char c = TypeName[0];

	if (c == 'B' || c == 'b') return EPowerSource::BATTERY;
	if (c == 'A' || c == 'a') return EPowerSource::AUXILIARY;
	if (c == 'F' || c == 'f') return EPowerSource::FUSION;

	return EPowerSource::NONE;
}

void UStarshatterShipDesignSubsystem::ParsePower(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParsePower()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParsePower called with null args"));
		return;
	}

	// If LoadShipDesign set this, we use it:
	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	float   Output = 1000.0f;
	float   FuelRange = 0.0f;
	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   HullFactor = 0.5f;

	Text    DesignName = "";
	Text    PName = "";
	Text    PAbrv = "";
	Text    TypeName = "";

	int32   ExplosionType = 0;
	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	FShipPower NewShipPower;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			if (GetDefText(TypeName, PDef, Fn))
			{
				NewShipPower.Type = PowerTypeFromText(TypeName);

				if (NewShipPower.Type == EPowerSource::NONE)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("ParsePower: unknown power type '%s' in '%s'"),
						*FString(ANSI_TO_TCHAR(TypeName.data())),
						*FString(ANSI_TO_TCHAR(Fn)));
				}
			}
		}
		else if (Key == "name")
		{
			GetDefText(PName, PDef, Fn);
			NewShipPower.PName = FString(PName);
		}
		else if (Key == "abrv")
		{
			GetDefText(PAbrv, PDef, Fn);
			NewShipPower.PAbrv = FString(PAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewShipPower.DesignName = FString(DesignName);
		}
		else if (Key == "max_output")
		{
			GetDefNumber(Output, PDef, Fn);
			NewShipPower.Output = Output;
		}
		else if (Key == "fuel_range")
		{
			GetDefNumber(FuelRange, PDef, Fn);
			NewShipPower.Fuel = FuelRange;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			Loc = V;
			NewShipPower.Loc = Loc;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;

			NewShipPower.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(HullFactor, PDef, Fn);
			NewShipPower.Hull = HullFactor;
		}
		else if (Key == "explosion")
		{
			GetDefNumber(ExplosionType, PDef, Fn);
			NewShipPower.ExplosionType = ExplosionType;
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewShipPower.Emcon1 = Emcon1;
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewShipPower.Emcon2 = Emcon2;
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewShipPower.Emcon3 = Emcon3;
		}
	}

	// Store like your other parse helpers:
	NewShipPowerArray.Add(NewShipPower);
}

static EDriveType ParseDriveTypeString(FString In)
{
	In.TrimStartAndEndInline();
	In.ToLowerInline();

	if (In == TEXT("plasma"))  return EDriveType::PLASMA;
	if (In == TEXT("fusion"))  return EDriveType::FUSION;
	if (In == TEXT("alien"))   return EDriveType::GREEN;
	if (In == TEXT("green"))   return EDriveType::GREEN;
	if (In == TEXT("red"))     return EDriveType::RED;
	if (In == TEXT("blue"))    return EDriveType::BLUE;
	if (In == TEXT("yellow"))  return EDriveType::YELLOW;
	if (In == TEXT("stealth")) return EDriveType::STEALTH;

	return EDriveType::UNKNOWN;
}
// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseDrive(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseDrive()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseDrive called with null args"));
		return;
	}

	// Match ParsePower behavior:
	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults (local mirrors for clarity):
	Text    DName = "";
	Text    DAbrv = "";
	Text    DesignName = "";
	Text    TypeName = "";

	float   Thrust = 1.0f;
	float   Augmenter = 0.0f;
	float   DriveScale = 1.0f;

	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   HullFactor = 0.5f;

	int32   ExplosionType = 0;
	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	bool    bTrail = true;

	FShipDrive NewDrive;
	NewDrive.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			if (GetDefText(TypeName, PDef, Fn))
			{
				const FString TypeStr = FString(ANSI_TO_TCHAR(TypeName.data()));
				NewDrive.Type = ParseDriveTypeString(TypeStr);

				if (NewDrive.Type == EDriveType::UNKNOWN)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("ParseDrive: unknown drive type '%s' in '%s'"),
						*TypeStr,
						*FString(ANSI_TO_TCHAR(Fn)));
				}
			}
		}
		else if (Key == "name")
		{
			GetDefText(DName, PDef, Fn);
			NewDrive.Name = FString(DName);
		}
		else if (Key == "abrv")
		{
			GetDefText(DAbrv, PDef, Fn);
			NewDrive.Abbrev = FString(DAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewDrive.DesignName = FString(DesignName);
		}
		else if (Key == "thrust")
		{
			GetDefNumber(Thrust, PDef, Fn);
			NewDrive.Thrust = Thrust;
		}
		else if (Key == "augmenter")
		{
			GetDefNumber(Augmenter, PDef, Fn);
			NewDrive.Augmenter = Augmenter;
		}
		else if (Key == "scale")
		{
			GetDefNumber(DriveScale, PDef, Fn);
			NewDrive.DriveScale = DriveScale;
		}
		else if (Key == "port")
		{
			// Matches legacy behavior:
			// - location is multiplied by global Scale
			// - flare scale defaults to drive's dscale unless overridden
			FDrivePort Port;
			Port.Location = FVector::ZeroVector;
			Port.FlareScale = (NewDrive.DriveScale > 0.0f) ? NewDrive.DriveScale : 1.0f;

			if (PDef->term()->isArray())
			{
				FVector PV = FVector::ZeroVector;
				if (GetDefVec(PV, PDef, Fn))
				{
					PV *= Scale;
					Port.Location = PV;
					// flare scale already defaulted
					NewDrive.Ports.Add(Port);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("ParseDrive: invalid port array in '%s'"), *NewDrive.SourceFile);
				}
			}
			else if (PDef->term()->isStruct())
			{
				TermStruct* PortStruct = PDef->term()->isStruct();

				FVector PV = FVector::ZeroVector;
				float FlareScale = 0.0f;

				const int32 PortCount = (int32)PortStruct->elements()->size();
				for (int32 j = 0; j < PortCount; ++j)
				{
					TermDef* P2 = PortStruct->elements()->at(j)->isDef();
					if (!P2)
						continue;

					const Text& PKey = P2->name()->value();

					if (PKey == "loc")
					{
						GetDefVec(PV, P2, Fn);
					}
					else if (PKey == "scale")
					{
						GetDefNumber(FlareScale, P2, Fn);
					}
				}

				PV *= Scale;

				if (FlareScale <= 0.0f)
					FlareScale = (NewDrive.DriveScale > 0.0f) ? NewDrive.DriveScale : 1.0f;

				Port.Location = PV;
				Port.FlareScale = FlareScale;
				NewDrive.Ports.Add(Port);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseDrive: invalid port term type in '%s'"), *NewDrive.SourceFile);
			}
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			if (GetDefVec(V, PDef, Fn))
			{
				V *= Scale;
				Loc = V;
				NewDrive.Location = Loc;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseDrive: invalid loc in '%s'"), *NewDrive.SourceFile);
			}
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;
			NewDrive.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(HullFactor, PDef, Fn);
			NewDrive.HullFactor = HullFactor;
		}
		else if (Key == "explosion")
		{
			GetDefNumber(ExplosionType, PDef, Fn);
			NewDrive.ExplosionType = ExplosionType;
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewDrive.Emcon1 = Emcon1;
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewDrive.Emcon2 = Emcon2;
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewDrive.Emcon3 = Emcon3;
		}
		else if (Key == "trail" || Key == "show_trail")
		{
			GetDefBool(bTrail, PDef, Fn);
			NewDrive.bShowTrail = bTrail;
		}
	}

	NewShipDriveArray.Add(NewDrive);
}

// +--------------------------------------------------------------------+

static EQuantumDriveType QuantumDriveTypeFromText(const Text& InType)
{
	FString S = FString(ANSI_TO_TCHAR(InType.data()));
	S.TrimStartAndEndInline();
	S.ToLowerInline();

	// Legacy: contains("hyper")
	return S.Contains(TEXT("hyper")) ? EQuantumDriveType::HYPER : EQuantumDriveType::QUANTUM;
}

static int32 ClampEmcon(int32 V)
{
	return (V >= 0 && V <= 100) ? V : -1;
}

void UStarshatterShipDesignSubsystem::ParseQuantumDrive(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseQuantumDrive()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseQuantumDrive called with null args"));
		return;
	}

	// Match ParsePower/ParseDrive behavior:
	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	double  Capacity = 250000.0;
	double  Consumption = 1000.0;
	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   Hull = 0.5f;
	float   Countdown = 5.0f;

	Text    DesignName = "";
	Text    TypeName = "";
	Text    Abrv = "";

	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	EQuantumDriveType QType = EQuantumDriveType::QUANTUM;

	FShipQuantum NewQuantum;
	NewQuantum.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewQuantum.DesignName = FString(DesignName);
		}
		else if (Key == "abrv")
		{
			GetDefText(Abrv, PDef, Fn);
			NewQuantum.Abbrev = FString(Abrv);
		}
		else if (Key == "type")
		{
			if (GetDefText(TypeName, PDef, Fn))
			{
				QType = QuantumDriveTypeFromText(TypeName);
				NewQuantum.Type = QType;
			}
		}
		else if (Key == "capacity")
		{
			GetDefNumber(Capacity, PDef, Fn);
			NewQuantum.Capacity = Capacity;
		}
		else if (Key == "consumption")
		{
			GetDefNumber(Consumption, PDef, Fn);
			NewQuantum.Consumption = Consumption;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			Loc = V;
			NewQuantum.Location = Loc;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;

			NewQuantum.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewQuantum.HullFactor = Hull;
		}
		else if (Key == "jump_time" || Key == "countdown")
		{
			GetDefNumber(Countdown, PDef, Fn);
			NewQuantum.Countdown = Countdown;
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewQuantum.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewQuantum.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewQuantum.Emcon3 = ClampEmcon(Emcon3);
		}
	}

	// Legacy: drive->SetSourceIndex(reactors.size() - 1);
	// Here: store what the runtime builder should use.
	// If you have NewShipReactorArray or similar, use that count:
	NewQuantum.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	// Store like your other parse helpers:
	NewShipQuantumArray.Add(NewQuantum);
}
// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseFarcaster(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseFarcaster()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseFarcaster called with null args"));
		return;
	}

	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	Text    DesignName = "";
	double  Capacity = 300000.0;
	double  Consumption = 15000.0;
	FVector Loc = FVector::ZeroVector;
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	float   Size = 0.0f;
	float   Hull = 0.5f;
	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	FShipFarcaster NewCaster;
	NewCaster.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// If you still want a legacy cap (NUM_APPROACH_PTS), set it here:
	const int32 MaxApproachPts = 8; // TODO: match Farcaster::NUM_APPROACH_PTS from legacy
	NewCaster.ApproachPoints.Reserve(MaxApproachPts);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewCaster.DesignName = FString(DesignName);
		}
		else if (Key == "capacity")
		{
			GetDefNumber(Capacity, PDef, Fn);
			NewCaster.Capacity = Capacity;
		}
		else if (Key == "consumption")
		{
			GetDefNumber(Consumption, PDef, Fn);
			NewCaster.Consumption = Consumption;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			Loc = V;
			NewCaster.Location = Loc;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;

			NewCaster.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewCaster.HullFactor = Hull;
		}
		else if (Key == "start")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			Start = V;
			NewCaster.StartPoint = Start;
		}
		else if (Key == "end")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			End = V;
			NewCaster.EndPoint = End;
		}
		else if (Key == "approach")
		{
			if (NewCaster.ApproachPoints.Num() < MaxApproachPts)
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, PDef, Fn);
				V *= Scale;

				NewCaster.ApproachPoints.Add(V);
			}
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("ParseFarcaster: approach point ignored in '%s' (max=%d)"),
					*NewCaster.SourceFile, MaxApproachPts);
			}
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewCaster.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewCaster.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewCaster.Emcon3 = ClampEmcon(Emcon3);
		}
	}

	// Legacy: caster->SetSourceIndex(reactors.size() - 1)
	NewCaster.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	// Store like your other parse helpers:
	NewShipFarcasterArray.Add(NewCaster);
}

// +--------------------------------------------------------------------+

static bool TryGetThrusterPortDirFromKey(const Text& Key, EThrusterPortDir& OutDir)
{
	// Keys in legacy:
	// port, port_bottom, port_top, port_left, port_right, port_fore, port_aft
	// NOTE: `Text` comparisons are case-insensitive in legacy by setSensitive(false),
	// but your pipeline uses exact Text keys. Keep it literal to match your other parsers.
	if (Key == "port" || Key == "port_bottom") { OutDir = EThrusterPortDir::BOTTOM; return true; }
	if (Key == "port_top") { OutDir = EThrusterPortDir::TOP; return true; }
	if (Key == "port_left") { OutDir = EThrusterPortDir::LEFT; return true; }
	if (Key == "port_right") { OutDir = EThrusterPortDir::RIGHT; return true; }
	if (Key == "port_fore") { OutDir = EThrusterPortDir::FORE; return true; }
	if (Key == "port_aft") { OutDir = EThrusterPortDir::AFT; return true; }
	return false;
}

void UStarshatterShipDesignSubsystem::ParseThruster(TermStruct* Val, const char* Fn)
{
	NewThrusterPortArray.Empty();
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseThruster()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseThruster called with null args"));
		return;
	}

	// Legacy: only one thruster allowed
	if (NewShipThrusterArray.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseThruster: additional thruster ignored in '%s'"), ANSI_TO_TCHAR(Fn));
		return;
	}

	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	double  Thrust = 100.0;
	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   Hull = 0.5f;
	Text    DesignName = "";
	float   TScale = 1.0f;
	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	EDriveType DriveType = EDriveType::UNKNOWN;

	FShipThruster NewThruster;
	NewThruster.Ports.Empty();
	NewThruster.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			TermText* TName = PDef->term() ? PDef->term()->isText() : nullptr;
			if (TName)
			{
				const FString TypeStr = FString(ANSI_TO_TCHAR(TName->value().data()));
				DriveType = ParseDriveTypeString(TypeStr);
				NewThruster.Type = DriveType;

				if (DriveType == EDriveType::UNKNOWN)
				{
					UE_LOG(LogTemp, Warning, TEXT("ParseThruster: unknown thruster type '%s' in '%s'"),
						*TypeStr, *NewThruster.SourceFile);
				}
			}
		}
		else if (Key == "thrust")
		{
			GetDefNumber(Thrust, PDef, Fn);
			NewThruster.Thrust = Thrust;
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewThruster.DesignName = FString(DesignName);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;

			Loc = V;
			NewThruster.Location = Loc;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;

			NewThruster.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewThruster.HullFactor = Hull;
		}
		else if (Key == "scale")
		{
			GetDefNumber(TScale, PDef, Fn);
			NewThruster.ThrusterScale = TScale;
		}
		else
		{
			// Port keys are name-driven: port, port_top, etc.

			EThrusterPortDir Dir;
			if (TryGetThrusterPortDirFromKey(Key, Dir) && PDef->term())
			{
				FThrusterPort Port;
				Port.Direction = Dir;
				Port.Location = FVector::ZeroVector;
				Port.Fire = 0;
				Port.PortScale = (NewThruster.ThrusterScale > 0.0f) ? NewThruster.ThrusterScale : 1.0f;

				if (PDef->term()->isArray())
				{
					FVector PV = FVector::ZeroVector;
					if (GetDefVec(PV, PDef, Fn))
					{
						PV *= Scale;
						Port.Location = PV;
						// PortScale default already TScale
						NewThruster.Ports.Add(Port);
					}
				}
				else if (PDef->term()->isStruct())
				{
					TermStruct* PortStruct = PDef->term()->isStruct();

					FVector PV = FVector::ZeroVector;
					float PortScale = 0.0f;
					DWORD Fire = 0;

					const int32 PortCount = (int32)PortStruct->elements()->size();
					for (int32 j = 0; j < PortCount; ++j)
					{
						TermDef* P2 = PortStruct->elements()->at(j)->isDef();
						if (!P2) continue;

						const Text& PKey = P2->name()->value();

						if (PKey == "loc")
						{
							GetDefVec(PV, P2, Fn);
						}
						else if (PKey == "fire")
						{
							GetDefNumber(Fire, P2, Fn);
						}
						else if (PKey == "scale")
						{
							GetDefNumber(PortScale, P2, Fn);
						}
					}

					PV *= Scale;

					if (PortScale <= 0.0f)
						PortScale = (NewThruster.ThrusterScale > 0.0f) ? NewThruster.ThrusterScale : 1.0f;

					Port.Location = PV;
					Port.Fire = (int32)Fire;
					Port.PortScale = PortScale;

					NewThruster.Ports.Add(Port);
				}
			}
		}

		if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewThruster.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewThruster.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewThruster.Emcon3 = ClampEmcon(Emcon3);
		}
	}

	// Legacy: drive->SetSourceIndex(reactors.size()-1)
	NewThruster.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	// Store like your other parse helpers:
	NewShipThrusterArray.Add(NewThruster);
}
// +--------------------------------------------------------------------+

static ENavLightType NavLightTypeFromLegacyInt(int32 InT)
{
	// Legacy: if t < 1 || t > 4 -> t = 1
	int32 T = InT;
	if (T < 1 || T > 4) T = 1;

	switch (T)
	{
	case 1: return ENavLightType::TYPE_1;
	case 2: return ENavLightType::TYPE_2;
	case 3: return ENavLightType::TYPE_3;
	case 4: return ENavLightType::TYPE_4;
	default: return ENavLightType::TYPE_1;
	}
}

void UStarshatterShipDesignSubsystem::ParseNavlight(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseNavlight()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseNavlight called with null args"));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	Text  DName = "";
	Text  DAbrv = "";
	Text  DesignName = "";

	float DScale = 1.0f;  // navlight scale param
	float Period = 10.0f;

	FShipNavLight NewNav;
	NewNav.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// If you want an explicit cap like legacy MAX_LIGHTS:
	const int32 MaxLights = 16; // TODO: set to NavLight::MAX_LIGHTS value from legacy
	NewNav.Beacons.Reserve(MaxLights);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(DName, PDef, Fn);
			NewNav.Name = FString(DName);
		}
		else if (Key == "abrv")
		{
			GetDefText(DAbrv, PDef, Fn);
			NewNav.Abbrev = FString(DAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewNav.DesignName = FString(DesignName);
		}
		else if (Key == "scale")
		{
			GetDefNumber(DScale, PDef, Fn);
			NewNav.Scale = DScale;
		}
		else if (Key == "period")
		{
			GetDefNumber(Period, PDef, Fn);
			NewNav.Period = Period;
		}
		else if (Key == "light")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseNavlight: light struct missing in '%s'"), *NewNav.SourceFile);
				continue;
			}

			if (NewNav.Beacons.Num() >= MaxLights)
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseNavlight: too many lights in '%s' (max=%d)"),
					*NewNav.SourceFile, MaxLights);
				continue;
			}

			TermStruct* LightStruct = PDef->term()->isStruct();

			FVector Loc = FVector::ZeroVector;
			int32   T = 1;      // legacy default before clamp
			DWORD   Pattern = 0;

			const int32 LightElemCount = (int32)LightStruct->elements()->size();
			for (int32 j = 0; j < LightElemCount; ++j)
			{
				TermDef* P2 = LightStruct->elements()->at(j)->isDef();
				if (!P2)
					continue;

				const Text& LKey = P2->name()->value();

				if (LKey == "type")
				{
					GetDefNumber(T, P2, Fn);
				}
				else if (LKey == "loc")
				{
					GetDefVec(Loc, P2, Fn);
				}
				else if (LKey == "pattern")
				{
					GetDefNumber(Pattern, P2, Fn);
				}
			}

			FNavLightBeacon Beacon;
			Beacon.Type = NavLightTypeFromLegacyInt(T);

			// Legacy: bloc[n] = loc * scale (ship scale, not navlight dscale)
			Beacon.Location = Loc * ShipScale;

			Beacon.Pattern = (int32)Pattern;

			NewNav.Beacons.Add(Beacon);
		}
	}

	// Store like your other parse helpers:
	NewShipNavLightArray.Add(NewNav);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseFlightDeck(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseFlightDeck()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseFlightDeck called with null args"));
		return;
	}

	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	Text  DName = "";
	Text  DAbrv = "";
	Text  DesignName = "";

	float Az = 0.0f;
	int32 ExplosionType = 0;

	bool  bLaunch = false;
	bool  bRecovery = false;

	FVector Loc = FVector::ZeroVector;
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	FVector Cam = FVector::ZeroVector;
	FVector Box = FVector::ZeroVector;

	float CycleTime = 0.0f;
	float Size = 0.0f;
	float Hull = 0.5f;
	float Light = 0.0f;

	FShipFlightDeck NewDeck;
	NewDeck.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// Caps (legacy uses fixed arrays: slots[10], filters[10], runway[2], approach[NUM_APPROACH_PTS])
	const int32 MaxSlots = 10;
	const int32 MaxRunwayPts = 2;
	const int32 MaxApproachPts = 8; // TODO: set to FlightDeck::NUM_APPROACH_PTS from legacy

	NewDeck.Slots.Reserve(MaxSlots);
	NewDeck.RunwayPoints.Reserve(MaxRunwayPts);
	NewDeck.ApproachPoints.Reserve(MaxApproachPts);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();
		Text Buf;

		if (Key == "name")
		{
			GetDefText(DName, PDef, Fn);
			NewDeck.Name = FString(DName);
		}
		else if (Key == "abrv")
		{
			GetDefText(DAbrv, PDef, Fn);
			NewDeck.Abbrev = FString(DAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewDeck.DesignName = FString(DesignName);
		}
		else if (Key == "start")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;
			NewDeck.StartPoint = V;
		}
		else if (Key == "end")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;
			NewDeck.EndPoint = V;
		}
		else if (Key == "cam")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;
			NewDeck.CamLocation = V;
		}
		else if (Key == "box" || Key == "bounding_box")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;
			NewDeck.BoundingBox = V;
		}
		else if (Key == "approach")
		{
			if (NewDeck.ApproachPoints.Num() < MaxApproachPts)
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, PDef, Fn);
				V *= Scale;
				NewDeck.ApproachPoints.Add(V);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseFlightDeck: approach point ignored in '%s' (max=%d)"),
					*NewDeck.SourceFile, MaxApproachPts);
			}
		}
		else if (Key == "runway")
		{
			if (NewDeck.RunwayPoints.Num() < MaxRunwayPts)
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, PDef, Fn);
				V *= Scale;
				NewDeck.RunwayPoints.Add(V);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseFlightDeck: extra runway point ignored in '%s' (max=%d)"),
					*NewDeck.SourceFile, MaxRunwayPts);
			}
		}
		else if (Key == "spot")
		{
			if (NewDeck.Slots.Num() >= MaxSlots)
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseFlightDeck: too many slots in '%s' (max=%d)"),
					*NewDeck.SourceFile, MaxSlots);
				continue;
			}

			FFlightDeckSlot Slot;

			if (PDef->term() && PDef->term()->isStruct())
			{
				TermStruct* S = PDef->term()->isStruct();

				FVector SpotLoc = FVector::ZeroVector;
				DWORD Filter = 0xF;

				const int32 SCnt = (int32)S->elements()->size();
				for (int32 j = 0; j < SCnt; ++j)
				{
					TermDef* D = S->elements()->at(j)->isDef();
					if (!D) continue;

					const Text& SKey = D->name()->value();

					if (SKey == "loc")
					{
						GetDefVec(SpotLoc, D, Fn);
					}
					else if (SKey == "filter")
					{
						GetDefNumber(Filter, D, Fn);
					}
				}

				SpotLoc *= Scale;

				Slot.Location = SpotLoc;
				Slot.FilterMask = (int32)Filter;
				NewDeck.Slots.Add(Slot);
			}
			else if (PDef->term() && PDef->term()->isArray())
			{
				FVector SpotLoc = FVector::ZeroVector;
				GetDefVec(SpotLoc, PDef, Fn);
				SpotLoc *= Scale;

				Slot.Location = SpotLoc;
				Slot.FilterMask = 0xF; // legacy default
				NewDeck.Slots.Add(Slot);
			}
		}
		else if (Key == "light")
		{
			GetDefNumber(Light, PDef, Fn);
			NewDeck.Light = Light;
		}
		else if (Key == "cycle_time")
		{
			GetDefNumber(CycleTime, PDef, Fn);
			NewDeck.CycleTime = CycleTime;
		}
		else if (Key == "launch")
		{
			GetDefBool(bLaunch, PDef, Fn);
			NewDeck.bLaunchDeck = bLaunch;
		}
		else if (Key == "recovery")
		{
			GetDefBool(bRecovery, PDef, Fn);
			NewDeck.bRecoveryDeck = bRecovery;
		}
		else if (Key == "azimuth")
		{
			GetDefNumber(Az, PDef, Fn);

			// If you have a legacy "degrees" flag, apply it here.
			// Otherwise leave in radians.
			// if (bLegacyAnglesAreDegrees) Az = FMath::DegreesToRadians(Az);

			NewDeck.AzimuthRadians = Az;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= Scale;
			NewDeck.Location = V;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= Scale;
			NewDeck.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewDeck.HullFactor = Hull;
		}
		else if (Key == "explosion")
		{
			GetDefNumber(ExplosionType, PDef, Fn);
			NewDeck.ExplosionType = ExplosionType;
		}
	}

	// Legacy mutually exclusive behavior:
	// if (launch) SetLaunchDeck(); else if (recovery) SetRecoveryDeck();
	if (NewDeck.bLaunchDeck && NewDeck.bRecoveryDeck)
	{
		// Keep legacy priority: launch wins
		NewDeck.bRecoveryDeck = false;
	}

	NewShipFlightDeckArray.Add(NewDeck);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseLandingGear(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseLandingGear()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseLandingGear called with null args"));
		return;
	}

	const float Scale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	Text DName = "";
	Text DAbrv = "";
	Text DesignName = "";

	FShipLandingGear NewGear;
	NewGear.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// Legacy cap:
	const int32 MaxGear = 8; // TODO: set to LandingGear::MAX_GEAR from legacy
	NewGear.GearItems.Reserve(MaxGear);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(DName, PDef, Fn);
			NewGear.Name = FString(DName);
		}
		else if (Key == "abrv")
		{
			GetDefText(DAbrv, PDef, Fn);
			NewGear.Abbrev = FString(DAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewGear.DesignName = FString(DesignName);
		}
		else if (Key == "gear")
		{
			if (!PDef->term() || !PDef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseLandingGear: gear struct missing in '%s'"), *NewGear.SourceFile);
				continue;
			}

			if (NewGear.GearItems.Num() >= MaxGear)
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseLandingGear: too many gear entries in '%s' (max=%d)"),
					*NewGear.SourceFile, MaxGear);
				continue;
			}

			TermStruct* GearStruct = PDef->term()->isStruct();

			Text ModelName = "";
			FVector V1 = FVector::ZeroVector;
			FVector V2 = FVector::ZeroVector;

			const int32 GearElemCount = (int32)GearStruct->elements()->size();
			for (int32 j = 0; j < GearElemCount; ++j)
			{
				TermDef* GDef = GearStruct->elements()->at(j)->isDef();
				if (!GDef)
					continue;

				const Text& GKey = GDef->name()->value();

				if (GKey == "model")
				{
					GetDefText(ModelName, GDef, Fn);
				}
				else if (GKey == "start")
				{
					GetDefVec(V1, GDef, Fn);
				}
				else if (GKey == "end")
				{
					GetDefVec(V2, GDef, Fn);
				}
			}

			FLandingGearItem Item;
			Item.ModelName = FString(ModelName);

			// Legacy: start/end scaled
			Item.Start = V1 * Scale;
			Item.End = V2 * Scale;

			// In legacy, if model fails to load, it skips the gear entry.
			// Since we are parsing-only, we keep it but warn if model name missing:
			if (Item.ModelName.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseLandingGear: gear entry missing model name in '%s'"), *NewGear.SourceFile);
			}

			NewGear.GearItems.Add(Item);
		}
	}

	// Store like your other parse helpers:
	NewShipLandingGearArray.Add(NewGear);
}

// +--------------------------------------------------------------------+

static void SetAimIfValid(float InVal, bool& bHas, float& Out)
{
	bHas = true;
	Out = InVal;
}

static void ResolveWeaponMuzzlesInPlace(FShipWeapon& Weapon, const FWeaponDesignMeta& Meta, const float ShipScale)
{
	if (Weapon.bMuzzlesResolved)
		return;

	const float ScaleToApply = Meta.bIsTurret ? Meta.WeaponScale : ShipScale;

	for (FVector& M : Weapon.Muzzles)
	{
		M *= ScaleToApply;
	}

	Weapon.bMuzzlesResolved = true;
	Weapon.bMuzzlesAreInShipSpace = !Meta.bIsTurret;
	Weapon.bMuzzlesWereAuthoredInShipSpace = !Meta.bIsTurret;
}

static int32 FindWeaponIndexByType(const TArray<FShipWeapon>& Weapons, const FString& WantedType)
{
	if (WantedType.IsEmpty())
		return INDEX_NONE;

	for (int32 i = 0; i < Weapons.Num(); ++i)
	{
		if (Weapons[i].WeaponType.Equals(WantedType, ESearchCase::IgnoreCase))
			return i;
	}
	return INDEX_NONE;
}

void UStarshatterShipDesignSubsystem::ParseWeapon(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseWeapon()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseWeapon called with null args"));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy locals/defaults:
	Text WType = "";
	Text WName = "";
	Text WAbrv = "";
	Text DesignName = "";
	Text GroupName = "";

	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   Hull = 0.5f;

	float   Az = 0.0f;
	float   El = 0.0f;

	float   AzMax = 1e6f;
	float   AzMin = 1e6f;
	float   ElMax = 1e6f;
	float   ElMin = 1e6f;
	float   AzRest = 1e6f;
	float   ElRest = 1e6f;

	int32   ExplosionType = 0;
	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	FShipWeapon NewWpn;
	NewWpn.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 MaxBarrels = 8; // TODO: replace with Weapon::MAX_BARRELS from legacy
	NewWpn.Muzzles.Reserve(MaxBarrels);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			GetDefText(WType, PDef, Fn);
			NewWpn.WeaponType = FString(WType);
		}
		else if (Key == "name")
		{
			GetDefText(WName, PDef, Fn);
			NewWpn.Name = FString(WName);
		}
		else if (Key == "abrv")
		{
			GetDefText(WAbrv, PDef, Fn);
			NewWpn.Abbrev = FString(WAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewWpn.DesignName = FString(DesignName);
		}
		else if (Key == "group")
		{
			GetDefText(GroupName, PDef, Fn);
			NewWpn.GroupName = FString(GroupName);
		}
		else if (Key == "muzzle")
		{
			if (NewWpn.Muzzles.Num() < MaxBarrels)
			{
				FVector M = FVector::ZeroVector;
				GetDefVec(M, PDef, Fn);

				// IMPORTANT: stored raw; scaled in ResolveWeaponsForCurrentShip based on meta:
				NewWpn.Muzzles.Add(M);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseWeapon: too many muzzles in '%s' (max=%d)"),
					*NewWpn.SourceFile, MaxBarrels);
			}
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewWpn.Location = V;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewWpn.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewWpn.HullFactor = Hull;
		}
		else if (Key == "azimuth")
		{
			GetDefNumber(Az, PDef, Fn);
			// if (bAnglesInDegrees) Az = FMath::DegreesToRadians(Az);
			NewWpn.AzimuthRadians = Az;
		}
		else if (Key == "elevation")
		{
			GetDefNumber(El, PDef, Fn);
			// if (bAnglesInDegrees) El = FMath::DegreesToRadians(El);
			NewWpn.ElevationRadians = El;
		}
		else if (Key == "aim_az_max")
		{
			GetDefNumber(AzMax, PDef, Fn);
			// if (bAnglesInDegrees) AzMax = FMath::DegreesToRadians(AzMax);
			AzMin = 0.0f - AzMax; // legacy
		}
		else if (Key == "aim_el_max")
		{
			GetDefNumber(ElMax, PDef, Fn);
			// if (bAnglesInDegrees) ElMax = FMath::DegreesToRadians(ElMax);
			ElMin = 0.0f - ElMax; // legacy
		}
		else if (Key == "aim_az_min")
		{
			GetDefNumber(AzMin, PDef, Fn);
			// if (bAnglesInDegrees) AzMin = FMath::DegreesToRadians(AzMin);
		}
		else if (Key == "aim_el_min")
		{
			GetDefNumber(ElMin, PDef, Fn);
			// if (bAnglesInDegrees) ElMin = FMath::DegreesToRadians(ElMin);
		}
		else if (Key == "aim_az_rest" || Key == "rest_azimuth")
		{
			GetDefNumber(AzRest, PDef, Fn);
			// if (bAnglesInDegrees) AzRest = FMath::DegreesToRadians(AzRest);
		}
		else if (Key == "aim_el_rest" || Key == "rest_elevation")
		{
			GetDefNumber(ElRest, PDef, Fn);
			// if (bAnglesInDegrees) ElRest = FMath::DegreesToRadians(ElRest);
		}
		else if (Key == "explosion")
		{
			GetDefNumber(ExplosionType, PDef, Fn);
			NewWpn.ExplosionType = ExplosionType;
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewWpn.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewWpn.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewWpn.Emcon3 = ClampEmcon(Emcon3);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ParseWeapon: unknown weapon parameter '%s' in '%s'"),
				ANSI_TO_TCHAR(Key.data()), ANSI_TO_TCHAR(Fn));
		}
	}

	// Aim sentinel -> flags:
	if (AzMax < 1e6f) { SetAimIfValid(AzMax, NewWpn.Aim.bHasAzMax, NewWpn.Aim.AzMax); }
	if (AzMin < 1e6f) { SetAimIfValid(AzMin, NewWpn.Aim.bHasAzMin, NewWpn.Aim.AzMin); }
	if (AzRest < 1e6f) { SetAimIfValid(AzRest, NewWpn.Aim.bHasAzRest, NewWpn.Aim.AzRest); }

	if (ElMax < 1e6f) { SetAimIfValid(ElMax, NewWpn.Aim.bHasElMax, NewWpn.Aim.ElMax); }
	if (ElMin < 1e6f) { SetAimIfValid(ElMin, NewWpn.Aim.bHasElMin, NewWpn.Aim.ElMin); }
	if (ElRest < 1e6f) { SetAimIfValid(ElRest, NewWpn.Aim.bHasElRest, NewWpn.Aim.ElRest); }

	// Reactor index:
	NewWpn.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	// Store parsed:
	NewShipWeaponArray.Add(NewWpn);
}

void UStarshatterShipDesignSubsystem::ResolveWeaponsForCurrentShip()
{
	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Reset per-ship resolved indices
	CurrentShipDecoyWeaponIndex = INDEX_NONE;
	CurrentShipProbeWeaponIndex = INDEX_NONE;

	// ----------------------------------------------------
	// PASS 1: resolve muzzles + legacy meta fallback
	// ----------------------------------------------------
	for (int32 i = 0; i < NewShipWeaponArray.Num(); ++i)
	{
		FShipWeapon& Wpn = NewShipWeaponArray[i];

		const FWeaponDesignMeta* Meta = WeaponMetaByType.Find(Wpn.WeaponType);
		if (!Meta)
			continue;

		ResolveWeaponMuzzlesInPlace(Wpn, *Meta, ShipScale);

		// Legacy behavior: first meta-marked decoy/probe wins
		if (Meta->bIsDecoy && CurrentShipDecoyWeaponIndex == INDEX_NONE)
		{
			CurrentShipDecoyWeaponIndex = i;
		}
		else if (Meta->bIsProbe && CurrentShipProbeWeaponIndex == INDEX_NONE)
		{
			CurrentShipProbeWeaponIndex = i;
		}
	}

	// ----------------------------------------------------
	// PASS 2: explicit ship-level override + validation
	// ----------------------------------------------------
	auto FindWeaponIndexByType =
		[](const TArray<FShipWeapon>& Weapons, const FString& WantedType) -> int32
		{
			if (WantedType.IsEmpty())
				return INDEX_NONE;

			for (int32 i = 0; i < Weapons.Num(); ++i)
			{
				if (Weapons[i].WeaponType.Equals(WantedType, ESearchCase::IgnoreCase))
					return i;
			}
			return INDEX_NONE;
		};

	// ---- DEC0Y ----
	const int32 ExplicitDecoy =
		FindWeaponIndexByType(NewShipWeaponArray, CurrentShipDecoyWeaponType);

	if (CurrentShipDecoyWeaponType.Len() && ExplicitDecoy == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Ship declares decoy='%s' but no weapon with that WeaponType exists (file='%s')"),
			*CurrentShipDecoyWeaponType,
			*CurrentShipSourceFile);
	}
	else if (ExplicitDecoy != INDEX_NONE)
	{
		CurrentShipDecoyWeaponIndex = ExplicitDecoy;
	}

	// ---- PROBE ----
	const int32 ExplicitProbe =
		FindWeaponIndexByType(NewShipWeaponArray, CurrentShipProbeWeaponType);

	if (CurrentShipProbeWeaponType.Len() && ExplicitProbe == INDEX_NONE)
	{
		const FString FileForLog = CurrentShipSourceFile.Len() ? CurrentShipSourceFile : TEXT("UNKNOWN");
		UE_LOG(LogTemp, Warning,
			TEXT("Ship declares decoy='%s' but no weapon with that WeaponType exists (file='%s')"),
			*CurrentShipDecoyWeaponType,
			*FileForLog);
	}
	else if (ExplicitProbe != INDEX_NONE)
	{
		CurrentShipProbeWeaponIndex = ExplicitProbe;
	}
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseHardPoint(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseHardPoint()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseHardPoint called with null args"));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy locals/defaults:
	Text  WName = "";
	Text  WAbrv = "";
	Text  Design = "";

	FVector Muzzle = FVector::ZeroVector;
	FVector Loc = FVector::ZeroVector;
	float   Size = 0.0f;
	float   Hull = 0.5f;
	float   Az = 0.0f;
	float   El = 0.0f;

	int32   Emcon1 = -1;
	int32   Emcon2 = -1;
	int32   Emcon3 = -1;

	FShipHardPoint NewHP;
	NewHP.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// Legacy cap:
	const int32 MaxTypes = 8;
	NewHP.AllowedWeaponTypes.Reserve(MaxTypes);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "type")
		{
			if (NewHP.AllowedWeaponTypes.Num() < MaxTypes)
			{
				Text WType = "";
				if (GetDefText(WType, PDef, Fn))
				{
					NewHP.AllowedWeaponTypes.Add(FString(WType));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseHardPoint: too many weapon types in '%s' (max=%d)"),
					*NewHP.SourceFile, MaxTypes);
			}
		}
		else if (Key == "name")
		{
			GetDefText(WName, PDef, Fn);
			NewHP.Name = FString(WName);
		}
		else if (Key == "abrv")
		{
			GetDefText(WAbrv, PDef, Fn);
			NewHP.Abbrev = FString(WAbrv);
		}
		else if (Key == "design")
		{
			GetDefText(Design, PDef, Fn);
			NewHP.DesignName = FString(Design);
		}
		else if (Key == "muzzle")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewHP.Muzzle = V;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewHP.Location = V;
		}
		else if (Key == "size")
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewHP.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewHP.HullFactor = Hull;
		}
		else if (Key == "azimuth")
		{
			GetDefNumber(Az, PDef, Fn);
			// if (bAnglesInDegrees) Az = FMath::DegreesToRadians(Az);
			NewHP.AzimuthRadians = Az;
		}
		else if (Key == "elevation")
		{
			GetDefNumber(El, PDef, Fn);
			// if (bAnglesInDegrees) El = FMath::DegreesToRadians(El);
			NewHP.ElevationRadians = El;
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewHP.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewHP.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewHP.Emcon3 = ClampEmcon(Emcon3);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ParseHardPoint: unknown parameter '%s' in '%s'"),
				ANSI_TO_TCHAR(Key.data()), ANSI_TO_TCHAR(Fn));
		}
	}

	NewShipHardPointArray.Add(NewHP);
}

void UStarshatterShipDesignSubsystem::ValidateLoadoutsForCurrentShip() const
{
	for (const FShipLoadout& L : NewShipLoadoutArray)
	{
		if (L.Stations.Num() > 16)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Loadout '%s' has %d stations (max=16)"),
				*L.Name, L.Stations.Num());
		}
	}
}
// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseLoadout(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseLoadout()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseLoadout called with null args"));
		return;
	}

	FShipLoadout NewLoadout;
	NewLoadout.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 MaxStations = 16;
	NewLoadout.Stations.Reserve(MaxStations);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			Text NameBuf;
			if (GetDefText(NameBuf, PDef, Fn))
			{
				NewLoadout.Name = FString(NameBuf);
			}
		}
		else if (Key == "stations")
		{
			// Legacy: GetDefArray(int*, max, ...)
			// We emulate this safely using a temp buffer.
			int32 Temp[MaxStations];
			FMemory::Memzero(Temp, sizeof(Temp));

			const int32 Count = GetDefArray(Temp, MaxStations, PDef, Fn);
			for (int32 i = 0; i < Count; ++i)
			{
				NewLoadout.Stations.Add(Temp[i]);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("ParseLoadout: unknown parameter '%s' in '%s'"),
				ANSI_TO_TCHAR(Key.data()), ANSI_TO_TCHAR(Fn));
		}
	}

	NewShipLoadoutArray.Add(NewLoadout);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseSensor(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseSensor()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSensor called with null args"));
		return;
	}

	// Legacy: only one sensor
	if (NewShipSensorArray.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSensor: additional sensor ignored in '%s'"), ANSI_TO_TCHAR(Fn));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	Text DesignName = "";
	float Size = 0.0f;
	float Hull = 0.5f;
	int32 Emcon1 = -1;
	int32 Emcon2 = -1;
	int32 Emcon3 = -1;

	FShipSensor NewSensor;
	NewSensor.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 MaxRanges = 8;
	NewSensor.Ranges.Reserve(MaxRanges);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "range")
		{
			if (NewSensor.Ranges.Num() < MaxRanges)
			{
				float R = 0.0f;
				GetDefNumber(R, PDef, Fn);
				NewSensor.Ranges.Add(R);
			}
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewSensor.Location = V;
		}
		else if (Key == "size")
		{
			// Fix legacy ordering: read then scale
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewSensor.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewSensor.HullFactor = Hull;
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewSensor.DesignName = FString(DesignName);
		}
		else if (Key == "emcon_1")
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewSensor.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key == "emcon_2")
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewSensor.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key == "emcon_3")
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewSensor.Emcon3 = ClampEmcon(Emcon3);
		}
	}

	NewSensor.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	NewShipSensorArray.Add(NewSensor);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseNavsys(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseNavsys()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseNavsys called with null args"));
		return;
	}

	// Legacy: only one nav system
	if (NewShipNavSystemArray.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseNavsys: additional nav system ignored in '%s'"), ANSI_TO_TCHAR(Fn));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	Text DesignName = "";
	float Size = 0.0f;
	float Hull = 0.5f;

	FShipNavSystem NewNav;
	NewNav.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewNav.Location = V;
		}
		else if (Key == "size")
		{
			// Fix legacy ordering: read then scale
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewNav.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewNav.HullFactor = Hull;
		}
		else if (Key == "design")
		{
			GetDefText(DesignName, PDef, Fn);
			NewNav.DesignName = FString(DesignName);
		}
	}

	NewNav.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	NewShipNavSystemArray.Add(NewNav);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseComputer(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseComputer()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseComputer called with null args"));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	Text CompName("Computer");
	Text CompAbrv("Comp");
	Text DesignName;
	int32 CompType = 1;

	float Size = 0.0f;
	float Hull = 0.5f;

	FShipComputer NewComp;
	NewComp.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			if (GetDefText(CompName, PDef, Fn))
			{
				NewComp.Name = FString(CompName);
			}
		}
		else if (Key == "abrv")
		{
			if (GetDefText(CompAbrv, PDef, Fn))
			{
				NewComp.Abbrev = FString(CompAbrv);
			}
		}
		else if (Key == "design")
		{
			if (GetDefText(DesignName, PDef, Fn))
			{
				NewComp.DesignName = FString(DesignName);
			}
		}
		else if (Key == "type")
		{
			GetDefNumber(CompType, PDef, Fn);
			NewComp.Type = CompType;
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewComp.Location = V;
		}
		else if (Key == "size")
		{
			// Fix legacy ordering: read then scale
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewComp.Size = Size;
		}
		else if (Key == "hull_factor")
		{
			GetDefNumber(Hull, PDef, Fn);
			NewComp.HullFactor = Hull;
		}
	}

	NewComp.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	NewShipComputerArray.Add(NewComp);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseShield(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseShield()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseShield called with null args"));
		return;
	}

	// Legacy: only one shield allowed
	if (NewShipShieldArray.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseShield: additional shield ignored in '%s'"), ANSI_TO_TCHAR(Fn));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy defaults:
	Text DName;
	Text DAbrv;
	Text DesignName;
	Text ModelName;

	double Factor = 0.0;
	double Capacity = 0.0;
	double Consumption = 0.0;
	double Cutoff = 0.0;
	double Curve = 0.0;
	double DefCost = 1.0;

	int32 ShieldType = 0;

	FVector Loc = FVector::ZeroVector;
	float Size = 0.0f;
	float Hull = 0.5f;

	int32 ExplosionType = 0;
	bool bCapacitor = false;
	bool bBubble = false;

	int32 Emcon1 = -1;
	int32 Emcon2 = -1;
	int32 Emcon3 = -1;

	Text BoltHitSound;
	Text BeamHitSound;

	FShipShield NewShield;
	NewShield.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		// We need "contains" behavior -> convert to FString (lowered)
		const FString Key = ANSI_TO_TCHAR(PDef->name()->value().data());

		if (Key.Equals(TEXT("type"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(ShieldType, PDef, Fn);
			NewShield.ShieldType = ShieldType;
		}
		else if (Key.Equals(TEXT("name"), ESearchCase::IgnoreCase))
		{
			GetDefText(DName, PDef, Fn);
			NewShield.Name = FString(DName);
		}
		else if (Key.Equals(TEXT("abrv"), ESearchCase::IgnoreCase))
		{
			GetDefText(DAbrv, PDef, Fn);
			NewShield.Abbrev = FString(DAbrv);
		}
		else if (Key.Equals(TEXT("design"), ESearchCase::IgnoreCase))
		{
			GetDefText(DesignName, PDef, Fn);
			NewShield.DesignName = FString(DesignName);
		}
		else if (Key.Equals(TEXT("model"), ESearchCase::IgnoreCase))
		{
			GetDefText(ModelName, PDef, Fn);
			NewShield.ModelName = FString(ModelName);
		}
		else if (Key.Equals(TEXT("loc"), ESearchCase::IgnoreCase))
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, PDef, Fn);
			V *= ShipScale;
			NewShield.Location = V;
		}
		else if (Key.Equals(TEXT("size"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Size, PDef, Fn);
			Size *= ShipScale;
			NewShield.Size = Size;
		}
		else if (Key.Equals(TEXT("hull_factor"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Hull, PDef, Fn);
			NewShield.HullFactor = Hull;
		}
		// Legacy: defname.contains("factor"/"cutoff"/"curve"/"capacitor"/"bubble")
		else if (Key.Contains(TEXT("factor"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Factor, PDef, Fn);
			NewShield.Factor = Factor;
		}
		else if (Key.Contains(TEXT("cutoff"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Cutoff, PDef, Fn);
			NewShield.Cutoff = Cutoff;
		}
		else if (Key.Contains(TEXT("curve"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Curve, PDef, Fn);
			NewShield.Curve = Curve;
		}
		else if (Key.Contains(TEXT("capacitor"), ESearchCase::IgnoreCase))
		{
			GetDefBool(bCapacitor, PDef, Fn);
			NewShield.bShieldCapacitor = bCapacitor;
		}
		else if (Key.Contains(TEXT("bubble"), ESearchCase::IgnoreCase))
		{
			GetDefBool(bBubble, PDef, Fn);
			NewShield.bShieldBubble = bBubble;
		}
		else if (Key.Equals(TEXT("capacity"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Capacity, PDef, Fn);
			NewShield.Capacity = Capacity;
		}
		else if (Key.Equals(TEXT("consumption"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Consumption, PDef, Fn);
			NewShield.Consumption = Consumption;
		}
		else if (Key.Equals(TEXT("deflection_cost"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(DefCost, PDef, Fn);
			NewShield.DeflectionCost = DefCost;
		}
		else if (Key.Equals(TEXT("explosion"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(ExplosionType, PDef, Fn);
			NewShield.ExplosionType = ExplosionType;
		}
		else if (Key.Equals(TEXT("emcon_1"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Emcon1, PDef, Fn);
			NewShield.Emcon1 = ClampEmcon(Emcon1);
		}
		else if (Key.Equals(TEXT("emcon_2"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Emcon2, PDef, Fn);
			NewShield.Emcon2 = ClampEmcon(Emcon2);
		}
		else if (Key.Equals(TEXT("emcon_3"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(Emcon3, PDef, Fn);
			NewShield.Emcon3 = ClampEmcon(Emcon3);
		}
		else if (Key.Equals(TEXT("bolt_hit_sound"), ESearchCase::IgnoreCase))
		{
			GetDefText(BoltHitSound, PDef, Fn);
			NewShield.BoltHitSound = FString(BoltHitSound);
		}
		else if (Key.Equals(TEXT("beam_hit_sound"), ESearchCase::IgnoreCase))
		{
			GetDefText(BeamHitSound, PDef, Fn);
			NewShield.BeamHitSound = FString(BeamHitSound);
		}
	}

	// Source index from last reactor:
	NewShield.SourceIndex = (NewShipPowerArray.Num() > 0) ? (NewShipPowerArray.Num() - 1) : INDEX_NONE;

	// Legacy validation: shield_type must be non-zero
	if (NewShield.ShieldType <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseShield: invalid shield type in '%s'"), *NewShield.SourceFile);
		return;
	}

	NewShipShieldArray.Add(NewShield);
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseSquadron(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseSquadron()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSquadron called with null args"));
		return;
	}

	// Legacy defaults:
	Text NameBuf = "";
	Text DesignBuf = "";
	int32 Count = 4;
	int32 Avail = 4;

	FShipSquadron NewSq;
	NewSq.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			if (GetDefText(NameBuf, PDef, Fn))
			{
				NewSq.Name = FString(NameBuf);
			}
		}
		else if (Key == "design")
		{
			if (GetDefText(DesignBuf, PDef, Fn))
			{
				NewSq.DesignName = FString(DesignBuf);
			}
		}
		else if (Key == "count")
		{
			GetDefNumber(Count, PDef, Fn);
			NewSq.Count = Count;
		}
		else if (Key == "avail")
		{
			GetDefNumber(Avail, PDef, Fn);
			NewSq.Avail = Avail;
		}
	}

	// Optional safety (legacy doesn’t clamp, but this prevents garbage):
	if (NewSq.Count < 0) NewSq.Count = 0;
	if (NewSq.Avail < 0) NewSq.Avail = 0;
	if (NewSq.Avail > NewSq.Count) NewSq.Avail = NewSq.Count;

	NewShipSquadronArray.Add(NewSq);
}

static void EnsureExplosionIndex(TArray<FExplosion>& Arr, int32 Index)
{
	while (Arr.Num() <= Index)
	{
		Arr.Add(FExplosion{});
	}
}

static void EnsureDebrisIndex(TArray<FDebris>& Arr, int32 Index)
{
	while (Arr.Num() <= Index)
	{
		Arr.Add(FDebris{});
	}
}

void UStarshatterShipDesignSubsystem::ParseDeathSpiral(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseDeathSpiral()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseDeathSpiral called with null args"));
		return;
	}

	// Legacy: one block (usually). Enforce if you want:
	if (NewShipDeathSpiralArray.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseDeathSpiral: additional block ignored in '%s'"), ANSI_TO_TCHAR(Fn));
		return;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	FShipDeathSpiral NewDS;
	NewDS.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	// ----------------------------
	// Backward compat: explosion
	// ----------------------------
	bool bHasCompatExplosion = false;
	FExplosion CompatExp;

	// ----------------------------
	// Backward compat: debris
	// ----------------------------
	bool bHasCompatDebris = false;
	FDebris CompatDeb;
	CompatDeb.FireLocations.Reserve(5);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const FString Key = ANSI_TO_TCHAR(Def->name()->value().data());

		// --------------------------------------------------------------------
		// time
		// --------------------------------------------------------------------
		if (Key.Equals(TEXT("time"), ESearchCase::IgnoreCase))
		{
			GetDefNumber(NewDS.Time, Def, Fn);
		}

		// --------------------------------------------------------------------
		// explosion (new format)
		// --------------------------------------------------------------------
		else if (Key.Equals(TEXT("explosion"), ESearchCase::IgnoreCase))
		{
			if (Def->term() && Def->term()->isStruct())
			{
				FExplosion Step = ParseExplosion(Def->term()->isStruct(), Fn);
				NewDS.Explosions.Add(Step);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseDeathSpiral: explosion struct missing in '%s'"), *NewDS.SourceFile);
			}
		}

		// --------------------------------------------------------------------
		// explosion (backward compat fields)
		// --------------------------------------------------------------------
		else if (Key.Equals(TEXT("explosion_type"), ESearchCase::IgnoreCase))
		{
			if (bHasCompatExplosion)
			{
				NewDS.Explosions.Add(CompatExp);
				CompatExp = FExplosion{};
			}

			bHasCompatExplosion = true;
			GetDefNumber(CompatExp.Type, Def, Fn);
		}
		else if (Key.Equals(TEXT("explosion_time"), ESearchCase::IgnoreCase))
		{
			bHasCompatExplosion = true;
			GetDefNumber(CompatExp.Time, Def, Fn);
		}
		else if (Key.Equals(TEXT("explosion_loc"), ESearchCase::IgnoreCase))
		{
			bHasCompatExplosion = true;

			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, Fn);
			V *= ShipScale;

			CompatExp.Location = V;
		}
		else if (Key.Equals(TEXT("final_type"), ESearchCase::IgnoreCase))
		{
			if (bHasCompatExplosion)
			{
				NewDS.Explosions.Add(CompatExp);
				CompatExp = FExplosion{};
			}

			bHasCompatExplosion = true;
			GetDefNumber(CompatExp.Type, Def, Fn);
			CompatExp.bFinal = true;
		}
		else if (Key.Equals(TEXT("final_loc"), ESearchCase::IgnoreCase))
		{
			bHasCompatExplosion = true;

			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, Fn);
			V *= ShipScale;

			CompatExp.Location = V;
		}

		// --------------------------------------------------------------------
		// debris (new format): debris { ... } OR debris "ModelName"
		// --------------------------------------------------------------------
		else if (Key.Equals(TEXT("debris"), ESearchCase::IgnoreCase))
		{
			// If we were building a compat debris, commit it before starting a new debris record:
			if (bHasCompatDebris)
			{
				NewDS.Debris.Add(CompatDeb);
				CompatDeb = FDebris{};
				CompatDeb.FireLocations.Reserve(5);
				bHasCompatDebris = false;
			}

			if (Def->term() && Def->term()->isStruct())
			{
				FDebris Step = ParseDebris(Def->term()->isStruct(), Fn);
				NewDS.Debris.Add(Step);
			}
			else if (Def->term() && Def->term()->isText())
			{
				// Text form: debris "ModelName"
				FDebris Step;
				Text ModelBuf;
				GetDefText(ModelBuf, Def, Fn);
				Step.ModelName = FString(ModelBuf);
				NewDS.Debris.Add(Step);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseDeathSpiral: debris struct missing in '%s'"), *NewDS.SourceFile);
			}
		}

		// --------------------------------------------------------------------
		// debris (backward compat fields)
		// These apply to the most recently-started debris entry.
		// We'll build CompatDeb and commit it at end (or when a new debris starts).
		// --------------------------------------------------------------------
		else if (Key.Equals(TEXT("debris_mass"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.Mass, Def, Fn);
		}
		else if (Key.Equals(TEXT("debris_speed"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.Speed, Def, Fn);
		}
		else if (Key.Equals(TEXT("debris_drag"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.Drag, Def, Fn);
		}
		else if (Key.Equals(TEXT("debris_loc"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;

			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, Fn);
			V *= ShipScale;

			CompatDeb.Location = V;
		}
		else if (Key.Equals(TEXT("debris_count"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.Count, Def, Fn);
		}
		else if (Key.Equals(TEXT("debris_life"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.Life, Def, Fn);
		}
		else if (Key.Equals(TEXT("debris_fire"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;

			if (CompatDeb.FireLocations.Num() < 5)
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, Def, Fn);
				V *= ShipScale;
				CompatDeb.FireLocations.Add(V);
			}
		}
		else if (Key.Equals(TEXT("debris_fire_type"), ESearchCase::IgnoreCase))
		{
			bHasCompatDebris = true;
			GetDefNumber(CompatDeb.FireType, Def, Fn);
		}
	}

	// Commit compat explosion if in progress:
	if (bHasCompatExplosion)
	{
		NewDS.Explosions.Add(CompatExp);
	}

	// Commit compat debris if in progress:
	if (bHasCompatDebris)
	{
		NewDS.Debris.Add(CompatDeb);
	}

	NewShipDeathSpiralArray.Add(NewDS);
}

// +--------------------------------------------------------------------+

FExplosion UStarshatterShipDesignSubsystem::ParseExplosion(TermStruct* Val, const char* Fn)
{
	FExplosion NewExp;

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseExplosion called with null args"));
		return NewExp;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const Text& Key = Def->name()->value();

		if (Key == "time")
		{
			GetDefNumber(NewExp.Time, Def, Fn);
		}
		else if (Key == "type")
		{
			GetDefNumber(NewExp.Type, Def, Fn);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, Fn);
			V *= ShipScale;
			NewExp.Location = V;
		}
		else if (Key == "final")
		{
			bool b = false;
			GetDefBool(b, Def, Fn);
			NewExp.bFinal = b;
		}
	}

	return NewExp;
}

// +--------------------------------------------------------------------+

FDebris UStarshatterShipDesignSubsystem::ParseDebris(TermStruct* Val, const char* Fn)
{
	FDebris NewDeb;

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseDebris called with null args"));
		return NewDeb;
	}

	const float ShipScale = (CurrentShipScale > 0.0f) ? CurrentShipScale : 1.0f;

	// Legacy: fire_index < 5
	NewDeb.FireLocations.Reserve(5);

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const Text& Key = Def->name()->value();

		if (Key == "model")
		{
			Text ModelBuf;
			if (GetDefText(ModelBuf, Def, Fn))
			{
				NewDeb.ModelName = FString(ModelBuf);
			}
		}
		else if (Key == "mass")
		{
			GetDefNumber(NewDeb.Mass, Def, Fn);
		}
		else if (Key == "speed")
		{
			GetDefNumber(NewDeb.Speed, Def, Fn);
		}
		else if (Key == "drag")
		{
			GetDefNumber(NewDeb.Drag, Def, Fn);
		}
		else if (Key == "loc")
		{
			FVector V = FVector::ZeroVector;
			GetDefVec(V, Def, Fn);
			V *= ShipScale;
			NewDeb.Location = V;
		}
		else if (Key == "count")
		{
			GetDefNumber(NewDeb.Count, Def, Fn);
		}
		else if (Key == "life")
		{
			GetDefNumber(NewDeb.Life, Def, Fn);
		}
		else if (Key == "fire")
		{
			if (NewDeb.FireLocations.Num() < 5)
			{
				FVector V = FVector::ZeroVector;
				GetDefVec(V, Def, Fn);
				V *= ShipScale;
				NewDeb.FireLocations.Add(V);
			}
		}
		else if (Key == "fire_type")
		{
			GetDefNumber(NewDeb.FireType, Def, Fn);
		}
	}

	// Optional safety (legacy doesn’t clamp, but prevents nonsense):
	if (NewDeb.Count < 0) NewDeb.Count = 0;

	return NewDeb;
}

// +--------------------------------------------------------------------+

void UStarshatterShipDesignSubsystem::ParseMap(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMap()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseMap called with null args"));
		return;
	}

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* PDef = Val->elements()->at(ElemIdx)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "sprite")
		{
			Text SpriteText;
			if (GetDefText(SpriteText, PDef, Fn))
			{
				FShipMapSprite NewSprite;
				NewSprite.SpriteName = FString(SpriteText);
				NewSprite.SourceFile = FString(ANSI_TO_TCHAR(Fn));

				NewShipMapSpriteArray.Add(NewSprite);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseMap: invalid/missing sprite in '%s'"), ANSI_TO_TCHAR(Fn));
			}
		}
	}
}

// +--------------------------------------------------------------------+

static FSkinMtlCell ParseSkinMtlCell(TermStruct* Val, const char* Fn)
{
	FSkinMtlCell Out;

	if (!Val || !Fn || !*Fn)
	{
		return Out;
	}

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const Text& Key = Def->name()->value();

		if (Key == "index" || Key == "cell" || Key == "id")
		{
			GetDefNumber(Out.CellIndex, Def, Fn);
		}
		else if (Key == "material" || Key == "mtl" || Key == "name")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.MaterialName = FString(Buf);
			}
		}
		else if (Key == "texture" || Key == "bitmap" || Key == "diffuse" || Key == "map")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.TextureName = FString(Buf);
			}
		}
		else if (Key == "aux" || Key == "normal" || Key == "spec" || Key == "specular")
		{
			Text Buf;
			if (GetDefText(Buf, Def, Fn))
			{
				Out.AuxTextureName = FString(Buf);
			}
		}
	}

	return Out;
}

void UStarshatterShipDesignSubsystem::ParseSkin(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseSkin()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSkin called with null args"));
		return;
	}

	FShipSkin NewSkin;
	NewSkin.SourceFile = FString(ANSI_TO_TCHAR(Fn));

	bool bHasName = false;

	const int32 ElemCount = (int32)Val->elements()->size();
	for (int32 ElemIdx = 0; ElemIdx < ElemCount; ++ElemIdx)
	{
		TermDef* Def = Val->elements()->at(ElemIdx)->isDef();
		if (!Def)
			continue;

		const Text& Key = Def->name()->value();

		if (Key == "name")
		{
			Text NameBuf;
			if (GetDefText(NameBuf, Def, Fn))
			{
				NewSkin.Name = FString(NameBuf);
				bHasName = true;
			}
		}
		else if (Key == "material" || Key == "mtl")
		{
			if (!Def->term() || !Def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("ParseSkin: skin material struct missing in '%s'"), ANSI_TO_TCHAR(Fn));
				continue;
			}

			FSkinMtlCell Cell = ParseSkinMtlCell(Def->term()->isStruct(), Fn);
			NewSkin.Cells.Add(Cell);
		}
	}

	// Legacy: if (skin && skin->NumCells()) skins.append(skin);
	if (!bHasName)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSkin: missing name in '%s' (skin ignored)"), ANSI_TO_TCHAR(Fn));
		return;
	}

	if (NewSkin.Cells.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseSkin: skin '%s' has no materials in '%s' (ignored)"), *NewSkin.Name, ANSI_TO_TCHAR(Fn));
		return;
	}

	NewShipSkinArray.Add(NewSkin);
}

