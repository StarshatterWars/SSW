
#include "SSWRuntimeSubsystem.h"

#include "SimShot.h"
#include "Sensor.h"
#include "WeaponDesign.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "BeamWeaponActor.h"

#include "Math/RandomStream.h"
#include "SimElement.h"
#include "CombatGroup.h"
#include "CombatUnit.h"

#include "SystemSceneBuilder.h"
#include "PlanetActor.h"
#include "Components/StaticMeshComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#include "OrbitalRegion.h"

#include "Starshatter.h"

#include "Game.h"
#include "WaypointActor.h"
#include "Instruction.h"
#include "Engine/World.h"
#include "Sim.h"
#include "TimerSubsystem.h"
#include "Engine/GameInstance.h"
#include "SimRegion.h"
#include "SimUniverse.h"
#include "Galaxy.h"
#include "Campaign.h"
#include "CameraManager.h"
#include "EventDispatch.h"
#include "HUDView.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"

#include "AudioConfig.h"
#include "UIButton.h"
#include "PlayerCharacter.h"
#include "HUDSounds.h"

#include "MultiController.h"
#include "Keyboard.h"
#include "Joystick.h"

#include "MusicManager.h"

#include "Ship.h"
#include "CombatRoster.h"

#include "RadioTraffic.h"
#include "RadioView.h"
#include "RadioVox.h"
#include "QuantumView.h"
#include "TacticalView.h"
#include "Tickable.h"

#include "Kismet/KismetSystemLibrary.h"

#include "ShipActor.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "ShipDesignRegistry.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"


#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogSSWRuntime, Log, All);

void USSWRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] Initialize"));
}

void USSWRuntimeSubsystem::Deinitialize()
{
    EndMissionPresentation();
    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] Deinitialize"));

    StopRuntime();

    if (SimInstance)
    {
        delete SimInstance;
        SimInstance = nullptr;
    }

    if (RuntimeInput)
    {
        delete RuntimeInput;
        RuntimeInput = nullptr;
    }

    if (LegacyGame)
    {
        delete LegacyGame;
        LegacyGame = nullptr;
    }

    RadioVox::Close();
    Super::Deinitialize();
}

bool USSWRuntimeSubsystem::Init()
{
    if (!LegacyGame)
    {
        UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] Creating Starshatter runtime bridge."));
        LegacyGame = new Starshatter();
    }

    if (!LegacyGame)
    {
        UE_LOG(LogSSWRuntime, Error, TEXT("[RUNTIME] Failed to create Starshatter runtime bridge."));
        return false;
    }

    return LegacyGame->Init();
}

bool USSWRuntimeSubsystem::InitGame()
{
    if (bRuntimeInitialized)
        return true;

    SetPaused(false);
    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] InitGame"));

    FMath::RandInit(static_cast<int32>(FPlatformTime::Cycles()));

    AudioConfig::Initialize();

    InitMouse();

    UIButton::Initialize();
    EventDispatch::Create();
    PlayerCharacter::Initialize();
    HUDSounds::Initialize();

    RuntimeInput = new MultiController();

    Keyboard* KeyboardController = new Keyboard();
    RuntimeInput->AddController(KeyboardController);

#if PLATFORM_WINDOWS
    ActivateKeyboardLayout(GetKeyboardLayout(0), 0);
#endif

    Joystick* JoystickController = new Joystick();
    JoystickController->SetSensitivity(15, 5000);
    RuntimeInput->AddController(JoystickController);

    Joystick::EnumerateDevices();

    MapKeys();

    /*
        Runtime must not load files:
            - no key.cfg
            - no sys.def
            - no wep.def

        Boot/DataSubsystem must fill the tables/registries before this point.
    */

    MusicManager::Initialize();

    if (bNoSplash)
    {
        Ship::Initialize();
        CombatRoster::Initialize();
        Campaign::Initialize();
    }
    else
    {
        SetupSplash();
    }

    CreateWorld();

    TimeMark = Game::GetGameTime();
    Minutes = 0;

    bRuntimeInitialized = true;

    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] InitGame complete."));

    return true;
}

void USSWRuntimeSubsystem::InitMouse()
{
    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] InitMouse skipped - Unreal PlayerController owns mouse input."));
}

void USSWRuntimeSubsystem::CreateWorld()
{
    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] CreateWorld"));

    RadioTraffic::Initialize();
    RadioView::Initialize();
    RadioVox::Initialize(this);
    QuantumView::Initialize();
    TacticalView::Initialize();

    if (!SimInstance)
    {
        SimInstance = new Sim(RuntimeInput);
        SimInstance->OnMissionUnloading.AddUObject(this, &USSWRuntimeSubsystem::HandleMissionUnloading);

        UE_LOG(LogSSWRuntime, Log,
            TEXT("[RUNTIME] World Created. Sim=%p Input=%p"),
            SimInstance,
            RuntimeInput);
    }

    CamDir = CameraManager::GetInstance();

    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] CameraManager=%p"),
        CamDir);
}

void USSWRuntimeSubsystem::StartRuntime()
{
    if (bRuntimeRunning)
        return;

    if (!bRuntimeInitialized)
    {
        if (!InitGame())
            return;
    }

    RuntimeTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(
            this,
            &USSWRuntimeSubsystem::TickRuntime
        )
    );

    bRuntimeRunning = true;
	SetPaused(false);

    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] Game loop started."));
}

void USSWRuntimeSubsystem::StopRuntime()
{
    if (!bRuntimeRunning)
        return;

    FTSTicker::GetCoreTicker().RemoveTicker(RuntimeTickerHandle);
    RuntimeTickerHandle.Reset();

    bRuntimeRunning = false;

    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] Game loop stopped."));
}

bool USSWRuntimeSubsystem::TickRuntime(float DeltaSeconds)
{
    LastDeltaSeconds = DeltaSeconds;

    GameLoop();

    return true;
}

void USSWRuntimeSubsystem::EnsureMfdPanels()
{
}

void USSWRuntimeSubsystem::OnSensorRangeUp()
{
    if (!SimInstance) return;
    if (UWorld* World = GetWorld())
        if (APlayerController* PC = World->GetFirstPlayerController())
            if (!(PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
                return;
    if (Ship* Player = SimInstance->GetPlayerShip())
        if (Sensor* S = Player->GetSensor())
            S->IncreaseRange();
}

void USSWRuntimeSubsystem::OnSensorRangeDown()
{
    if (!SimInstance) return;
    if (UWorld* World = GetWorld())
        if (APlayerController* PC = World->GetFirstPlayerController())
            if (!(PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift)))
                return;
    if (Ship* Player = SimInstance->GetPlayerShip())
        if (Sensor* S = Player->GetSensor())
            S->DecreaseRange();
}

void USSWRuntimeSubsystem::GameLoop()
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            static bool bRangeBound = false;
            if (!bRangeBound)
                bRangeBound = true;
            const bool bMod = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift);
            const bool bUp = PC->IsInputKeyDown(EKeys::RightBracket);
            const bool bDown = PC->IsInputKeyDown(EKeys::LeftBracket);
            static bool bWasUp = false, bWasDown = false;
            if (bMod && SimInstance)
            {
                if (Ship* Player = SimInstance->GetPlayerShip())
                {
                    if (Sensor* S = Player->GetSensor())
                    {
                        if (bUp && !bWasUp) S->IncreaseRange();
                        if (bDown && !bWasDown) S->DecreaseRange();
                    }
                }
            }
            bWasUp = bUp;
            bWasDown = bDown;
            float MX = 0.f, MY = 0.f;
            if (!PC->GetMousePosition(MX, MY))
            {
                const FVector2D Cursor = FSlateApplication::Get().GetCursorPos();
                MX = Cursor.X;
                MY = Cursor.Y;
            }
            Mouse::SetState(
                (int)MX, (int)MY,
                PC->IsInputKeyDown(EKeys::LeftMouseButton) ? 1 : 0,
                PC->IsInputKeyDown(EKeys::RightMouseButton) ? 1 : 0);
        }
    }

    EventDispatch* ED = EventDispatch::GetInstance();
    if (ED)
    {
        ED->Dispatch();
    }

    UpdateWorld();
    GameState();
    UpdateScreen();
    CollectStats();
}

void USSWRuntimeSubsystem::UpdateWorld()
{
    TRACE_CPUPROFILER_EVENT_SCOPE(SSW_UpdateWorld);
    const double Seconds =
        static_cast<double>(LastDeltaSeconds) *
        static_cast<double>(TimeCompression);

    Galaxy* GalaxyInstance = Galaxy::GetInstance();
    if (GalaxyInstance)
    {
        GalaxyInstance->ExecFrame();
    }

    Campaign* CampaignInstance = Campaign::GetCampaign();
    if (CampaignInstance)
    {
        CampaignInstance->ExecFrame();
    }

    if (SimInstance && SimInstance->GetMission() && !bPaused)
    {
        const uint32 BeforeTime =
            Game::GetGameTime();

        SimInstance->ExecFrame(Seconds);
        if (ASystemSceneBuilder* Builder = MissionPresentationBuilder.Get())
            if (SimRegion* ActiveRegion = SimInstance->GetActiveRegion())
            {
                Ship* PlayerShip = SimInstance->GetPlayerShip();
                Builder->UpdateMissionSpace(ActiveRegion->GetLocation(),
                    PlayerShip ? ConvertLegacyShipLocationToUE(PlayerShip->GetLocation()) : FVector::ZeroVector);
            }
        UpdateProjectileVisuals();
        WaypointRefreshSeconds += Seconds;
        if (WaypointRefreshSeconds >= 0.25) { WaypointRefreshSeconds = 0; UpdateMissionWaypoints(); }

        const uint32 AfterTime =
            Game::GetGameTime();

        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeSubsystem::UpdateWorld] Sim::ExecFrame DeltaMS=%d GameTime=%u Regions=%d"),
            (int32)(AfterTime - BeforeTime),
            AfterTime,
            SimInstance->GetRegions().size());
    }

    if (CamDir)
    {
        CamDir->ExecFrame(Seconds);
    }
}

void USSWRuntimeSubsystem::GameState()
{
    switch (GameMode)
    {
    case EGameMode::BOOT:
    case EGameMode::INIT:
        break;

    case EGameMode::MENU:
        HandleMenuState();
        break;

    case EGameMode::CLOD:
    case EGameMode::PREP:
    case EGameMode::LOAD:
        HandleLoadState();
        break;

    case EGameMode::PLAN:
        HandlePlanState();
        break;

    case EGameMode::CMPN:
        HandleCampaignState();
        break;

    case EGameMode::PLAY:
        HandlePlayState();
        break;

    case EGameMode::EXIT:
        HandleExitState();
        break;

    default:
        break;
    }

    if (MusicManager::GetInstance())
    {
        MusicManager::GetInstance()->ExecFrame();
    }
}

void USSWRuntimeSubsystem::UpdateScreen()
{
}

void USSWRuntimeSubsystem::CollectStats()
{
}

void USSWRuntimeSubsystem::SetGameMode(EGameMode NewMode)
{
    if (GameMode == NewMode)
        return;

    const EGameMode OldMode = GameMode;

    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] GameMode: %d -> %d"),
        static_cast<int32>(OldMode),
        static_cast<int32>(NewMode));

    switch (NewMode)
    {
    case EGameMode::BOOT:
    case EGameMode::INIT:
        SetPaused(true);
        break;

    case EGameMode::LOAD:
    case EGameMode::CLOD:
    case EGameMode::PREP:
        SetPaused(true);
        break;

    case EGameMode::PLAY:
        if (!SimInstance)
        {
            CreateWorld();
        }

        SetTimeCompression(1);
        SetPaused(false);
        break;

    case EGameMode::MENU:
        SetPaused(true);

        if (OldMode == EGameMode::PLAY)
        {
            if (SimInstance)
            {
                SimInstance->UnloadMission();
            }
        }
        break;

    case EGameMode::CMPN:
    case EGameMode::PLAN:
        SetPaused(true);
        break;

    default:
        SetPaused(true);
        break;
    }

    GameMode = NewMode;
}

EGameMode USSWRuntimeSubsystem::GetGameMode() const
{
    return GameMode;
}

void USSWRuntimeSubsystem::HandleMenuState()
{
    if (MusicManager::GetInstance() &&
        MusicManager::GetInstance()->GetMode() != MusicMode::CREDITS)
    {
        MusicManager::SetMode(MusicMode::MENU);
    }
}

void USSWRuntimeSubsystem::HandleLoadState()
{
    if (GameMode == EGameMode::CLOD)
    {
        MusicManager::SetMode(MusicMode::MENU);
    }
    else
    {
        MusicManager::SetMode(MusicMode::BRIEFING);
    }
}

void USSWRuntimeSubsystem::HandlePlanState()
{
    MusicManager::SetMode(MusicMode::BRIEFING);
}

void USSWRuntimeSubsystem::HandleCampaignState()
{
}

void USSWRuntimeSubsystem::HandlePlayState()
{
    MusicManager::SetMode(MusicMode::FLIGHT);
}

void USSWRuntimeSubsystem::HandleExitState()
{
    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] EXIT state requested."));

    StopRuntime();

    if (UWorld* WorldContext = GetWorld())
    {
        UKismetSystemLibrary::QuitGame(
            WorldContext,
            nullptr,
            EQuitPreference::Quit,
            true
        );
    }
}

void USSWRuntimeSubsystem::MapKeys()
{
    const int32 NumKeys = KeyCfg.GetNumKeys();

    if (NumKeys <= 0)
    {
        UE_LOG(LogSSWRuntime, Warning,
            TEXT("[RUNTIME] No runtime key mappings available."));
        return;
    }

    MapKeys(&KeyCfg, NumKeys);

    if (RuntimeInput)
    {
        RuntimeInput->MapKeys(KeyCfg.GetMapping(), NumKeys);
    }

    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] Applied runtime key mappings. NumKeys=%d"),
        NumKeys);
}

void USSWRuntimeSubsystem::MapKeys(KeyMap* Mapping, int32 NumKeys)
{
    if (!Mapping)
        return;

    for (int32 Index = 0; Index < NumKeys; ++Index)
    {
        KeyMapEntry* Entry = Mapping->GetKeyMap(Index);

        if (!Entry)
            continue;

        if (Entry->act >= KEY_MAP_FIRST && Entry->act <= KEY_MAP_LAST)
        {
            MapKey(Entry->act, Entry->key, Entry->alt);
        }
    }
}

void USSWRuntimeSubsystem::MapKey(int32 Action, int32 Key, int32 Alt)
{
    if (Action < 0 || Action >= 256)
        return;

    KeyMapArray[Action] = Key;
    KeyAltArray[Action] = Alt;

#if PLATFORM_WINDOWS
    GetAsyncKeyState(Key);
    GetAsyncKeyState(Alt);
#endif
}

void USSWRuntimeSubsystem::SetupSplash()
{
    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] SetupSplash skipped - Unreal boot/menu flow owns splash."));
}

uint32 USSWRuntimeSubsystem::GetTimeCompression() const
{
    return TimeCompression;
}

void USSWRuntimeSubsystem::SetTimeCompression(uint32 Comp)
{
    if (Comp > 0 && Comp <= 100)
    {
        UE_LOG(LogSSWRuntime, Log,
            TEXT("[RUNTIME] TimeCompression %u -> %u"),
            TimeCompression,
            Comp);

        TimeCompression = Comp;
    }
}

void USSWRuntimeSubsystem::SetPaused(bool bInPaused)
{
    if (bPaused == bInPaused)
        return;

    bPaused = bInPaused;

    UE_LOG(LogSSWRuntime, Log,
        TEXT("[RUNTIME] Pause state: %s"),
        bPaused ? TEXT("PAUSED") : TEXT("RUNNING"));
}

bool USSWRuntimeSubsystem::IsPaused() const
{
    return bPaused;
}

void USSWRuntimeSubsystem::StartOrResumeGame()
{
    UE_LOG(LogSSWRuntime, Log, TEXT("[RUNTIME] StartOrResumeGame"));

    SetPaused(false);
    SetTimeCompression(1);
    SetGameMode(EGameMode::PLAY);
}

void USSWRuntimeSubsystem::GetPlayerCam(int32 Mode)
{
    if (!CamDir)
    {
        CamDir = CameraManager::GetInstance();
    }

    if (!CamDir)
    {
        UE_LOG(LogSSWRuntime, Warning,
            TEXT("[RUNTIME] PlayerCam failed. CameraManager is null."));
        return;
    }

    CamDir->SetMode(Mode);

    UE_LOG(LogSSWRuntime, Verbose,
        TEXT("[RUNTIME] PlayerCam Mode=%d"),
        Mode);
}

AShipActor*
USSWRuntimeSubsystem::SpawnVisualForRuntimeShip(Ship* RuntimeShip)
{
    if (!RuntimeShip)
    {
        return nullptr;
    }

    // One managed visual per simulation object. Do not deduplicate by ship name:
    // distinct mission elements/regions can legitimately reuse a callsign.
    for (int32 Index = MissionPresentationShips.Num()-1; Index >= 0; --Index)
    {
        AShipActor* Existing = MissionPresentationShips[Index].Get();
        if (!Existing || Existing->IsActorBeingDestroyed())
        {
            MissionPresentationShips.RemoveAtSwap(Index);
            continue;
        }
        if (Existing->GetRuntimeShip() == RuntimeShip)
        {
            UE_LOG(LogSSWRuntime, Verbose, TEXT("[RuntimeVisual] Reusing actor '%s' for Ship=%p"),
                *Existing->GetName(), RuntimeShip);
            return Existing;
        }
    }

    UWorld* World =
        GetWorld();

    if (!World)
    {
        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeVisual] No World for Ship='%hs'"),
            RuntimeShip->GetName());

        return nullptr;
    }

    const ShipDesign* Design =
        RuntimeShip->Design();

    if (!Design)
    {
        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeVisual] No Design for Ship='%hs'"),
            RuntimeShip->GetName());

        return nullptr;
    }

    const FString DesignName =
        ANSI_TO_TCHAR(Design->name);

    const FShipDesign* Row =
        ShipDesignRegistry::Find(DesignName);

    if (!Row)
    {
        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeVisual] No FShipDesign row for Ship='%hs' Design='%s'"),
            RuntimeShip->GetName(),
            *DesignName);

        return nullptr;
    }

    const FString ModelName =
        !Row->Model.IsEmpty()
        ? Row->Model
        : DesignName;

    const FString BPClassPath =
        FString::Printf(
            TEXT("/Game/Models/%s/BP_%s.BP_%s_C"),
            *ModelName,
            *ModelName,
            *ModelName);

    UClass* BPClass =
        LoadClass<AShipActor>(
            nullptr,
            *BPClassPath);

    if (!BPClass)
    {
        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeVisual] Missing BP class '%s'"),
            *BPClassPath);

        return nullptr;
    }

    const FVector SpawnLoc =
        GetVisualSpawnLocationForRuntimeShip(
            RuntimeShip);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    if (ASystemSceneBuilder* Builder = MissionPresentationBuilder.Get())
    {
        Params.Owner = Builder;
        Params.OverrideLevel = Builder->GetLevel();
    }
    AShipActor* ShipActor =
        World->SpawnActor<AShipActor>(
            BPClass,
            SpawnLoc,
            FRotator::ZeroRotator,
            Params);

    if (!ShipActor)
    {
        UE_LOG(LogSSWRuntime, VeryVerbose,
            TEXT("[RuntimeVisual] Spawn failed Ship='%hs'"),
            RuntimeShip->GetName());

        return nullptr;
    }

#if WITH_EDITOR
    ShipActor->SetActorLabel(
        FString(
            ANSI_TO_TCHAR(
                RuntimeShip->GetName())));
#endif

    // Register even when no presentation builder exists, so the next
    // Begin/EndMissionPresentation can remove pre-mission visual proxies.
    MissionPresentationShips.AddUnique(ShipActor);
    ShipActor->BindRuntimeShip(
        RuntimeShip);

    // Runtime positions already include the region transform. Parenting to a
    // presentation region would apply its transform a second time between updates.
    ShipActor->UpdateFromRuntimeShip(
        0.0f);

    UE_LOG(LogSSWRuntime, VeryVerbose,
        TEXT("[RuntimeVisual] Spawned Ship='%hs' Actor='%s' Model='%s' Loc=%s"),
        RuntimeShip->GetName(),
        *ShipActor->GetName(),
        *ModelName,
        *SpawnLoc.ToString());

    return ShipActor;
}

FVector
USSWRuntimeSubsystem::ConvertLegacyShipLocationToUE(
    const FVector& LegacyLoc) const
{
    return FVector(
        LegacyLoc.Z,
        LegacyLoc.X,
        LegacyLoc.Y);
}

AActor*
USSWRuntimeSubsystem::FindRegionActorForRuntimeShip(
    Ship* RuntimeShip) const
{
    if (!RuntimeShip ||
        !RuntimeShip->GetRegion())
    {
        return nullptr;
    }

    UWorld* World =
        GetWorld();

    if (!World)
    {
        return nullptr;
    }

    const FString RegionName =
        FString(
            ANSI_TO_TCHAR(
                RuntimeShip->GetRegion()->GetName()))
        .TrimStartAndEnd();

    if (RegionName.IsEmpty())
    {
        return nullptr;
    }

    const FString RegionActorNameA =
        RegionName + TEXT("_REGION");

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Candidate =
            *It;

        if (!IsValid(Candidate))
        {
            continue;
        }

        const FString ActorName =
            Candidate->GetName();

        const FString ActorLabel =
#if WITH_EDITOR
            Candidate->GetActorLabel();
#else
            ActorName;
#endif

        if (ActorName.Equals(RegionActorNameA, ESearchCase::IgnoreCase) ||
            ActorLabel.Equals(RegionActorNameA, ESearchCase::IgnoreCase) ||
            ActorName.Contains(RegionName, ESearchCase::IgnoreCase) &&
            ActorName.Contains(TEXT("REGION"), ESearchCase::IgnoreCase) ||
            ActorLabel.Contains(RegionName, ESearchCase::IgnoreCase) &&
            ActorLabel.Contains(TEXT("REGION"), ESearchCase::IgnoreCase))
        {
            UE_LOG(LogSSWRuntime, VeryVerbose,
                TEXT("[RuntimeVisual] RegionActor Ship='%hs' Region='%s' Actor='%s' Label='%s'"),
                RuntimeShip->GetName(),
                *RegionName,
                *ActorName,
                *ActorLabel);

            return Candidate;
        }
    }

    UE_LOG(LogSSWRuntime, VeryVerbose,
        TEXT("[RuntimeVisual] No RegionActor found Ship='%hs' Region='%s'"),
        RuntimeShip->GetName(),
        *RegionName);

    return nullptr;
}

FVector
USSWRuntimeSubsystem::GetRegionVisualLocationForRuntimeShip(
    Ship* RuntimeShip) const
{
    return RuntimeShip ? GetVisualLocationForRegion(RuntimeShip->GetRegion(), RuntimeShip->GetLocation()) : FVector::ZeroVector;
}



void USSWRuntimeSubsystem::BeginMissionPresentation(ASystemSceneBuilder* Builder)
{
    EndMissionPresentation();
    MissionPresentationBuilder = Builder;
    if (Builder && Sim::GetSim() && Sim::GetSim()->GetActiveRegion())
    {
        Ship* PlayerShip = Sim::GetSim()->GetPlayerShip();
        Builder->UpdateMissionSpace(Sim::GetSim()->GetActiveRegion()->GetLocation(),
            PlayerShip ? ConvertLegacyShipLocationToUE(PlayerShip->GetLocation()) : FVector::ZeroVector);
    }
}
void USSWRuntimeSubsystem::HandleMissionUnloading()
{
    SetPaused(true);
    SetTimeCompression(1);
    if (UGameInstance* GI = GetGameInstance())
        if (UTimerSubsystem* Timer = GI->GetSubsystem<UTimerSubsystem>()) Timer->StopMissionRun();
    EndMissionPresentation();
}

void USSWRuntimeSubsystem::EndMissionPresentation()
{
    ClearMissionWaypoints();
    ClearProjectileVisuals();
    for (const TWeakObjectPtr<AShipActor>& Actor : MissionPresentationShips)
    {
        if (!Actor.IsValid()) continue;
        Actor->SetActorTickEnabled(false);
        Actor->BindRuntimeShip(nullptr);
        Actor->Destroy();
    }
    MissionPresentationShips.Empty();
    MissionPresentationOffsets.Empty();
    MissionPresentationRadii.Empty();
    if (ASystemSceneBuilder* Builder = MissionPresentationBuilder.Get())
        { Builder->EndMissionSpace(); Builder->ResetTemporaryCutsceneBodyScales(); }
    MissionPresentationBuilder.Reset();
}

// Deck positions must share the carrier's visual frame: region-shell offsets,
// per-group depth and overlap spacing are not valid inside a hangar.
bool USSWRuntimeSubsystem::GetCarrierVisualPoint(Ship* Carrier, const FVector& SimPoint, FVector& OutPoint) const
{
    if (!Carrier || !Carrier->GetRegion()) return false;
    OutPoint = GetVisualLocationForRegion(Carrier->GetRegion(), SimPoint);
    return true;
}

FVector USSWRuntimeSubsystem::GetVisualSpawnLocationForRuntimeShip(Ship* RuntimeShip) const
{
    // Ship positions already use simulation handedness. No extra deck interpolation,
    // radius-dependent displacement, random depth or collision-based visual offsets.
    return GetRegionVisualLocationForRuntimeShip(RuntimeShip);
}

void USSWRuntimeSubsystem::ClearProjectileVisuals()
{
    for(auto& Pair:ProjectileVisuals)if(Pair.Value.IsValid())Pair.Value->Destroy();
    ProjectileVisuals.Empty(); ProjectileMeshes.Empty(); ProjectileFallbackMesh=nullptr;
}
void USSWRuntimeSubsystem::UpdateProjectileVisuals()
{
    UWorld* World=GetWorld();
    SimRegion* Region=SimInstance?SimInstance->GetActiveRegion():nullptr;
    if(!World || !Region || !MissionPresentationBuilder.IsValid()){ClearProjectileVisuals();return;}
    TSet<SimShot*> Alive;
    auto& Shots=Region->GetShots();
    for(int I=0;I<Shots.size();++I){
        SimShot* Shot=Shots[I]; if(!Shot || Shot->GetLife()<=0)continue;
        Ship* Owner=const_cast<Ship*>(Shot->Owner());
        if(!Owner || !Region->GetShips().contains(Owner))continue;
        // Match the owner's presentation offset. These spheres never collide in UE:
        // simulation shots remain authoritative for movement and damage.
        const FVector Offset=GetVisualSpawnLocationForRuntimeShip(Owner)-ConvertLegacyShipLocationToUE(Owner->GetLocation());
        Ship* TargetShip=dynamic_cast<Ship*>(Shot->GetTarget());
        if(TargetShip && !Region->GetShips().contains(TargetShip))TargetShip=nullptr;
        auto VisualPoint=[this,Owner,TargetShip,Offset](const FVector& Point){
            FVector Result=ConvertLegacyShipLocationToUE(Point)+Offset;
            if(TargetShip){
                const FVector Leg=TargetShip->GetLocation()-Owner->GetLocation();
                const double Along=Leg.SizeSquared()>SMALL_NUMBER?FMath::Clamp(
                    FVector::DotProduct(Point-Owner->GetLocation(),Leg)/Leg.SizeSquared(),0.0,1.0):0.0;
                const FVector TargetOffset=GetVisualSpawnLocationForRuntimeShip(TargetShip)-ConvertLegacyShipLocationToUE(TargetShip->GetLocation());
                Result+=(TargetOffset-Offset)*Along;
            }
            return Result;
        };
        const FVector Position=VisualPoint(Shot->GetLocation());
        if(Shot->IsPrimary()){
            const FVector Start=VisualPoint(Shot->GetOrigin());
            if(Shot->IsBeam()){
                Alive.Add(Shot);
                ABeamWeaponActor* Beam=Cast<ABeamWeaponActor>(ProjectileVisuals.FindRef(Shot).Get());
                if(!Beam){
                    TSubclassOf<ABeamWeaponActor> BeamClass=LoadClass<ABeamWeaponActor>(nullptr,TEXT("/Game/Weapons/BP_Laser.BP_Laser_C"));
                    Beam=ABeamWeaponActor::SpawnBeam(World,BeamClass,Start,Position,nullptr);
                    if(Beam){
                        Beam->SetBeamColor(FLinearColor(0.16f,0.7f,1.0f));
                        ProjectileVisuals.Add(Shot,Beam);
                    }
                }
                if(Beam) Beam->SetBeamPoints(Start,Position);
                else DrawDebugLine(World,Start,Position,FColor::Cyan,false,0.0f,0,2.0f);
            }
            else{
                DrawDebugLine(World,Start,Position,FColor::Yellow,false,0.0f,0,2.0f);
            }
            continue;
        }
        Alive.Add(Shot);
        AStaticMeshActor* Visual=Cast<AStaticMeshActor>(ProjectileVisuals.FindRef(Shot).Get());
        if(!Visual && ProjectileVisuals.Num()<512){
            if(!ProjectileFallbackMesh)ProjectileFallbackMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
            UStaticMesh* Mesh=ProjectileFallbackMesh;
            const WeaponDesign* Design=Shot->GetDesign();
            const FString Model=Design?FString(ANSI_TO_TCHAR(Design->model.data())):FString();
            // A UE object path in the existing model field can supply the later asset.
            if(Model.StartsWith(TEXT("/Game/"))){
                if(!ProjectileMeshes.Contains(Model))ProjectileMeshes.Add(Model,LoadObject<UStaticMesh>(nullptr,*Model));
                if(ProjectileMeshes[Model])Mesh=ProjectileMeshes[Model];
            }
            if(!Mesh)continue;
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Params.ObjectFlags|=RF_Transient;
            Visual=World->SpawnActor<AStaticMeshActor>(Position,FRotator::ZeroRotator,Params);
            if(!Visual)continue;
            auto* Component=Visual->GetStaticMeshComponent();Component->SetMobility(EComponentMobility::Movable);
            Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetCastShadow(false);
            Component->SetStaticMesh(Mesh);
            if(Mesh==ProjectileFallbackMesh)Visual->SetActorScale3D(FVector(0.1f));
            ProjectileVisuals.Add(Shot,Visual);
        }
        if(Visual)Visual->SetActorLocationAndRotation(Position,ConvertLegacyShipLocationToUE(Shot->GetVelocity()).Rotation());
    }
    for(auto It=ProjectileVisuals.CreateIterator();It;++It)if(!Alive.Contains(It.Key())){
        if(It.Value().IsValid())It.Value()->Destroy();It.RemoveCurrent();
    }
}

FVector USSWRuntimeSubsystem::GetVisualLocationForRegion(SimRegion* Region, const FVector& SimPoint) const
{
    const FVector Local = ConvertLegacyShipLocationToUE(SimPoint);
    ASystemSceneBuilder* Builder = MissionPresentationBuilder.Get();
    Sim* LiveSim = Sim::GetSim();
    SimRegion* Active = LiveSim ? LiveSim->GetActiveRegion() : nullptr;
    if (Builder && Active && Region)
        return Builder->GetActorLocation() + ConvertLegacyShipLocationToUE(OtherHand(Region->GetLocation() - Active->GetLocation())) + Local;
    return Local;
}

void USSWRuntimeSubsystem::ClearMissionWaypoints()
{
    for (auto& Pair : MissionWaypoints) if (Pair.Value.IsValid()) Pair.Value->Destroy();
    MissionWaypoints.Empty();
    WaypointRefreshSeconds = 0;
    WaypointClass = nullptr;
    bWaypointClassResolved = false;
}

void USSWRuntimeSubsystem::UpdateMissionWaypoints()
{
    Sim* LiveSim = Sim::GetSim();
    UWorld* World = GetWorld();
    ASystemSceneBuilder* Builder = MissionPresentationBuilder.Get();
    if (!LiveSim || !LiveSim->GetMission() || !World || !Builder) return;
    if (!bWaypointClassResolved)
    {
        bWaypointClassResolved = true;
        WaypointClass = LoadClass<AWaypointActor>(nullptr, TEXT("/Game/Models/Waypoint/BP_Waypoint.BP_Waypoint_C"));
        if (!WaypointClass) WaypointClass = LoadClass<AWaypointActor>(nullptr, TEXT("/Game/Actors/BP_Waypoint.BP_Waypoint_C"));
        if (!WaypointClass) UE_LOG(LogSSWRuntime, Warning, TEXT("Create /Game/Actors/BP_Waypoint with the supplied editor script. Using hidden native waypoint actors until then."));
    }
    TSet<FString> ActiveKeys;
    ListIter<SimElement> Elements = LiveSim->GetElements();
    while (++Elements)
    {
        SimElement* Element = Elements.value();
        if (!Element || Element->IsFinished() || Element->IsSquadron()) continue;
        ListIter<Instruction> Plan = Element->GetFlightPlan();
        int32 Index = 0;
        while (++Plan)
        {
            Instruction* Point = Plan.value();
            ++Index;
            if (!Point || !Point->GetRegion()) continue;
            const FString Key = FString::Printf(TEXT("%d:%d"), Element->GetIdentity(), Index);
            ActiveKeys.Add(Key);
            AWaypointActor* Actor = MissionWaypoints.FindRef(Key).Get();
            if (!IsValid(Actor))
            {
                FActorSpawnParameters Params;
                Params.Owner = Builder;
                Params.OverrideLevel = Builder->GetLevel();
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                UClass* Class = WaypointClass ? WaypointClass.Get() : AWaypointActor::StaticClass();
                Actor = World->SpawnActor<AWaypointActor>(Class, FVector::ZeroVector, FRotator::ZeroRotator, Params);
                if (!Actor) continue;
                MissionWaypoints.Add(Key, Actor);
            }
            Ship* RadiusShip = LiveSim->GetPlayerShip();
            if (!RadiusShip || RadiusShip->GetElement() != Element) RadiusShip = Element->GetShip(1);
            Actor->SetArrivalHullRadius(RadiusShip ? RadiusShip->GetNavigationHullRadius() : 0.0);
            Actor->ElementIdentity = Element->GetIdentity();
            Actor->WaypointIndex = Index;
            Actor->SectorName = ANSI_TO_TCHAR(Point->GetRegionName());
            Actor->Description = ANSI_TO_TCHAR(Point->GetDescription());
            Actor->LegacyLocation = Point->GetLocation();
            Actor->bNextWaypoint = Point == Element->GetNextNavPoint();
            Actor->SetActorLocation(GetVisualLocationForRegion(Point->GetRegion(), OtherHand(Point->GetLocation())));
            Actor->SetActorHiddenInGame(true);
            Actor->SetActorEnableCollision(false);
#if WITH_EDITOR
            Actor->SetActorLabel(FString::Printf(TEXT("NAV %hs %d"), Element->GetName().data(), Index));
#endif
        }
    }
    for (auto It = MissionWaypoints.CreateIterator(); It; ++It)
        if (!ActiveKeys.Contains(It.Key())) { if (It.Value().IsValid()) It.Value()->Destroy(); It.RemoveCurrent(); }
}
