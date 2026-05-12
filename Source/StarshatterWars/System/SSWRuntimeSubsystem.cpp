#include "SSWRuntimeSubsystem.h"

#include "Starshatter.h"

#include "Game.h"
#include "Sim.h"
#include "SimUniverse.h"
#include "Galaxy.h"
#include "Campaign.h"
#include "CameraManager.h"
#include "EventDispatch.h"

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
    RadioVox::Initialize();
    QuantumView::Initialize();
    TacticalView::Initialize();

    if (!SimInstance)
    {
        SimInstance = new Sim(RuntimeInput);

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

void USSWRuntimeSubsystem::GameLoop()
{
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

    if (SimInstance)
    {
        const uint32 BeforeTime =
            Game::GetGameTime();

        SimInstance->ExecFrame(Seconds);

        const uint32 AfterTime =
            Game::GetGameTime();

        UE_LOG(LogSSWRuntime, Warning,
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