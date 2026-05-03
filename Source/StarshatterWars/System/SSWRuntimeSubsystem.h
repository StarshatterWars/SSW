/*
    Project Starshatter Wars
    Fractal Dev Studios

    FILE:         SSWRuntimeSubsystem.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Owns the Starshatter game loop under Unreal Engine.

    BootSubsystem:
        Loads assets and tables

    GameInitSubsystem:
        Calls Init / InitGame / StartRuntime

    RuntimeSubsystem (this class):
        Owns GameLoop execution every frame
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "GameStructs.h"
#include "KeyMap.h"
#include "SSWRuntimeSubsystem.generated.h"

class Starshatter;
class SimUniverse;
class CameraManager;
class MultiController;

UCLASS()
class STARSHATTERWARS_API USSWRuntimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    bool Init();
    bool InitGame();

    void CreateWorld();

    bool GameLoop();

    void UpdateWorld();
    void GameState();
    void UpdateScreen();
    void CollectStats();

public:
    void StartRuntime();
    void StopRuntime();

    void StartOrResumeGame();

    void SetGameMode(EGameMode NewMode);
    EGameMode GetGameMode() const;

    void SetPaused(bool bInPaused);
    bool IsPaused() const;

    uint32 GetTimeCompression() const;
    void SetTimeCompression(uint32 Comp);

    KeyMap& GetKeyMap() { return KeyCfg; }
    void MapKeys();

    void GetPlayerCam(int32 Mode);

    int32 GetLoadProgress() const { return LoadProgress; }
    const char* GetLoadActivity() const { return TCHAR_TO_ANSI(*LoadActivity); }

    void SetLoadProgress(int32 NewProgress) { LoadProgress = NewProgress; }
    void SetLoadActivity(const FString& NewActivity) { LoadActivity = NewActivity; }

    bool IsRuntimeInitialized() const { return bRuntimeInitialized; }
    bool IsRuntimeRunning() const { return bRuntimeRunning; }

private:
    bool TickRuntime(float DeltaSeconds);

private:
    void InitMouse();

    void MapKeys(KeyMap* Mapping, int32 NumKeys);
    void MapKey(int32 Action, int32 Key, int32 Alt);

    void SetupSplash();

private:
    void HandleMenuState();
    void HandleLoadState();
    void HandlePlanState();
    void HandleCampaignState();
    void HandlePlayState();
    void HandleExitState();

private:
    bool bRuntimeInitialized = false;
    bool bRuntimeRunning = false;
    bool bRuntimeActive = true;
    bool bPaused = true;
    bool bNoSplash = true;

    float LastDeltaSeconds = 0.0f;

    double TimeMark = 0.0;
    int32 Minutes = 0;

    uint32 TimeCompression = 1;

    EGameMode GameMode = EGameMode::INIT;

    int32 LoadProgress = 0;
    FString LoadActivity;

    Starshatter* LegacyGame = nullptr;

    SimUniverse* World = nullptr;
    CameraManager* CamDir = nullptr;

    MultiController* RuntimeInput = nullptr;

    KeyMap KeyCfg;

    int32 KeyMapArray[256] = {};
    int32 KeyAltArray[256] = {};

    FTSTicker::FDelegateHandle RuntimeTickerHandle;
}; 