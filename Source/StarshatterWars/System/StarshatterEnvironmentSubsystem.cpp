/*=============================================================================
	Project:        Starshatter Wars
	Studio:         Fractal Dev Games
	Copyright:      (C) 2024-2026. All Rights Reserved.

	SUBSYSTEM:      StarshatterWars (Unreal Engine)
	FILE:           StarshatterEnvironmentSubsystem.cpp
	AUTHOR:         Carlos Bott

	OVERVIEW
	========
	JSON catalog ingestion with retained DataTable runtime hydration.

=============================================================================*/

#include "StarshatterEnvironmentSubsystem.h"
#include "GalaxyJsonCatalog.h"
#include "Engine/DataTable.h"
#include "StarshatterAssetRegistrySubsystem.h"
#include "FractalVerseSubsystem.h"
#include "SSWSessionLobbyGameMode.h"
#include "SSWRuntimeSubsystem.h"

// Core
#include "Logging/LogMacros.h"

// Engine / file helpers
#include "Misc/Paths.h"
#include "Engine/Engine.h"

// Legacy parsing / registry headers
#include "Galaxy.h"
#include "StarSystem.h"

#include "Sim.h"
#include "SimRegion.h"
#include "Orbital.h"

#include "StarSystemRegistry.h"

#include "SSWGameInstance.h"
#include "TimerSubsystem.h"

// Project

#include "GameStructs.h"
#include "GameStructs_System.h"


DEFINE_LOG_CATEGORY(LogStarshatterEnvironment);

TWeakObjectPtr<UStarshatterEnvironmentSubsystem>
UStarshatterEnvironmentSubsystem::ActiveInstance = nullptr;

// -----------------------------------------------------------------------------
// UStarshatterEnvironmentSubsystem
// -----------------------------------------------------------------------------

template<typename TRowStruct>
static bool ReadTableToArray(
	const UDataTable* Table,
	TArray<TRowStruct>& OutArray,
	const TCHAR* Label)
{
	OutArray.Reset();

	if (!Table)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] %s is null."), Label);
		return false;
	}

	if (Table->GetRowStruct() != TRowStruct::StaticStruct())
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] %s row struct mismatch. Expected=%s Actual=%s"),
			Label,
			*GetNameSafe(TRowStruct::StaticStruct()),
			*GetNameSafe(Table->GetRowStruct()));
		return false;
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

	return true;
}


void UStarshatterEnvironmentSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogStarshatterEnvironment, Log, TEXT("[Environment] Initialize"));

	ActiveInstance = this;

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Initialize (ActiveInstance set)"));

	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[Environment] Initialize: GameInstance is null"));
		return;
	}

	SSWInstance = Cast<USSWGameInstance>(GI);

	SetProjectPath();
	ResolveDataTables();

	// Runtime time state
	bBaseTimeInitialized = false;
	EnvironmentBaseTime = 0.0;
	SimulationClockMs = 0;
	SetSimTimeScale(1.0);

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

UStarshatterEnvironmentSubsystem* UStarshatterEnvironmentSubsystem::Get()
{
	return ActiveInstance.Get();
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

    if (!LoadGalaxyMap())
    {
        bLoaded = false;
        return;
    }
    CreateEnvironmentTables();

	if (RuntimeStarSystems.Num() == 0)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] LoadAll failed: no runtime star systems built"));
		bLoaded = false;
		return;
	}

	Galaxy::InitializeFromEnvironment(this);

	// Initialize the Babylon 5 universe epoch and propagate it to live systems.
	InitSimulationBaseTime();

	// Synchronize the environment simulation clock with the authoritative
	// universe elapsed time when available.  New universes begin at T+0.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>())
		{
			SetSimulationClockMs(
				static_cast<int64>(Timer->GetUniverseTimeSeconds()) * 1000LL);
		}
		else
		{
			ResetSimulationClock();
		}
	}
	else
	{
		ResetSimulationClock();
	}

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();

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
			TEXT("[Environment] BaseTime already initialized: %.0f"),
			EnvironmentBaseTime);
		return;
	}

	// ---------------------------------------------------------------------
	// Babylon 5 canonical universe epoch
	// ---------------------------------------------------------------------
	// Starshatter Wars is not based on the host computer's real UTC clock.
	// The game universe begins at 2228-01-01 00:00:00.
	//
	// EnvironmentBaseTime is stored in Unix-seconds form only because the
	// legacy StarSystem stardate/orbital code expects an absolute base value.
	// The date itself is the fictional game epoch, not "now".
	// ---------------------------------------------------------------------
	const FDateTime Babylon5Epoch =
		GetUniverseEpoch();

	const int64 Babylon5BaseUnixSeconds =
		Babylon5Epoch.ToUnixTimestamp();

	EnvironmentBaseTime =
		static_cast<double>(Babylon5BaseUnixSeconds);

	bBaseTimeInitialized = true;

	// If the authoritative universe clock has already been loaded, use its
	// elapsed simulation time.  The calendar base remains fixed at 2228.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTimerSubsystem* Timer =
			GI->GetSubsystem<UTimerSubsystem>())
		{
			SimulationClockMs =
				static_cast<int64>(
					Timer->GetUniverseTimeSeconds()) *
				1000LL;
		}
	}

	StarSystem::SetSimulationTime(
		GetSimulationClockSeconds());

	int32 Count = 0;

	for (StarSystem* System : RuntimeStarSystems)
	{
		if (!System)
		{
			continue;
		}

		System->SetBaseTime(
			EnvironmentBaseTime,
			true);

		++Count;
	}

	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Warning,
		TEXT(
			"[Environment] Babylon 5 epoch initialized: "
			"2228-01-01 00:00:00  UnixBase=%lld  "
			"Elapsed=%.3f sec  Systems=%d"),
		static_cast<long long>(
			Babylon5BaseUnixSeconds),
		GetSimulationClockSeconds(),
		Count);
}

uint32 UStarshatterEnvironmentSubsystem::GetGameTime() const
{
	if (SimulationClockMs < 0)
	{
		return 0;
	}

	return (uint32)SimulationClockMs;
}

void UStarshatterEnvironmentSubsystem::ResetGameTime()
{
	SimulationClockMs = 0;

	StarSystem::SetSimulationTime(
		GetSimulationClockSeconds());

	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] GameTime reset to 0"));
}

void UStarshatterEnvironmentSubsystem::SetGameTime(uint32 NewTimeMs)
{
	SimulationClockMs =
		(int64)NewTimeMs;

	StarSystem::SetSimulationTime(
		GetSimulationClockSeconds());

	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Log,
		TEXT("[Environment] GameTime set to %u ms"),
		NewTimeMs);
}

void UStarshatterEnvironmentSubsystem::SkipGameTime(double Seconds)
{
	if (Seconds <= 0.0)
	{
		return;
	}

	const uint32 DeltaMs =
		(uint32)(Seconds * 1000.0);

	SimulationClockMs += DeltaMs;

	StarSystem::SetSimulationTime(
		GetSimulationClockSeconds());

	StarSystem::CalcStardate();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] SkipGameTime Seconds=%.3f DeltaMs=%u NewTime=%lld"),
		Seconds,
		DeltaMs,
		SimulationClockMs);
}
void UStarshatterEnvironmentSubsystem::RegisterStarSystem(StarSystem* System)
{
	if (!System)
		return;

	RuntimeStarSystems.AddUnique(System);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] Registered StarSystem. Count=%d"),
		RuntimeStarSystems.Num());

	// If the Babylon 5 universe base is already initialized,
	// apply it immediately to systems registered after initial load.
	if (bBaseTimeInitialized)
	{
		System->SetBaseTime(
			EnvironmentBaseTime,
			true);

		StarSystem::SetSimulationTime(
			GetSimulationClockSeconds());

		StarSystem::CalcStardate();
	}
}


void UStarshatterEnvironmentSubsystem::CreateEnvironmentTables()
{
	UE_LOG(LogStarshatterEnvironment, Log, TEXT("[Environment] CreateEnvironmentTables"));

	const bool bHydrateOk = HydrateAllFromTables();
	if (!bHydrateOk)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] CreateEnvironmentTables aborted: hydration failed. Registry left untouched."));
		return;
	}

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


bool UStarshatterEnvironmentSubsystem::LoadGalaxyMap()
{
    TArray<FS_Galaxy> Systems;
    TArray<FRegion> Regions;
    FString Error;
    const FString Path = FPaths::ProjectContentDir() / TEXT("GameData/Galaxy/galaxy.json");
    if (!GalaxyJsonCatalog::Load(Path, Systems, Regions, Error))
    {
        UE_LOG(LogStarshatterEnvironment, Error, TEXT("[Galaxy] %s"), *Error);
        return false;
    }
    // Preserve the table bridge for existing runtime and editor consumers.
    if (!GalaxyDataTable) GalaxyDataTable = NewObject<UDataTable>(this);
    if (!RegionsDataTable) RegionsDataTable = NewObject<UDataTable>(this);
    if ((GalaxyDataTable->GetRowStruct() && GalaxyDataTable->GetRowStruct() != FS_Galaxy::StaticStruct()) ||
        (RegionsDataTable->GetRowStruct() && RegionsDataTable->GetRowStruct() != FRegion::StaticStruct()))
    {
        UE_LOG(LogStarshatterEnvironment, Error, TEXT("[Galaxy] Compatibility table row type mismatch"));
        return false;
    }
    GalaxyDataTable->RowStruct = FS_Galaxy::StaticStruct();
    RegionsDataTable->RowStruct = FRegion::StaticStruct();
    GalaxyDataTable->EmptyTable();
    RegionsDataTable->EmptyTable();
    for (const auto& Row : Systems) GalaxyDataTable->AddRow(FName(*Row.Name), Row);
    for (const auto& Row : Regions) RegionsDataTable->AddRow(FName(*Row.Name), Row);
    GalaxyDataArray = MoveTemp(Systems);
    RegionDataArray = MoveTemp(Regions);
    BuildStarSystemArrayFromGalaxy();
    BuildEnvironmentCaches();
    return true;
}


void UStarshatterEnvironmentSubsystem::ClearRuntimeCaches()
{
	GalaxyDataArray.Reset();
	StarSystemDataArray.Reset();
	StarDataArray.Reset();
	PlanetDataArray.Reset();
	MoonDataArray.Reset();
	RegionDataArray.Reset();
	TerrainRegionsArray.Reset();

	PlanetMapByName.Reset();
	MoonMapByName.Reset();

	GalaxyByName.Reset();
	StarSystemByName.Reset();
	StarByName.Reset();
	PlanetByName.Reset();
	MoonByName.Reset();
	RegionByName.Reset();
	TerrainRegionByName.Reset();

	RegionParentByName.Reset();
	RegionChildrenByParent.Reset();

	RuntimeStarSystems.Reset();
	RuntimeStars.Reset();
	RuntimePlanets.Reset();
	RuntimeMoons.Reset();
	RuntimeRegions.Reset();

	UE_LOG(LogStarshatterEnvironment, Verbose,
		TEXT("[Environment] ClearRuntimeCaches complete"));
}

bool UStarshatterEnvironmentSubsystem::BuildStarSystemArrayFromGalaxy()
{
	StarSystemDataArray.Reset();
	StarDataArray.Reset();
	PlanetDataArray.Reset();
	MoonDataArray.Reset();

	for (const FS_Galaxy& GalaxyRow : GalaxyDataArray)
	{
		if (GalaxyRow.Name.IsEmpty())
		{
			continue;
		}

		// Build one system row per galaxy/system entry.
		// FStarSystem does not contain Location/Iff/Star/Empire/Link, so only fill valid fields.
		FStarSystem SystemRow;
		SystemRow.Name = GalaxyRow.Name;
		SystemRow.SystemName = GalaxyRow.Name;
		SystemRow.Class = GalaxyRow.Class;

		StarSystemDataArray.Add(SystemRow);

		// Flatten nested stellar data into StarDataArray
		for (const FStarSystem& StarRowFromGalaxy : GalaxyRow.Stellar)
		{
			if (StarRowFromGalaxy.Name.IsEmpty())
			{
				continue;
			}

			FStarSystem StarRow = StarRowFromGalaxy;

			if (StarRow.SystemName.IsEmpty())
			{
				StarRow.SystemName = GalaxyRow.Name;
			}

			StarDataArray.Add(StarRow);

			// Flatten planets from each star
			for (const FPlanet& PlanetRowFromStar : StarRowFromGalaxy.Planet)
			{
				if (PlanetRowFromStar.Name.IsEmpty())
				{
					continue;
				}

				FPlanet PlanetRow = PlanetRowFromStar;
				PlanetDataArray.Add(PlanetRow);

				// Flatten moons from each planet
				for (const FMoon& MoonRowFromPlanet : PlanetRowFromStar.Moon)
				{
					if (MoonRowFromPlanet.Name.IsEmpty())
					{
						continue;
					}

					FMoon MoonRow = MoonRowFromPlanet;

					if (MoonRow.Parent.IsEmpty())
					{
						MoonRow.Parent = PlanetRowFromStar.Name;
					}

					MoonDataArray.Add(MoonRow);
				}
			}
		}
	}

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Built from GalaxyDataArray: Systems=%d Stars=%d Planets=%d Moons=%d"),
		StarSystemDataArray.Num(),
		StarDataArray.Num(),
		PlanetDataArray.Num(),
		MoonDataArray.Num());

	return StarSystemDataArray.Num() > 0;
}

void UStarshatterEnvironmentSubsystem::BuildEnvironmentCaches()
{
	GalaxyByName.Reset();
	StarSystemByName.Reset();
	StarByName.Reset();
	PlanetByName.Reset();
	MoonByName.Reset();
	RegionByName.Reset();
	TerrainRegionByName.Reset();
	RegionParentByName.Reset();
	RegionChildrenByParent.Reset();

	PlanetMapByName.Reset();
	MoonMapByName.Reset();

	for (const FS_Galaxy& G : GalaxyDataArray)
	{
		if (!G.Name.IsEmpty())
		{
			GalaxyByName.Add(G.Name, G);
		}
	}

	for (const FStarSystem& S : StarSystemDataArray)
	{
		if (!S.SystemName.IsEmpty())
		{
			StarSystemByName.Add(S.SystemName, S);
		}
	}

	for (const FStarSystem& S : StarDataArray)
	{
		if (!S.Name.IsEmpty())
		{
			StarByName.Add(S.Name, S);
		}
	}

	for (const FPlanet& P : PlanetDataArray)
	{
		if (P.Name.IsEmpty())
		{
			continue;
		}

		const FString Key = P.Name.TrimStartAndEnd();

		PlanetByName.Add(Key, P);
		PlanetMapByName.Add(Key, P);
	}

	for (const FMoon& M : MoonDataArray)
	{
		if (M.Name.IsEmpty())
		{
			continue;
		}

		const FString Key = M.Name.TrimStartAndEnd();

		MoonByName.Add(Key, M);
		MoonMapByName.Add(Key, M);
	}

	for (const FRegion& R : RegionDataArray)
	{
		if (R.Name.IsEmpty())
		{
			continue;
		}

		RegionByName.Add(R.Name, R);
		RegionParentByName.Add(R.Name, R.Parent);

		if (!R.Parent.IsEmpty())
		{
			TArray<FString>& Children = RegionChildrenByParent.FindOrAdd(R.Parent);
			Children.Add(R.Name);
		}
	}

	for (const FS_TerrainRegion& T : TerrainRegionsArray)
	{
		if (!T.Name.IsEmpty())
		{
			TerrainRegionByName.Add(T.Name, T);
		}
	}

	UE_LOG(LogStarshatterEnvironment, Log,
		TEXT("[Environment] Caches built. PlanetByName=%d PlanetMapByName=%d MoonByName=%d MoonMapByName=%d"),
		PlanetByName.Num(),
		PlanetMapByName.Num(),
		MoonByName.Num(),
		MoonMapByName.Num());
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

	UE_LOG(LogTemp, VeryVerbose,
		TEXT("[Environment] Simulation clock set to %lld ms"),
		SimulationClockMs);
}

void UStarshatterEnvironmentSubsystem::AdvanceSimulationClock(double DeltaSeconds)
{
    if (const UWorld* World = GetWorld(); World && World->GetNetMode() == NM_Client)
    {
        if (const auto* State = World->GetGameState<ASSWSessionGameState>())
        {
            SimulationClockMs = State->GalaxyClockMs;
            StarSystem::SetSimulationTime(GetSimulationClockSeconds());
            StarSystem::CalcStardate();
        }
        return;
    }

	if (DeltaSeconds <= 0.0)
	{
		return;
	}

	// -------------------------------------------------------------------------
	// Authoritative simulation timestep
	// -------------------------------------------------------------------------

	const double SimSeconds =
		DeltaSeconds * SimTimeScale;

	const int64 DeltaMs =
		(int64)FMath::RoundToInt64(SimSeconds * 1000.0);

	SimulationClockMs += DeltaMs;

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();

	UE_LOG(LogTemp, VeryVerbose,
		TEXT("[Environment] AdvanceSimulationClock UE=%.6f Sim=%.6f Scale=%.3f ClockMs=%lld"),
		DeltaSeconds,
		SimSeconds,
		SimTimeScale,
		SimulationClockMs);
}

void UStarshatterEnvironmentSubsystem::SetSimTimeScale(double NewScale)
{
	// Prevent invalid or negative scales:
	SimTimeScale = FMath::Max(0.0001, NewScale);

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] SimTimeScale set to %.3f"),
		SimTimeScale);
}

void UStarshatterEnvironmentSubsystem::TickEnvironmentTime(double DeltaSeconds)
{
    if (!bLoaded || !bBaseTimeInitialized) return;
    if (UFractalVerseSubsystem::ProcessRole()!=EFractalVerseRole::Disabled)
    {
        const auto* Service=GetGameInstance()?GetGameInstance()->GetSubsystem<UFractalVerseSubsystem>():nullptr;
        if (!Service) return;
        const auto Snapshot=Service->Status();
        if(Snapshot.UniverseBaseUtc<=0) return;
        const int64 ConfirmedMs=Snapshot.UniverseSeconds*1000LL;
        if(ConfirmedMs!=SimulationClockMs) SetSimulationClockMs(ConfirmedMs);
    }
    else
	AdvanceSimulationClock(DeltaSeconds);

	StarSystem::SetSimulationTime(GetSimulationClockSeconds());
	StarSystem::CalcStardate();
}

void UStarshatterEnvironmentSubsystem::Tick(float DeltaTime)
{
    if (UFractalVerseSubsystem::ProcessRole()!=EFractalVerseRole::Disabled) { TickEnvironmentTime(0); return; }
	if (!bLoaded)
	{
		return;
	}

	if (!bBaseTimeInitialized)
	{
		return;
	}

    if (auto* GI=GetGameInstance())
        if (auto* Runtime=GI->GetSubsystem<USSWRuntimeSubsystem>())
            if (Runtime->IsRuntimeRunning()) return;
	AdvanceSimulationClock(DeltaTime);
}

bool UStarshatterEnvironmentSubsystem::IsTickable() const
{
	return !IsTemplate();
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

const FS_Galaxy* UStarshatterEnvironmentSubsystem::FindGalaxyByName(const FString& InName) const
{
	if (InName.IsEmpty())
	{
		return nullptr;
	}

	for (const FS_Galaxy& Row : GalaxyDataArray)
	{
		if (Row.Name.Equals(InName, ESearchCase::IgnoreCase))
		{
			return &Row;
		}
	}

	return nullptr;
}

const FStarSystem* UStarshatterEnvironmentSubsystem::FindStarSystemByName(const FString& InName) const
{
	if (InName.IsEmpty())
	{
		return nullptr;
	}

	for (const FStarSystem& Row : StarSystemDataArray)
	{
		if (Row.SystemName.Equals(InName, ESearchCase::IgnoreCase))
		{
			return &Row;
		}
	}

	return nullptr;
}

const FPlanet* UStarshatterEnvironmentSubsystem::FindPlanetMapByName(const FString& Name) const
{
	const FString SearchName = Name.TrimStartAndEnd();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] FindPlanetMapByName: searching for '%s' PlanetMapByName.Num=%d"),
		*SearchName,
		PlanetMapByName.Num());

	if (const FPlanet* Found = PlanetMapByName.Find(SearchName))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Environment] FindPlanetMapByName: FOUND '%s' -> Type=%d"),
			*SearchName,
			(int32)Found->PlanetType);

		return Found;
	}

	for (const TPair<FString, FPlanet>& Pair : PlanetMapByName)
	{
		if (Pair.Key.TrimStartAndEnd().Equals(SearchName, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Environment] FindPlanetMapByName: FOUND (IgnoreCase) '%s' -> Key='%s' Type=%d"),
				*SearchName,
				*Pair.Key,
				(int32)Pair.Value.PlanetType);

			return &Pair.Value;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] FindPlanetMapByName: NOT FOUND '%s'"),
		*SearchName);

	return nullptr;
}

const FMoon* UStarshatterEnvironmentSubsystem::FindMoonMapByName(const FString& Name) const
{
	const FString SearchName = Name.TrimStartAndEnd();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] FindMoonMapByName: searching for '%s' MoonMapByName.Num=%d"),
		*SearchName,
		MoonMapByName.Num());

	if (const FMoon* Found = MoonMapByName.Find(SearchName))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Environment] FindMoonMapByName: FOUND '%s' -> Type=%d"),
			*SearchName,
			(int32)Found->PlanetType);

		return Found;
	}

	for (const TPair<FString, FMoon>& Pair : MoonMapByName)
	{
		if (Pair.Key.TrimStartAndEnd().Equals(SearchName, ESearchCase::IgnoreCase))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[Environment] FindMoonMapByName: FOUND (IgnoreCase) '%s' -> Key='%s' Type=%d"),
				*SearchName,
				*Pair.Key,
				(int32)Pair.Value.PlanetType);

			return &Pair.Value;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] FindMoonMapByName: NOT FOUND '%s'"),
		*SearchName);

	return nullptr;
}

void UStarshatterEnvironmentSubsystem::BuildSimRegionsForSim(Sim* SimInst)
{
	if (!SimInst)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[Environment] BuildSimRegionsForSim failed: SimInst is null"));
		return;
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] BuildSimRegionsForSim RuntimeRegions=%d"),
		RuntimeRegions.Num());

	for (OrbitalRegion* Orbital : RuntimeRegions)
	{
		if (!Orbital)
		{
			continue;
		}

		if (SimInst->FindRegion(Orbital->GetName()))
		{
			continue;
		}

		SimRegion* Region =
			new SimRegion(SimInst, Orbital);

		SimInst->GetRegions().append(Region);

		UE_LOG(LogTemp, Warning,
			TEXT("[Environment] SIM REGION CREATE Name='%hs' SimRegion=%p Orbital=%p"),
			Orbital->GetName(),
			Region,
			Orbital);
	}
}


const FRegion* UStarshatterEnvironmentSubsystem::FindRegionByName(const FString& Name) const
{
    if (const auto* Found = RegionByName.Find(Name)) return Found;
    for (const auto& Pair : RegionByName)
        if (Pair.Key.Equals(Name, ESearchCase::IgnoreCase)) return &Pair.Value;
    return nullptr;
}

void UStarshatterEnvironmentSubsystem::ResolveDataTables()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UStarshatterAssetRegistrySubsystem* Assets =
		GI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
	if (!Assets)
	{
		return;
	}

	GalaxyDataTable = Assets->GetDataTable(TEXT("Data.GalaxyMapTable"), true);
	RegionsDataTable = Assets->GetDataTable(TEXT("Data.RegionsTable"), true);
}

bool UStarshatterEnvironmentSubsystem::HydrateAllFromTables()
{
	ClearRuntimeCaches();

	if (!ReadGalaxyDataTable())
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] HydrateAllFromTables failed: GalaxyDataTable invalid"));
		return false;
	}

	if (!BuildStarSystemArrayFromGalaxy())
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] HydrateAllFromTables failed: StarSystemDataArray build failed"));
		return false;
	}

	if (!ReadRegionsTable())
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] HydrateAllFromTables failed: RegionsDataTable invalid"));
		return false;
	}

	// Terrain/zones are still optional if you really have them elsewhere.
	if (TerrainRegionsDataTable)
	{
		ReadTerrainRegionsTable();
	}

	BuildEnvironmentCaches();

	UE_LOG(LogTemp, Warning,
		TEXT("[Environment] HydrateAllFromTables: Galaxy=%d Systems=%d Stars=%d Planets=%d Moons=%d Regions=%d Terrain=%d Zones=%d"),
		GalaxyDataArray.Num(),
		StarSystemDataArray.Num(),
		StarDataArray.Num(),
		PlanetDataArray.Num(),
		MoonDataArray.Num(),
		RegionDataArray.Num(),
		TerrainRegionsArray.Num(),
		ZoneDataArray.Num());

	if (GalaxyDataArray.Num() == 0)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] HydrateAllFromTables failed: GalaxyDataArray is empty"));
		return false;
	}

	if (StarSystemDataArray.Num() == 0)
	{
		UE_LOG(LogStarshatterEnvironment, Error,
			TEXT("[Environment] HydrateAllFromTables failed: StarSystemDataArray is empty"));
		return false;
	}

	return true;
}

bool UStarshatterEnvironmentSubsystem::ReadGalaxyDataTable()
{
	return ReadTableToArray<FS_Galaxy>(
		GalaxyDataTable,
		GalaxyDataArray,
		TEXT("GalaxyDataTable (FS_Galaxy)"));
}

bool UStarshatterEnvironmentSubsystem::ReadStarsTable()
{
	return ReadTableToArray<FStarSystem>(
		StarsDataTable,
		StarDataArray,
		TEXT("StarsDataTable (FS_Star)"));
}

bool UStarshatterEnvironmentSubsystem::ReadPlanetsTable()
{
	return ReadTableToArray<FPlanet>(
		PlanetsDataTable,
		PlanetDataArray,
		TEXT("PlanetsDataTable (FS_Planet)"));
}

bool UStarshatterEnvironmentSubsystem::ReadMoonsTable()
{
	return ReadTableToArray<FMoon>(
		MoonsDataTable,
		MoonDataArray,
		TEXT("MoonsDataTable (FS_Moon)"));
}

bool UStarshatterEnvironmentSubsystem::ReadRegionsTable()
{
	return ReadTableToArray<FRegion>(
		RegionsDataTable,
		RegionDataArray,
		TEXT("RegionsDataTable (FRegion)"));
}

bool UStarshatterEnvironmentSubsystem::ReadTerrainRegionsTable()
{
	return ReadTableToArray<FS_TerrainRegion>(
		TerrainRegionsDataTable,
		TerrainRegionsArray,
		TEXT("TerrainRegionsDataTable (FS_TerrainRegion)"));
}
