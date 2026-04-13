/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024-2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           StarshatterEnvironmentSubsystem.cpp
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    Implementation skeleton for UStarshatterEnvironmentSubsystem.

    NOTE:
    Parsing and hydration logic will be copied from the existing
    UStarshatterGameDataSubsystem. This file intentionally provides the
    lifecycle, load flow, and safe guardrails without duplicating parsing.
=============================================================================*/

#include "StarshatterEnvironmentSubsystem.h"

// Core
#include "Logging/LogMacros.h"

// Engine / file helpers
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"

// Legacy parsing / registry headers
#include "DataLoader.h"
#include "ParseUtil.h"
#include "FormatUtil.h"
#include "Text.h"
#include "Term.h"
#include "Galaxy.h"
#include "StarSystem.h"

#include "StarSystemRegistry.h"

#include "SSWGameInstance.h"

#include "Engine/DataTable.h"
#include "FormattingUtils.h"
// Project

#include "GameStructs.h"
#include "GameStructs_System.h"

#include "StarshatterAssetRegistrySubsystem.h"

DEFINE_LOG_CATEGORY(LogStarshatterEnvironment);

// -----------------------------------------------------------------------------
// UStarshatterEnvironmentSubsystem
// -----------------------------------------------------------------------------

template<typename TRowStruct>
static void ReadTableToArray(
	const UDataTable* Table,
	TArray<TRowStruct>& OutArray,
	const TCHAR* Label)
{
	OutArray.Reset();

	if (!Table)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] %s is null."), Label);
		return;
	}

	if (Table->GetRowStruct() != TRowStruct::StaticStruct())
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] %s row struct mismatch. Expected=%s Actual=%s"),
			Label,
			*GetNameSafe(TRowStruct::StaticStruct()),
			*GetNameSafe(Table->GetRowStruct()));
		return;
	}

	static const FString Context(TEXT("ReadTableToArray"));
	const TArray<FName> RowNames = Table->GetRowNames();

	OutArray.Reserve(RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const TRowStruct* Row =
			Table->FindRow<TRowStruct>(RowName, Context, false);

		if (Row)
		{
			OutArray.Add(*Row);
		}
	}

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Read %d rows from %s."),
		OutArray.Num(), Label);
}


static FColor Vec3ToColor255(const Vec3& a)
{
	return FColor(UFormattingUtils::ToByteClamp(a.X), UFormattingUtils::ToByteClamp(a.Y), UFormattingUtils::ToByteClamp(a.Z), 255);
}

void UStarshatterEnvironmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogStarshatterEnvironment, Log, TEXT("[Environment] Initialize"));

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] Initialize: GameInstance is null"));
		return;
	}

	SSWInstance = Cast<USSWGameInstance>(GI);

	UStarshatterAssetRegistrySubsystem* Assets = GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
	if (!Assets)
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] Initialize: AssetRegistry subsystem missing"));
		return;
	}

	SetProjectPath();

	ResolveDataTables();

	if (Assets)
	{
		UE_LOG(LogStarshatterEnvironment, Warning,
			TEXT("[Environment] DTs -> StarSystems=%s Regions=%s"),
			*GetNameSafe(GalaxyDataTable),
			*GetNameSafe(RegionsDataTable));
	}

	// Ensure we actually have a data table to fill:
	if (!GalaxyDataTable)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Environment] GalaxyDataTable is null. Assign a DT asset with RowStruct=FS_Galaxy in defaults."));
		return;
	}


	// Runtime time state
	bBaseTimeInitialized = false;
	EnvironmentBaseTime = 0.0;
	SimulationClockMs = 0;

	// Runtime object caches
	RuntimeStarSystems.Reset();
	RuntimeStars.Reset();
	RuntimePlanets.Reset();
	RuntimeMoons.Reset();
	RuntimeRegions.Reset();

	if (bLoaded)
	{
		UE_LOG(LogStarshatterEnvironment, Warning,
			TEXT("[Environment] LoadAll forced reload (previous state detected)"));

		Unload(); // force clean state
	}
}

void UStarshatterEnvironmentSubsystem::Deinitialize()
{
    UE_LOG(LogStarshatterEnvironment, Log, TEXT("[Environment] Deinitialize"));

    Unload();
    Super::Deinitialize();
}

void UStarshatterEnvironmentSubsystem::ResolveDataTables()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI) return;

	UStarshatterAssetRegistrySubsystem* Assets = GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
	if (!Assets) return;

	GalaxyDataTable = Assets->GetDataTable(TEXT("Data.GalaxyMapTable"), true);
	RegionsDataTable = Assets->GetDataTable(TEXT("Data.RegionsTable"), true);
}

void UStarshatterEnvironmentSubsystem::Unload()
{
	ReleaseAssets();

	bLoaded = false;

	// Reset time state
	bBaseTimeInitialized = false;
	EnvironmentBaseTime = 0.0;
	SimulationClockMs = 0;

	// Reset ALL runtime caches (not just systems)
	RuntimeStarSystems.Reset();
	RuntimeStars.Reset();
	RuntimePlanets.Reset();
	RuntimeMoons.Reset();
	RuntimeRegions.Reset();

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Unload complete: Systems=%d Stars=%d Planets=%d Moons=%d Regions=%d"),
		RuntimeStarSystems.Num(),
		RuntimeStars.Num(),
		RuntimePlanets.Num(),
		RuntimeMoons.Num(),
		RuntimeRegions.Num());
}

void UStarshatterEnvironmentSubsystem::ReleaseAssets()
{
	// Only if you truly want to drop hard refs (usually Deinitialize)
	GalaxyDataTable = nullptr;
	StarsDataTable = nullptr;
	PlanetsDataTable = nullptr;
	MoonsDataTable = nullptr;
	RegionsDataTable = nullptr;
	TerrainRegionsDataTable = nullptr;

	// Legacy list
	//systems.clear();

	FilePath.Reset();

	UE_LOG(LogStarshatterEnvironment, Verbose, TEXT("[Environment] Releaae Assets"));
}

void UStarshatterEnvironmentSubsystem::GetSSWInstance()
{
	SSWInstance = (USSWGameInstance*)GetGameInstance();
}

void UStarshatterEnvironmentSubsystem::SetProjectPath()
{
	ProjectPath = FPaths::ProjectDir();
	ProjectPath.Append(TEXT("GameData/"));

	UE_LOG(LogTemp, Log, TEXT("Setting Game Data Directory %s"), *ProjectPath);
}

FString UStarshatterEnvironmentSubsystem::GetProjectPath()
{
	return ProjectPath;
}

void UStarshatterEnvironmentSubsystem::LoadAll(bool bFull /*= false*/)
{
	if (bLoaded)
	{
		UE_LOG(LogStarshatterEnvironment, Verbose,
			TEXT("[Environment] LoadAll skipped (already loaded)"));
		return;
	}

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] LoadAll (Full=%s)"),
		bFull ? TEXT("true") : TEXT("false"));

	ClearRuntimeCaches();
	ResolveDataTables();
	//LoadGalaxyMap();
	//LoadStarsystems();
	CreateEnvironmentTables();

	Galaxy::InitializeFromEnvironment(this);

	// Initialize global runtime base time ONCE and propagate to live systems
	InitSimulationBaseTime();
	ResetSimulationClock();

	bLoaded = true;

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] LoadAll complete: DT_Galaxies=%d DT_Systems=%d RuntimeSystems=%d RuntimeStars=%d RuntimePlanets=%d RuntimeMoons=%d RuntimeRegions=%d Terrain=%d Zones=%d"),
		GalaxyDataArray.Num(),
		StarSystemDataArray.Num(),
		RuntimeStarSystems.Num(),
		RuntimeStars.Num(),
		RuntimePlanets.Num(),
		RuntimeMoons.Num(),
		RuntimeRegions.Num(),
		TerrainRegionsArray.Num(),
		ZoneDataArray.Num());
}

void UStarshatterEnvironmentSubsystem::InitSimulationBaseTime()
{
	if (bBaseTimeInitialized)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Environment] BaseTime already initialized: %f"),
			EnvironmentBaseTime);
		return;
	}

	const FDateTime UtcNow = FDateTime::UtcNow();
	const FDateTime UnixEpoch(1970, 1, 1);
	const FTimespan SinceEpoch = UtcNow - UnixEpoch;

	EnvironmentBaseTime = SinceEpoch.GetTotalSeconds();
	bBaseTimeInitialized = true;

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] BaseTime initialized from UE clock: %f"),
		EnvironmentBaseTime);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] RuntimeStarSystems.Num() = %d"),
		RuntimeStarSystems.Num());

	int32 Count = 0;

	for (StarSystem* System : RuntimeStarSystems)
	{
		if (!System)
			continue;

		System->SetBaseTime(EnvironmentBaseTime, true);
		StarSystem::CalcStardate();
		++Count;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] BaseTime propagated to %d systems"),
		Count);
}

void UStarshatterEnvironmentSubsystem::RegisterStarSystem(StarSystem* System)
{
	if (!System)
		return;

	RuntimeStarSystems.AddUnique(System);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered StarSystem. Count=%d"),
		RuntimeStarSystems.Num());

	// If base time already initialized, apply immediately
	if (bBaseTimeInitialized)
	{
		System->SetBaseTime(EnvironmentBaseTime, true);
		StarSystem::CalcStardate();
	}
}


void UStarshatterEnvironmentSubsystem::CreateEnvironmentTables()
{
	UE_LOG(LogStarshatterEnvironment, Log, TEXT("[Environment] CreateEnvironmentTables"));

	HydrateAllFromTables();

	// Clear any prior runtime registry ownership before rebuild:
	StarSystemRegistry::Clear(true);

	// Clear local runtime arrays before registering fresh objects:
	RuntimeStarSystems.Reset();
	RuntimeStars.Reset();
	RuntimePlanets.Reset();
	RuntimeMoons.Reset();
	RuntimeRegions.Reset();

	// Build runtime StarSystem hierarchy from hydrated FS_Galaxy rows:
	for (const FS_Galaxy& GalaxyRow : GalaxyDataArray)
	{
		StarSystem* RuntimeSystem =
			StarSystemRegistry::BuildAndRegister(GalaxyRow, this);

		if (!RuntimeSystem)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[Environment] Failed to build runtime StarSystem for '%s'"),
				*GalaxyRow.Name);
			continue;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("===== RUNTIME STAR SYSTEM CHECK ====="));
	UE_LOG(LogTemp, Warning,
		TEXT("RuntimeSystems=%d Stars=%d Planets=%d Moons=%d Regions=%d"),
		RuntimeStarSystems.Num(),
		RuntimeStars.Num(),
		RuntimePlanets.Num(),
		RuntimeMoons.Num(),
		RuntimeRegions.Num());

	for (StarSystem* System : RuntimeStarSystems)
	{
		if (!System)
		{
			UE_LOG(LogTemp, Error, TEXT("Runtime system is null"));
			continue;
		}

		UE_LOG(LogTemp, Warning,
			TEXT("System: %s  Loc=(%.2f, %.2f, %.2f)  Radius=%.2f"),
			ANSI_TO_TCHAR(System->GetName()),
			System->GetLocation().X,
			System->GetLocation().Y,
			System->GetLocation().Z,
			System->GetRadius());
	}
}

// +--------------------------------------------------------------------+

// ------------------------------------------------------------
// FIXED: LoadGalaxyMap
// - UE-native file load (no DataLoader / no ReleaseBuffer)
// - Correct per-system reset (system scratch arrays cleared once per system)
// - Removes undefined fn/filename usage; uses a single Fn derived from FileName
// - Uses your ParseStarMap -> ParsePlanetMap -> ParseMoonMap chain
// ------------------------------------------------------------

void UStarshatterEnvironmentSubsystem::LoadGalaxyMap()
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterEnvironmentSubsystem::LoadGalaxyMap()"));

	const FString Dir = FPaths::ProjectContentDir() / TEXT("GameData/Galaxy/");
	const FString FileName = Dir / TEXT("Galaxy.def");

	UE_LOG(LogTemp, Log, TEXT("[Environment] Loading Galaxy: %s"), *FileName);

	if (!FPaths::FileExists(FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] Galaxy file not found: %s"), *FileName);
		return;
	}

	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *FileName))
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] Failed to read galaxy file: %s"), *FileName);
		return;
	}
	Bytes.Add(0); // null-terminate for BlockReader

	const char* Fn = TCHAR_TO_ANSI(*FileName);

	if (!SSWInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] SSWInstance is null in LoadGalaxyMap()"));
		return;
	}


	// Ensure RowStruct is correct:
	if (GalaxyDataTable->GetRowStruct() == nullptr)
	{
		GalaxyDataTable->RowStruct = FS_Galaxy::StaticStruct();
	}
	else if (GalaxyDataTable->GetRowStruct() != FS_Galaxy::StaticStruct())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Environment] GalaxyDataTable RowStruct mismatch. Expected %s, got %s"),
			*FS_Galaxy::StaticStruct()->GetName(),
			*GalaxyDataTable->GetRowStruct()->GetName());
		return;
	}

	// Clear output containers:
	GalaxyDataArray.Empty();
	GalaxyDataTable->EmptyTable();

	// Scratch arrays:
	StarMapArray.Empty();
	PlanetMapArray.Empty();
	MoonMapArray.Empty();
	RegionMapArray.Empty();

	Parser parser(new BlockReader((const char*)Bytes.GetData()));
	Term* term = parser.ParseTerm();

	if (!term)
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING: could not parse '%s'"), *FileName);
		return;
	}

	TermText* file_type = term->isText();
	if (!file_type || file_type->value() != "GALAXY")
	{
		UE_LOG(LogTemp, Warning, TEXT("WARNING: invalid galaxy file '%s' (missing GALAXY header)"), *FileName);
		delete term;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Galaxy file OK: %s"), *FileName);

	double GalaxyRadius = 0.0;
	int32 SystemsParsed = 0;

	while (true)
	{
		delete term;
		term = parser.ParseTerm();
		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def)
			continue;

		const Text& DefName = def->name()->value();

		// global radius:
		if (DefName == "radius")
		{
			GetDefNumber(GalaxyRadius, def, Fn);
			continue;
		}

		// system:
		if (DefName == "system")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: system struct missing in '%s'"), *FileName);
				continue;
			}

			// Reset per system (scratch):
			StarMapArray.Empty();
			PlanetMapArray.Empty();
			MoonMapArray.Empty();
			RegionMapArray.Empty();

			FS_Galaxy NewGalaxyData;
			NewGalaxyData.Link.Empty();

			TermStruct* sys = def->term()->isStruct();

			Text  SystemName = "";
			Text  ClassName = "";
			Text  Link = "";
			Text  StarName = "";
			FVector SystemLocation{};
			int   SystemIff = 0;
			int   EmpireId = 0;

			ESPECTRAL_CLASS StarClass = ESPECTRAL_CLASS::G;

			for (int i = 0; i < sys->elements()->size(); i++)
			{
				TermDef* pdef = sys->elements()->at(i)->isDef();
				if (!pdef) continue;

				const Text& Key = pdef->name()->value();

				if (Key == "name")
				{
					GetDefText(SystemName, pdef, Fn);
					NewGalaxyData.Name = FString(SystemName);
				}
				else if (Key == "loc")
				{
					GetDefVec(SystemLocation, pdef, Fn);
					NewGalaxyData.Location = FVector(SystemLocation.X, SystemLocation.Y, SystemLocation.Z);
				}
				else if (Key == "iff")
				{
					GetDefNumber(SystemIff, pdef, Fn);
					NewGalaxyData.Iff = SystemIff;
				}
				else if (Key == "empire")
				{
					GetDefNumber(EmpireId, pdef, Fn);
					NewGalaxyData.Empire = UFormattingUtils::GetEmpireTypeFromIndex(EmpireId);
				}
				else if (Key == "link")
				{
					GetDefText(Link, pdef, Fn);
					NewGalaxyData.Link.Add(FString(Link));
				}
				else if (Key == "star")
				{
					GetDefText(StarName, pdef, Fn);
					NewGalaxyData.Star = FString(StarName);
				}
				else if (Key == "class")
				{
					GetDefText(ClassName, pdef, Fn);

					switch (ClassName[0])
					{
					case 'B': StarClass = ESPECTRAL_CLASS::B;           break;
					case 'A': StarClass = ESPECTRAL_CLASS::A;           break;
					case 'F': StarClass = ESPECTRAL_CLASS::F;           break;
					case 'G': StarClass = ESPECTRAL_CLASS::G;           break;
					case 'K': StarClass = ESPECTRAL_CLASS::K;           break;
					case 'M': StarClass = ESPECTRAL_CLASS::M;           break;
					case 'R': StarClass = ESPECTRAL_CLASS::RED_GIANT;   break;
					case 'W': StarClass = ESPECTRAL_CLASS::WHITE_DWARF; break;
					case 'Z': StarClass = ESPECTRAL_CLASS::BLACK_HOLE;  break;
					default:  StarClass = ESPECTRAL_CLASS::G;           break;
					}
					NewGalaxyData.Class = StarClass;
				}
				else if (Key == "stellar")
				{
					if (!pdef->term() || !pdef->term()->isStruct())
					{
						UE_LOG(LogTemp, Warning, TEXT("WARNING: stellar struct missing in '%s'"), *FileName);
					}
					else
					{
						ParseStarMap(pdef->term()->isStruct(), Fn);
						NewGalaxyData.Stellar = StarMapArray;
					}
				}
			}

			// HARD VALIDATION before adding:
			if (NewGalaxyData.Name.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("Parsed a system with empty name. Skipping row."));
				continue;
			}

			// Ensure unique row names (duplicates overwrite in many workflows):
			FString BaseRow = NewGalaxyData.Name;
			FString RowStr = BaseRow;
			int32 Suffix = 1;

			while (GalaxyDataTable->FindRow<FS_Galaxy>(*RowStr, TEXT("LoadGalaxyMap"), false) != nullptr)
			{
				RowStr = FString::Printf(TEXT("%s_%d"), *BaseRow, Suffix++);
			}

			const FName RowName(*RowStr);

			GalaxyDataTable->AddRow(RowName, NewGalaxyData);
			GalaxyDataArray.Add(NewGalaxyData);

			SystemsParsed++;

			UE_LOG(LogTemp, Log, TEXT("Added system row: %s | Stars=%d"),
				*RowStr, NewGalaxyData.Stellar.Num());
		}
	}

	delete term; // defensive

	UE_LOG(LogTemp, Log, TEXT("Galaxy parse complete. SystemsParsed=%d  GalaxyData.Num=%d  DataTableRows=%d"),
		SystemsParsed,
		GalaxyDataArray.Num(),
		GalaxyDataTable->GetRowMap().Num());

	//UGalaxyManager::Get(this)->LoadGalaxy(GalaxyDataArray);
}


// +--------------------------------------------------------------------+

void UStarshatterEnvironmentSubsystem::ParseStar(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseStar()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseStar called with invalid args"));
		return;
	}

	Text   StarName = "";
	Text   ImgName = "";
	Text   MapName = "";

	double Light = 0.0;
	double Radius = 0.0;
	double Rot = 0.0;
	double Mass = 0.0;
	double Orbit = 0.0;
	double Tscale = 1.0;

	bool   Retro = false;

	FS_Star NewStarData;

	// IMPORTANT: Only clear this if it is truly a per-star scratch array.
	// If you support multiple stars per system and aggregate planets elsewhere,
	// keep this as-is. Otherwise remove the Empty().
	PlanetDataArray.Empty();

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(StarName, pdef, fn);
			NewStarData.Name = FString(StarName);
		}
		else if (Key == "map" || Key == "icon")
		{
			GetDefText(MapName, pdef, fn);
			NewStarData.Map = FString(MapName);
		}
		else if (Key == "image")
		{
			GetDefText(ImgName, pdef, fn);
			NewStarData.Image = FString(ImgName);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewStarData.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewStarData.Orbit = Orbit;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewStarData.Radius = Radius;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewStarData.Rot = Rot;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewStarData.Tscale = Tscale;
		}
		else if (Key == "light")
		{
			GetDefNumber(Light, pdef, fn);
			NewStarData.Light = Light;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewStarData.Retro = Retro;
		}
		else if (Key == "color")
		{
			Vec3 v;
			GetDefVec(v, pdef, fn);

			// v is legacy 0..255 most of the time. Clamp and cast safely:
			const uint8 R = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.X), 0, 255);
			const uint8 G = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.Y), 0, 255);
			const uint8 B = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.Z), 0, 255);

			NewStarData.Color = FColor(R, G, B, 255);
		}
		else if (Key == "back" || Key == "back_color")
		{
			Vec3 v;
			GetDefVec(v, pdef, fn);

			const uint8 R = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.X), 0, 255);
			const uint8 G = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.Y), 0, 255);
			const uint8 B = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)v.Z), 0, 255);

			NewStarData.Back = FColor(R, G, B, 255);
		}
		else if (Key == "planet")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: planet struct missing in '%s'"), *FString(fn));
			}
			else
			{
				ParsePlanet(pdef->term()->isStruct(), fn);

				// ParsePlanet appends to PlanetDataArray; snapshot it into this star:
				NewStarData.Planet = PlanetDataArray;
			}
		}
	}

	StarDataArray.Add(NewStarData);
}

// +--------------------------------------------------------------------+

void UStarshatterEnvironmentSubsystem::ParsePlanet(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParsePlanet()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParsePlanet called with invalid args"));
		return;
	}

	Text   PlanetName = "";
	Text   ImgName = "";
	Text   MapName = "";
	Text   HiName = "";
	Text   ImgRing = "";
	Text   GloName = "";
	Text   GloHiName = "";
	Text   GlossName = "";

	double Radius = 0.0;
	double Mass = 0.0;
	double Orbit = 0.0;
	double Rot = 0.0;
	double Minrad = 0.0;
	double Maxrad = 0.0;
	double Tscale = 1.0;
	double Tilt = 0.0;

	bool   Retro = false;
	bool   Lumin = false;

	FS_Planet NewPlanetData;

	// per-planet scratch array
	MoonDataArray.Empty();

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(PlanetName, pdef, fn);
			NewPlanetData.Name = FString(PlanetName);
		}
		else if (Key == "map" || Key == "icon")
		{
			GetDefText(MapName, pdef, fn);
			NewPlanetData.Map = FString(MapName);
		}
		else if (Key == "image" || Key == "image_west" || Key == "image_east")
		{
			GetDefText(ImgName, pdef, fn);
			NewPlanetData.Image = FString(ImgName);
		}
		else if (Key == "glow")
		{
			GetDefText(GloName, pdef, fn);
			NewPlanetData.Glow = FString(GloName);
		}
		else if (Key == "gloss")
		{
			GetDefText(GlossName, pdef, fn);
			NewPlanetData.Gloss = FString(GlossName);
		}
		else if (Key == "high_res" || Key == "high_res_west" || Key == "high_res_east")
		{
			GetDefText(HiName, pdef, fn);
			NewPlanetData.High = FString(HiName);
		}
		else if (Key == "glow_high_res")
		{
			GetDefText(GloHiName, pdef, fn);
			NewPlanetData.GlowHigh = FString(GloHiName);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewPlanetData.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewPlanetData.Orbit = Orbit;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewPlanetData.Retro = Retro;
		}
		else if (Key == "luminous")
		{
			GetDefBool(Lumin, pdef, fn);
			NewPlanetData.Lumin = Lumin;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewPlanetData.Rot = Rot;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewPlanetData.Radius = Radius;
		}
		else if (Key == "ring")
		{
			GetDefText(ImgRing, pdef, fn);
			NewPlanetData.Rings = FString(ImgRing);
		}
		else if (Key == "minrad")
		{
			GetDefNumber(Minrad, pdef, fn);
			NewPlanetData.Minrad = Minrad;
		}
		else if (Key == "maxrad")
		{
			GetDefNumber(Maxrad, pdef, fn);
			NewPlanetData.Maxrad = Maxrad;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewPlanetData.Tscale = Tscale;
		}
		else if (Key == "tilt")
		{
			GetDefNumber(Tilt, pdef, fn);
			NewPlanetData.Tilt = Tilt;
		}
		else if (Key == "atmosphere")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);

			// Legacy Vec3 is typically 0..255. Clamp & cast to bytes.
			const uint8 R = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)a.X), 0, 255);
			const uint8 G = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)a.Y), 0, 255);
			const uint8 B = (uint8)FMath::Clamp((int32)FMath::RoundToInt((float)a.Z), 0, 255);

			NewPlanetData.Atmos = FColor(R, G, B, 255);
		}
		else if (Key == "moon")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: moon struct missing in '%s'"), *FString(fn));
			}
			else
			{
				ParseMoon(pdef->term()->isStruct(), fn);

				// ParseMoon appends into MoonDataArray; snapshot it:
				NewPlanetData.Moon = MoonDataArray;
			}
		}
	}

	PlanetDataArray.Add(NewPlanetData);
}

// +--------------------------------------------------------------------+

void UStarshatterEnvironmentSubsystem::ParseMoon(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMoon()"));

	if (!val || !fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseMoon called with invalid args"));
		return;
	}

	Text   MapName = "";
	Text   MoonName = "";
	Text   ImgName = "";
	Text   HiName = "";
	Text   GloName = "";
	Text   GloHiName = "";
	Text   GlossName = "";

	double Radius = 0.0;
	double Mass = 0.0;
	double Orbit = 0.0;
	double Rot = 0.0;
	double Tscale = 1.0;
	double Tilt = 0.0;
	bool   Retro = false;

	FS_Moon NewMoonData;

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef)
			continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(MoonName, pdef, fn);
			NewMoonData.Name = FString(MoonName);
		}
		else if (Key == "map" || Key == "icon")
		{
			GetDefText(MapName, pdef, fn);
			NewMoonData.Map = FString(MapName);
		}
		else if (Key == "image")
		{
			GetDefText(ImgName, pdef, fn);
			NewMoonData.Image = FString(ImgName);
		}
		else if (Key == "glow")
		{
			GetDefText(GloName, pdef, fn);
			NewMoonData.Glow = FString(GloName);
		}
		else if (Key == "high_res")
		{
			GetDefText(HiName, pdef, fn);
			NewMoonData.High = FString(HiName);
		}
		else if (Key == "glow_high_res")
		{
			GetDefText(GloHiName, pdef, fn);
			NewMoonData.GlowHigh = FString(GloHiName);
		}
		else if (Key == "gloss")
		{
			GetDefText(GlossName, pdef, fn);
			NewMoonData.Gloss = FString(GlossName);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewMoonData.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewMoonData.Orbit = Orbit;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewMoonData.Rot = Rot;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewMoonData.Retro = Retro;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewMoonData.Radius = Radius;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewMoonData.Tscale = Tscale;
		}
		else if (Key == "inclination")
		{
			GetDefNumber(Tilt, pdef, fn);
			NewMoonData.Tilt = Tilt;
		}
		else if (Key == "atmosphere")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);

			// Assume legacy Vec3 is 0..255; clamp + cast.
			const uint8 R = (uint8)FMath::Clamp(FMath::RoundToInt((float)a.X), 0, 255);
			const uint8 G = (uint8)FMath::Clamp(FMath::RoundToInt((float)a.Y), 0, 255);
			const uint8 B = (uint8)FMath::Clamp(FMath::RoundToInt((float)a.Z), 0, 255);

			NewMoonData.Atmos = FColor(R, G, B, 255);
		}
	}

	MoonDataArray.Add(NewMoonData);
}


void UStarshatterEnvironmentSubsystem::ParseRegion(TermStruct* Val, const char* Fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseRegion()"));

	if (!Val || !Fn || !*Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseRegion called with invalid args"));
		return;
	}

	Text RegionName;
	Text RegionParent;
	Text LinkName;
	Text ParentType;

	double Size = 1.0e6;
	double Orbit = 0.0;
	double Grid = 25000.0;
	double Inclination = 0.0;
	int    Asteroids = 0;

	EOrbitalType ParsedType = EOrbitalType::NOTHING;

	TArray<FString> LinksName;
	LinksName.Reserve(8);

	FS_RegionMap NewRegionData;

	// ----------------------------
	// Parse fields (order-independent)
	// ----------------------------
	for (int i = 0; i < Val->elements()->size(); i++)
	{
		TermDef* PDef = Val->elements()->at(i)->isDef();
		if (!PDef)
			continue;

		const Text& Key = PDef->name()->value();

		if (Key == "name")
		{
			GetDefText(RegionName, PDef, Fn);
			NewRegionData.Name = FString(ANSI_TO_TCHAR(RegionName.data())).TrimStartAndEnd();
		}
		else if (Key == "parent")
		{
			GetDefText(RegionParent, PDef, Fn);
			NewRegionData.Parent = FString(ANSI_TO_TCHAR(RegionParent.data())).TrimStartAndEnd();
		}
		else if (Key == "type")
		{
			GetDefText(ParentType, PDef, Fn);

			const FString RawTypeStr = FString(ANSI_TO_TCHAR(ParentType.data())).TrimStartAndEnd();

			ParsedType = EOrbitalType::NOTHING;
			if (UFormattingUtils::GetRegionTypeFromString(*RawTypeStr, ParsedType))
			{
				NewRegionData.Type = ParsedType;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Region type parse failed. Raw='%s' (file=%s)"),
					*RawTypeStr, *FString(Fn));
				NewRegionData.Type = EOrbitalType::NOTHING;
			}
		}
		else if (Key == "link")
		{
			GetDefText(LinkName, PDef, Fn);

			if (LinkName.length() > 0)
			{
				LinksName.Add(FString(ANSI_TO_TCHAR(LinkName.data())).TrimStartAndEnd());
			}
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, PDef, Fn);
			NewRegionData.Orbit = Orbit;
		}
		else if (Key == "size" || Key == "radius")
		{
			GetDefNumber(Size, PDef, Fn);
			NewRegionData.Size = Size;
		}
		else if (Key == "grid")
		{
			GetDefNumber(Grid, PDef, Fn);
			NewRegionData.Grid = Grid;
		}
		else if (Key == "inclination")
		{
			GetDefNumber(Inclination, PDef, Fn);
			NewRegionData.Inclination = Inclination;
		}
		else if (Key == "asteroids")
		{
			// If you only have GetDefNumber, keep this:
			double Temp = 0.0;
			GetDefNumber(Temp, PDef, Fn);
			Asteroids = (int)Temp;

			NewRegionData.Asteroids = Asteroids;
		}
	}

	// Assign links once
	NewRegionData.Link = LinksName;

	// Log using resolved name if available:
	if (!NewRegionData.Name.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("Parsed Region: %s (parent=%s links=%d)"),
			*NewRegionData.Name, *NewRegionData.Parent, NewRegionData.Link.Num());
	}
	// Add to DT_Regions
	if (!NewRegionData.Name.IsEmpty() && RegionsDataTable)
	{
		const FName RowName(*NewRegionData.Name);

		if (RegionsDataTable->GetRowMap().Contains(RowName))
		{
			UE_LOG(LogTemp, Warning, TEXT("DT_Regions already has row '%s' - skipping duplicate"), *RowName.ToString());
		}
		else
		{
			RegionsDataTable->AddRow(RowName, NewRegionData);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseRegion: missing Name or RegionsDataTable is null"));
	}	RegionMapArray.Add(NewRegionData);
}


void UStarshatterEnvironmentSubsystem::ParseMoonMap(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseMoonMap()"));

	Text   MoonIcon = "";
	Text   MoonName = "";
	Text   MoonTexture = "";

	double Radius = 0.0;
	double Mass = 0.0;
	double Orbit = 0.0;
	double Inclination = 0.0;
	double Rot = 0.0;
	double Tscale = 1.0;
	double Tilt = 0.0;
	bool   Retro = false;

	FS_MoonMap NewMoonMap;

	// Reset per-moon:
	RegionMapArray.Empty();

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef) continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(MoonName, pdef, fn);
			NewMoonMap.Name = FString(MoonName);
		}
		else if (Key == "icon")
		{
			GetDefText(MoonIcon, pdef, fn);
			NewMoonMap.Icon = FString(MoonIcon);
		}
		else if (Key == "texture")
		{
			GetDefText(MoonTexture, pdef, fn);
			NewMoonMap.Texture = FString(MoonTexture);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewMoonMap.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewMoonMap.Orbit = Orbit;
		}
		else if (Key == "inclination")
		{
			GetDefNumber(Inclination, pdef, fn);
			NewMoonMap.Inclination = Inclination;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewMoonMap.Rot = Rot;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewMoonMap.Retro = Retro;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewMoonMap.Radius = Radius;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewMoonMap.Tscale = Tscale;
		}
		else if (Key == "tilt") // FIXED (was mistakenly "inclination" again)
		{
			GetDefNumber(Tilt, pdef, fn);
			NewMoonMap.Tilt = Tilt;
		}
		else if (Key == "atmosphere")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);
			NewMoonMap.Atmos = Vec3ToColor255(a);
		}
		else if (Key == "region")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: region struct missing in '%s'"), *FString(fn));
			}
			else
			{
				ParseRegion(pdef->term()->isStruct(), fn);
				NewMoonMap.Region = RegionMapArray;
			}
		}
	}

	MoonMapArray.Add(NewMoonMap);
}

void UStarshatterEnvironmentSubsystem::ParseStarMap(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseStarMap()"));

	Text  StarName = "";
	Text  SystemName = "";
	Text  ImgName = "";
	Text  MapName = "";
	Text  ClassName = "";

	double Light = 0.0;
	double Radius = 0.0;
	double Rot = 0.0;
	double Mass = 0.0;
	double Orbit = 0.0;
	double Tscale = 1.0;
	bool   Retro = false;

	ESPECTRAL_CLASS StarClass = ESPECTRAL_CLASS::G;

	FS_StarMap NewStarMap;

	// Reset per-star:
	PlanetMapArray.Empty();
	RegionMapArray.Empty();

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef) continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(StarName, pdef, fn);
			NewStarMap.Name = FString(StarName);
		}
		else if (Key == "system")
		{
			GetDefText(SystemName, pdef, fn);
			NewStarMap.SystemName = FString(SystemName); // FIXED
		}
		else if (Key == "map")
		{
			GetDefText(MapName, pdef, fn);
			NewStarMap.Map = FString(MapName);
		}
		else if (Key == "image")
		{
			GetDefText(ImgName, pdef, fn);
			NewStarMap.Image = FString(ImgName);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewStarMap.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewStarMap.Orbit = Orbit;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewStarMap.Radius = Radius;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewStarMap.Rot = Rot;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewStarMap.Tscale = Tscale;
		}
		else if (Key == "light")
		{
			GetDefNumber(Light, pdef, fn);
			NewStarMap.Light = Light;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewStarMap.Retro = Retro;
		}
		else if (Key == "color")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);
			NewStarMap.Color = Vec3ToColor255(a);
		}
		else if (Key == "back" || Key == "back_color")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);
			NewStarMap.Back = Vec3ToColor255(a);
		}
		else if (Key == "class")
		{
			GetDefText(ClassName, pdef, fn);

			switch (ClassName[0])
			{
			case 'B': StarClass = ESPECTRAL_CLASS::B;           break;
			case 'A': StarClass = ESPECTRAL_CLASS::A;           break;
			case 'F': StarClass = ESPECTRAL_CLASS::F;           break;
			case 'G': StarClass = ESPECTRAL_CLASS::G;           break;
			case 'K': StarClass = ESPECTRAL_CLASS::K;           break;
			case 'M': StarClass = ESPECTRAL_CLASS::M;           break;
			case 'R': StarClass = ESPECTRAL_CLASS::RED_GIANT;   break;
			case 'W': StarClass = ESPECTRAL_CLASS::WHITE_DWARF; break;
			case 'Z': StarClass = ESPECTRAL_CLASS::BLACK_HOLE;  break;
			default:  StarClass = ESPECTRAL_CLASS::G;           break;
			}

			NewStarMap.Class = StarClass;
		}
		else if (Key == "region")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: region struct missing in '%s'"), *FString(fn));
			}
			else
			{
				ParseRegion(pdef->term()->isStruct(), fn);
				NewStarMap.Region = RegionMapArray;
			}
		}
		else if (Key == "planet")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: planet struct missing in '%s'"), *FString(fn));
			}
			else
			{
				// ParsePlanetMap appends to PlanetMapArray (and moons inside it)
				ParsePlanetMap(pdef->term()->isStruct(), fn);
				NewStarMap.Planet = PlanetMapArray;
			}
		}
	}

	StarMapArray.Add(NewStarMap);
}

void UStarshatterEnvironmentSubsystem::ParsePlanetMap(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParsePlanetMap()"));

	Text   PlanetName = "";
	Text   PlanetIcon = "";
	Text   PlanetRing = "";
	Text   PlanetTexture = "";
	Text   PlanetGloss = "";
	Text   PlanetLights = "";

	double Mass = 0.0;
	double Orbit = 0.0;
	double Inclination = 0.0;
	double Aphelion = 0.0;
	double Perihelion = 0.0;
	double Eccentricity = 0.0;
	double Radius = 0.0;
	double Rot = 0.0;
	double Minrad = 0.0;
	double Maxrad = 0.0;
	double Tscale = 1.0;
	double Tilt = 0.0;

	bool Retro = false;

	FS_PlanetMap NewPlanetMap;

	// Reset per-planet:
	MoonMapArray.Empty();
	RegionMapArray.Empty();

	for (int i = 0; i < val->elements()->size(); i++)
	{
		TermDef* pdef = val->elements()->at(i)->isDef();
		if (!pdef) continue;

		const Text& Key = pdef->name()->value();

		if (Key == "name")
		{
			GetDefText(PlanetName, pdef, fn);
			NewPlanetMap.Name = FString(PlanetName);
		}
		else if (Key == "icon")
		{
			GetDefText(PlanetIcon, pdef, fn);
			NewPlanetMap.Icon = FString(PlanetIcon);
		}
		else if (Key == "texture")
		{
			GetDefText(PlanetTexture, pdef, fn);
			NewPlanetMap.Texture = FString(PlanetTexture);
		}
		else if (Key == "gloss")
		{
			GetDefText(PlanetGloss, pdef, fn);
			NewPlanetMap.Gloss = FString(PlanetGloss);
		}
		else if (Key == "lights")
		{
			GetDefText(PlanetLights, pdef, fn);
			NewPlanetMap.Lights = FString(PlanetLights);
		}
		else if (Key == "ring")
		{
			GetDefText(PlanetRing, pdef, fn);
			NewPlanetMap.Ring = FString(PlanetRing);
		}
		else if (Key == "mass")
		{
			GetDefNumber(Mass, pdef, fn);
			NewPlanetMap.Mass = Mass;
		}
		else if (Key == "orbit")
		{
			GetDefNumber(Orbit, pdef, fn);
			NewPlanetMap.Orbit = Orbit;
		}
		else if (Key == "inclination")
		{
			GetDefNumber(Inclination, pdef, fn);
			NewPlanetMap.Inclination = Inclination;
		}
		else if (Key == "aphelion")
		{
			GetDefNumber(Aphelion, pdef, fn);
			NewPlanetMap.Aphelion = Aphelion;
		}
		else if (Key == "perihelion")
		{
			GetDefNumber(Perihelion, pdef, fn);
			NewPlanetMap.Perihelion = Perihelion;
		}
		else if (Key == "eccentricity")
		{
			GetDefNumber(Eccentricity, pdef, fn);
			NewPlanetMap.Eccentricity = Eccentricity;
		}
		else if (Key == "retro")
		{
			GetDefBool(Retro, pdef, fn);
			NewPlanetMap.Retro = Retro;
		}
		else if (Key == "rotation")
		{
			GetDefNumber(Rot, pdef, fn);
			NewPlanetMap.Rot = Rot;
		}
		else if (Key == "radius")
		{
			GetDefNumber(Radius, pdef, fn);
			NewPlanetMap.Radius = Radius;
		}
		else if (Key == "minrad")
		{
			GetDefNumber(Minrad, pdef, fn);
			NewPlanetMap.Minrad = Minrad;
		}
		else if (Key == "maxrad")
		{
			GetDefNumber(Maxrad, pdef, fn);
			NewPlanetMap.Maxrad = Maxrad;
		}
		else if (Key == "tscale")
		{
			GetDefNumber(Tscale, pdef, fn);
			NewPlanetMap.Tscale = Tscale;
		}
		else if (Key == "tilt")
		{
			GetDefNumber(Tilt, pdef, fn);
			NewPlanetMap.Tilt = Tilt;
		}
		else if (Key == "atmosphere")
		{
			Vec3 a;
			GetDefVec(a, pdef, fn);
			NewPlanetMap.Atmos = Vec3ToColor255(a);
		}
		else if (Key == "moon")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: moon struct missing in '%s'"), *FString(fn));
			}
			else
			{
				// ParseMoonMap appends to MoonMapArray
				ParseMoonMap(pdef->term()->isStruct(), fn);
				NewPlanetMap.Moon = MoonMapArray;
			}
		}
		else if (Key == "region")
		{
			if (!pdef->term() || !pdef->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: region struct missing in '%s'"), *FString(fn));
			}
			else
			{
				ParseRegion(pdef->term()->isStruct(), fn);
				NewPlanetMap.Region = RegionMapArray;
			}
		}
	}

	PlanetMapArray.Add(NewPlanetMap);
}

void UStarshatterEnvironmentSubsystem::ParseTerrain(TermStruct* val, const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseTerrain()"));

	for (int i = 0; i < val->elements()->size(); i++) {
		TermDef* pdef = val->elements()->at(i)->isDef();

		Text   RegionName = "";
		Text   PatchTexture = "";
		Text   NoiseTex0 = "";
		Text   NoiseTex1 = "";
		Text   ApronName = "";
		Text   ApronTexture = "";
		Text   WaterTexture = "";
		Text   EnvTexturePositive_x = "";
		Text   EnvTextureNegative_x = "";
		Text   EnvTexturePositive_y = "";
		Text   EnvTextureNegative_y = "";
		Text   EnvTexturePositive_z = "";
		Text   EnvTextureNegative_z = "";
		Text   HazeName = "";
		Text   SkyName = "";
		Text   CloudsHigh = "";
		Text   CloudsLow = "";
		Text   ShadesHigh = "";
		Text   ShadesLow = "";

		double size = 1.0e6;
		double grid = 25000;
		double inclination = 0.0;
		double scale = 10e3;
		double mtnscale = 1e3;
		double fog_density = 0;
		double fog_scale = 0;
		double haze_fade = 0;
		double clouds_alt_high = 0;
		double clouds_alt_low = 0;
		double w_period = 0;
		double w_chances[EWEATHER_STATE::NUM_STATES];

		if (pdef) {
			if (pdef->name()->value() == "name") {
				GetDefText(RegionName, pdef, fn);
			}
			else if (pdef->name()->value() == "patch" || pdef->name()->value() == "patch_texture") {
				GetDefText(PatchTexture, pdef, fn);
			}
			else if (pdef->name()->value() == "detail_texture_0") {
				GetDefText(NoiseTex0, pdef, fn);
			}
			else if (pdef->name()->value() == "detail_texture_1") {
				GetDefText(NoiseTex1, pdef, fn);
			}
			else if (pdef->name()->value() == "apron") {
				GetDefText(ApronName, pdef, fn);
			}
			else if (pdef->name()->value() == "apron_texture") {
				GetDefText(ApronTexture, pdef, fn);
			}
			else if (pdef->name()->value() == "water_texture") {
				GetDefText(WaterTexture, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_positive_x") {
				GetDefText(EnvTexturePositive_x, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_negative_x") {
				GetDefText(EnvTextureNegative_x, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_positive_y") {
				GetDefText(EnvTexturePositive_y, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_negative_y") {
				GetDefText(EnvTextureNegative_y, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_positive_z") {
				GetDefText(EnvTexturePositive_z, pdef, fn);
			}
			else if (pdef->name()->value() == "env_texture_negative_z") {
				GetDefText(EnvTextureNegative_z, pdef, fn);
			}
			else if (pdef->name()->value() == "clouds_high") {
				GetDefText(CloudsHigh, pdef, fn);
			}
			else if (pdef->name()->value() == "shades_high") {
				GetDefText(ShadesHigh, pdef, fn);
			}
			else if (pdef->name()->value() == "clouds_low") {
				GetDefText(CloudsLow, pdef, fn);
			}
			else if (pdef->name()->value() == "shades_low") {
				GetDefText(ShadesLow, pdef, fn);
			}
			else if (pdef->name()->value() == "haze") {
				GetDefText(HazeName, pdef, fn);
			}
			else if (pdef->name()->value() == "sky_color") {
				GetDefText(SkyName, pdef, fn);
			}
			else if (pdef->name()->value() == "size" || pdef->name()->value() == "radius") {
				GetDefNumber(size, pdef, fn);
			}
			else if (pdef->name()->value() == "grid") {
				GetDefNumber(grid, pdef, fn);
			}
			else if (pdef->name()->value() == "inclination") {
				GetDefNumber(inclination, pdef, fn);
			}
			else if (pdef->name()->value() == "scale") {
				GetDefNumber(scale, pdef, fn);
			}
			else if (pdef->name()->value() == "mtnscale" || pdef->name()->value() == "mtn_scale") {
				GetDefNumber(mtnscale, pdef, fn);
			}
			else if (pdef->name()->value() == "fog_density") {
				GetDefNumber(fog_density, pdef, fn);
			}
			else if (pdef->name()->value() == "fog_scale") {
				GetDefNumber(fog_scale, pdef, fn);
			}
			else if (pdef->name()->value() == "haze_fade") {
				GetDefNumber(haze_fade, pdef, fn);
			}
			else if (pdef->name()->value() == "clouds_alt_high") {
				GetDefNumber(clouds_alt_high, pdef, fn);
			}
			else if (pdef->name()->value() == "clouds_alt_low") {
				GetDefNumber(clouds_alt_low, pdef, fn);
			}
			else if (pdef->name()->value() == "weather_period") {
				GetDefNumber(w_period, pdef, fn);
			}
			else if (pdef->name()->value() == "weather_clear") {
				GetDefNumber(w_chances[0], pdef, fn);
			}
			else if (pdef->name()->value() == "weather_high_clouds") {
				GetDefNumber(w_chances[1], pdef, fn);
			}
			else if (pdef->name()->value() == "weather_moderate_clouds") {
				GetDefNumber(w_chances[2], pdef, fn);
			}
			else if (pdef->name()->value() == "weather_overcast") {
				GetDefNumber(w_chances[3], pdef, fn);
			}
			else if (pdef->name()->value() == "weather_fog") {
				GetDefNumber(w_chances[4], pdef, fn);
			}
			else if (pdef->name()->value() == "weather_storm") {
				GetDefNumber(w_chances[5], pdef, fn);
			}

			else if (pdef->name()->value() == "layer") {
				if (!pdef->term() || !pdef->term()->isStruct()) {
					UE_LOG(LogTemp, Warning,
						TEXT("WARNING: terrain layer struct missing in '%s'"),
						ANSI_TO_TCHAR(fn));
				}
				else {

					//if (!region)
					//	region = new  TerrainRegion(this, rgn_name, size, primary);

					//TermStruct* val = pdef->term()->isStruct();
					//ParseLayer(region, val);
				}
			}
		}
	}
}
// +-------------------------------------------------------------------+

void UStarshatterEnvironmentSubsystem::LoadStarsystems()
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::LoadStarsystems()"));
	ProjectPath = FPaths::ProjectContentDir();
	ProjectPath.Append(TEXT("GameData/Galaxy/Systems/"));
	FString PathName = ProjectPath;

	TArray<FString> output;
	output.Empty();

	FString SysPath = PathName + "*.def";
	FFileManagerGeneric::Get().FindFiles(output, *SysPath, true, false);

	for (int i = 0; i < output.Num(); i++) {

		FString FileName = ProjectPath;
		FileName.Append(output[i]);

		char* fn = TCHAR_TO_ANSI(*FileName);
		UE_LOG(LogTemp, Log, TEXT("Found StarSystem: '%s'"), *FString(FileName));

		ParseStarSystem(fn);
	}
}

// +--------------------------------------------------------------------+

void UStarshatterEnvironmentSubsystem::ParseStarSystem(const char* fn)
{
	UE_LOG(LogTemp, Log, TEXT("UStarshatterGameDataSubsystem::ParseStarSystem()"));

	if (!fn || !*fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("ParseStarSystem called with null/empty filename"));
		return;
	}

	const FString LocalPath = ANSI_TO_TCHAR(fn);

	if (!FPaths::FileExists(LocalPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Star system file not found: %s"), *LocalPath);
		return;
	}

	// UE-native raw bytes load (preserves legacy parser expectations)
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *LocalPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read star system file: %s"), *LocalPath);
		return;
	}
	Bytes.Add(0); // null terminate

	Parser parser(new BlockReader(reinterpret_cast<const char*>(Bytes.GetData())));
	Term* term = parser.ParseTerm();

	if (!term)
	{
		UE_LOG(LogTemp, Warning, TEXT("ERROR: could not parse '%s'"), *LocalPath);
		return;
	}

	// Header check:
	{
		TermText* file_type = term->isText();
		if (!file_type || file_type->value() != "STARSYSTEM")
		{
			UE_LOG(LogTemp, Warning, TEXT("ERROR: invalid star system file '%s'"), *LocalPath);
			delete term;
			return;
		}
	}

	// Locals
	Text  SystemName = "";
	Text  SkyPolyStars = "";
	Text  SkyNebula = "";
	Text  SkyHaze = "";

	int   SkyStars = 0;
	int   SkyDust = 0;

	FColor AmbientColor = FColor::Black;

	FS_StarSystem NewStarSystem;

	// scratch arrays used by nested parsers:
	StarDataArray.Empty();
	PlanetDataArray.Empty();
	MoonDataArray.Empty();
	RegionMapArray.Empty();

	do
	{
		delete term;
		term = parser.ParseTerm();

		if (!term)
			break;

		TermDef* def = term->isDef();
		if (!def)
			continue;

		const Text& Key = def->name()->value();

		if (Key == "name")
		{
			GetDefText(SystemName, def, fn);
			NewStarSystem.SystemName = FString(SystemName);
		}
		else if (Key == "sky")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: sky struct missing in '%s'"), *LocalPath);
			}
			else
			{
				TermStruct* val = def->term()->isStruct();
				for (int i = 0; i < val->elements()->size(); i++)
				{
					TermDef* pdef = val->elements()->at(i)->isDef();
					if (!pdef) continue;

					const Text& SkyKey = pdef->name()->value();

					if (SkyKey == "poly_stars")
					{
						GetDefText(SkyPolyStars, pdef, fn);
						NewStarSystem.StarSky.SkyPolyStars = FString(SkyPolyStars);
					}
					else if (SkyKey == "nebula")
					{
						GetDefText(SkyNebula, pdef, fn);
						NewStarSystem.StarSky.SkyNebula = FString(SkyNebula);
					}
					else if (SkyKey == "haze")
					{
						GetDefText(SkyHaze, pdef, fn);
						NewStarSystem.StarSky.SkyHaze = FString(SkyHaze);
					}
				}
			}
		}
		else if (Key == "stars")
		{
			GetDefNumber(SkyStars, def, fn);
			NewStarSystem.SkyStars = SkyStars;
		}
		else if (Key == "ambient")
		{
			Vec3 a;
			GetDefVec(a, def, fn);
			AmbientColor = FColor((uint8)a.X, (uint8)a.Y, (uint8)a.Z, 255);
			NewStarSystem.AmbientColor = AmbientColor;
		}
		else if (Key == "dust")
		{
			GetDefNumber(SkyDust, def, fn);
			NewStarSystem.SkyDust = SkyDust;
		}
		else if (Key == "star")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: star struct missing in '%s'"), *LocalPath);
			}
			else
			{
				ParseStar(def->term()->isStruct(), fn);
				NewStarSystem.Star = StarDataArray; // <-- this must match FS_StarSystem::Star type
			}
		}
		else if (Key == "region")
		{
			if (!def->term() || !def->term()->isStruct())
			{
				UE_LOG(LogTemp, Warning, TEXT("WARNING: region struct missing in '%s'"), *LocalPath);
			}
			else
			{
				ParseRegion(def->term()->isStruct(), fn);

				// IMPORTANT:
				// This only compiles if FS_StarSystem::Region is TArray<FS_RegionMap>.
				// If it's not, you must change FS_StarSystem OR change ParseRegion output.
				// NewStarSystem.Region = RegionMapArray;
			}
		}
		else if (Key == "terrain")
		{
			// TODO: ParseTerrain(def->term()->isStruct(), fn);
		}

	} while (term);

	// free last term if loop ended without deleting
	if (term)
	{
		delete term;
		term = nullptr;
	}
}
void UStarshatterEnvironmentSubsystem::HydrateAllFromTables()
{
	ClearRuntimeCaches();

	ReadGalaxyDataTable();
	BuildStarSystemArrayFromGalaxy();
	ReadRegionsTable();

	BuildEnvironmentCaches();
}

void UStarshatterEnvironmentSubsystem::ClearRuntimeCaches()
{
	// -----------------------------------------------------------------
	// DT / hydrated row arrays
	// -----------------------------------------------------------------
	GalaxyDataArray.Reset();
	StarSystemDataArray.Reset();
	StarDataArray.Reset();
	PlanetDataArray.Reset();
	MoonDataArray.Reset();
	RegionDataArray.Reset();
	TerrainRegionsArray.Reset();

	// -----------------------------------------------------------------
	// Lookup caches
	// -----------------------------------------------------------------
	GalaxyByName.Reset();
	StarSystemByName.Reset();
	StarByName.Reset();
	PlanetByName.Reset();
	MoonByName.Reset();
	RegionByName.Reset();
	TerrainRegionByName.Reset();

	RegionParentByName.Reset();
	RegionChildrenByParent.Reset();

	// -----------------------------------------------------------------
	// Runtime object caches
	// -----------------------------------------------------------------
	RuntimeStarSystems.Reset();
	RuntimeStars.Reset();
	RuntimePlanets.Reset();
	RuntimeMoons.Reset();
	RuntimeRegions.Reset();

	UE_LOG(LogStarshatterEnvironment, Verbose,
		TEXT("[Environment] ClearRuntimeCaches complete"));
}

void UStarshatterEnvironmentSubsystem::ReadGalaxyDataTable()
{
	ReadTableToArray<FS_Galaxy>(
		GalaxyDataTable,
		GalaxyDataArray,
		TEXT("GalaxyDataTable (FS_Galaxy)"));
}

void UStarshatterEnvironmentSubsystem::BuildStarSystemArrayFromGalaxy()
{
	StarSystemDataArray.Reset();

	for (const FS_Galaxy& GalaxyRow : GalaxyDataArray)
	{
		if (GalaxyRow.Name.IsEmpty())
			continue;

		FS_StarSystem SystemRow;
		SystemRow.SystemName = GalaxyRow.Name;

		// Optional mapping
		// SystemRow.Location = GalaxyRow.Location;
		// SystemRow.Iff      = GalaxyRow.Iff;
		// SystemRow.Empire   = GalaxyRow.Empire;

		StarSystemDataArray.Add(SystemRow);
	}

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Built %d FS_StarSystem rows from GalaxyDataArray."),
		StarSystemDataArray.Num());
}

void UStarshatterEnvironmentSubsystem::ReadStarsTable()
{
	ReadTableToArray<FS_Star>(
		StarsDataTable,
		StarDataArray,
		TEXT("StarsDataTable (FS_Star)"));
}

void UStarshatterEnvironmentSubsystem::ReadPlanetsTable()
{
	ReadTableToArray<FS_Planet>(
		PlanetsDataTable,
		PlanetDataArray,
		TEXT("PlanetsDataTable (FS_Planet)"));
}

void UStarshatterEnvironmentSubsystem::ReadMoonsTable()
{
	ReadTableToArray<FS_Moon>(
		MoonsDataTable,
		MoonDataArray,
		TEXT("MoonsDataTable (FS_Moon)"));
}

void UStarshatterEnvironmentSubsystem::ReadRegionsTable()
{
	ReadTableToArray<FS_Region>(
		RegionsDataTable,
		RegionDataArray,
		TEXT("RegionsDataTable (FS_Region)"));
}

void UStarshatterEnvironmentSubsystem::ReadTerrainRegionsTable()
{
	ReadTableToArray<FS_TerrainRegion>(
		TerrainRegionsDataTable,
		TerrainRegionsArray,
		TEXT("TerrainRegionsDataTable (FS_TerrainRegion)"));
}

void UStarshatterEnvironmentSubsystem::BuildEnvironmentCaches()
{
	for (const FS_Galaxy& G : GalaxyDataArray)
		if (!G.Name.IsEmpty())
			GalaxyByName.Add(G.Name, G);

	for (const FS_StarSystem& S : StarSystemDataArray)
		if (!S.SystemName.IsEmpty())
			StarSystemByName.Add(S.SystemName, S);

	for (const FS_Star& S : StarDataArray)
		if (!S.Name.IsEmpty())
			StarByName.Add(S.Name, S);

	for (const FS_Planet& P : PlanetDataArray)
		if (!P.Name.IsEmpty())
			PlanetByName.Add(P.Name, P);

	for (const FS_Moon& M : MoonDataArray)
		if (!M.Name.IsEmpty())
			MoonByName.Add(M.Name, M);

	for (const FS_Region& R : RegionDataArray)
	{
		if (R.Name.IsEmpty())
			continue;

		RegionByName.Add(R.Name, R);
		RegionParentByName.Add(R.Name, R.Parent);

		if (!R.Parent.IsEmpty())
		{
			TArray<FString>& Children =
				RegionChildrenByParent.FindOrAdd(R.Parent);

			Children.Add(R.Name);
		}
	}

	for (const FS_TerrainRegion& T : TerrainRegionsArray)
		if (!T.Name.IsEmpty())
			TerrainRegionByName.Add(T.Name, T);

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Caches built."));
}

// -----------------------------------------------------------------------------
// Runtime Simulation Clock
// -----------------------------------------------------------------------------

void UStarshatterEnvironmentSubsystem::ResetSimulationClock()
{
	SimulationClockMs = 0;

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Simulation clock reset to 0 ms"));
}

void UStarshatterEnvironmentSubsystem::SetSimulationClockMs(int64 InMs)
{
	SimulationClockMs = InMs;

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Simulation clock set to %lld ms"),
		SimulationClockMs);
}

void UStarshatterEnvironmentSubsystem::AdvanceSimulationClock(double DeltaSeconds)
{
	const int64 DeltaMs = (int64)FMath::RoundToInt64(DeltaSeconds * 1000.0);
	SimulationClockMs += DeltaMs;

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Verbose,
		TEXT("[Environment] Simulation clock advanced by %lld ms -> %lld ms"),
		DeltaMs, SimulationClockMs);
}

void UStarshatterEnvironmentSubsystem::TickEnvironmentTime(double DeltaSeconds)
{
	AdvanceSimulationClock(DeltaSeconds);

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();
}

void UStarshatterEnvironmentSubsystem::Tick(float DeltaTime)
{
	if (!bLoaded)
	{
		return;
	}

	if (!bBaseTimeInitialized)
	{
		return;
	}

	AdvanceSimulationClock(DeltaTime);

	// Push authoritative runtime time into legacy StarSystem static clock:
	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();
}

bool UStarshatterEnvironmentSubsystem::IsTickable() const
{
	// Keep it simple and safe:
	return !IsTemplate() && bLoaded && bBaseTimeInitialized;
	//return true;
}

TStatId UStarshatterEnvironmentSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStarshatterEnvironmentSubsystem, STATGROUP_Tickables);
}

void UStarshatterEnvironmentSubsystem::BuildRuntimeStarSystems()
{
	RuntimeStarSystems.Empty();

	for (const FS_Galaxy& GalaxyRow : GalaxyDataArray)
	{
		if (GalaxyRow.Name.IsEmpty())
		{
			continue;
		}

		const float GalaxyRuntimeScale = 10.0f;
		const FVector RuntimeLoc = GalaxyRow.Location * GalaxyRuntimeScale;

		UE_LOG(LogTemp, Warning,
			TEXT("[Env] %s raw=(%.2f %.2f %.2f) scaled=(%.2f %.2f %.2f)"),
			*GalaxyRow.Name,
			GalaxyRow.Location.X, GalaxyRow.Location.Y, GalaxyRow.Location.Z,
			RuntimeLoc.X, RuntimeLoc.Y, RuntimeLoc.Z);

		StarSystem* StarSys = new StarSystem(
			TCHAR_TO_ANSI(*GalaxyRow.Name),
			RuntimeLoc,
			GalaxyRow.Iff,
			Star::G);

		if (!StarSys)
		{
			continue;
		}

		StarSys->HydrateFromEnvironment(GalaxyRow, nullptr);

		RuntimeStarSystems.Add(StarSys);

		UE_LOG(LogTemp, Warning,
			TEXT("[Env] Built StarSystem: %s"),
			*GalaxyRow.Name);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Env] Total Runtime Systems: %d"),
		RuntimeStarSystems.Num());
}

void UStarshatterEnvironmentSubsystem::RegisterStar(OrbitalBody* Body)
{
	if (!Body)
	{
		return;
	}

	RuntimeStars.AddUnique(Body);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered Star. Count=%d"),
		RuntimeStars.Num());
}

void UStarshatterEnvironmentSubsystem::RegisterPlanet(OrbitalBody* Body)
{
	if (!Body)
	{
		return;
	}

	RuntimePlanets.AddUnique(Body);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered Planet. Count=%d"),
		RuntimePlanets.Num());
}

void UStarshatterEnvironmentSubsystem::RegisterMoon(OrbitalBody* Body)
{
	if (!Body)
	{
		return;
	}

	RuntimeMoons.AddUnique(Body);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered Moon. Count=%d"),
		RuntimeMoons.Num());
}

void UStarshatterEnvironmentSubsystem::RegisterRegion(OrbitalRegion* Region)
{
	if (!Region)
	{
		return;
	}

	RuntimeRegions.AddUnique(Region);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered Region. Count=%d"),
		RuntimeRegions.Num());
}

