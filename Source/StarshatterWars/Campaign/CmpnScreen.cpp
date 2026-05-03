/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CmpnScreen.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UCmpnScreen
    - Legacy-faithful campaign screen manager.
    - UCmdDlg is the main campaign hub.
    - UCmpnScreen owns campaign overlays and preserves
      legacy campaign flow logic.
*/

#include "CmpnScreen.h"

// UE:
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LevelStreamingDynamic.h"
#include "ContentStreaming.h"

// Dialogs:
#include "CmdDlg.h"
#include "CmdMsgDlg.h"
#include "CmpFileDlg.h"
#include "CmpCompleteDlg.h"
#include "CmpLoadDlg.h"
#include "CampaignSceneDlg.h"
#include "ShaderPipelineCache.h"
#include "MenuScreen.h"

// Legacy/runtime:
#include "Campaign.h"
#include "Starshatter.h"
#include "CombatEvent.h"
#include "PlayerCharacter.h"
#include "Mouse.h"
#include "Game.h"
#include "MusicManager.h"
#include "Sim.h"
#include "Ship.h"
#include "Keyboard.h"
#include "GameStructs.h"
#include "SSWRuntimeSubsystem.h"

#include "Engine/DataTable.h"
#include "StarshatterGameDataSubsystem.h"

UCmpnScreen::UCmpnScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCmpnScreen::NativeConstruct()
{
    Super::NativeConstruct();

    Setup();

    SetVisibility(ESlateVisibility::Hidden);
    SetDialogInputEnabled(false);
}

void UCmpnScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bIsShown)
    {
        ExecFrame((double) InDeltaTime);
    }
}

template<typename TDialog>
TDialog* UCmpnScreen::EnsureDialog(TSubclassOf<TDialog> ClassToSpawn, TObjectPtr<TDialog>& Storage, int32 ZOrder)
{
    if (Storage)
    {
        return Storage.Get();
    }

    if (!ClassToSpawn)
    {
        return nullptr;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        if (UWorld* World = GetWorld())
        {
            PC = UGameplayStatics::GetPlayerController(World, 0);
        }
    }

    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmpnScreen] EnsureDialog: no player controller"));
        return nullptr;
    }

    TDialog* Created = CreateWidget<TDialog>(PC, ClassToSpawn);
    if (!Created)
    {
        return nullptr;
    }

    Created->AddToViewport(ZOrder);
    Created->SetVisibility(ESlateVisibility::Hidden);
    Created->SetIsEnabled(false);

    Storage = Created;
    return Created;
}

void UCmpnScreen::RefreshRuntimePointers()
{
    Stars = Starshatter::GetInstance();
    CampaignPtr = Campaign::GetCampaign();
}

void UCmpnScreen::ApplyManagerToChildren()
{
    if (CmdDlg)
    {
        CmdDlg->SetManager(this);

        if (MenuManager)
        {
            CmdDlg->SetMenuManager(MenuManager);
            CmdDlg->InitializeDlg(MenuManager);
        }
    }

    if (CmdMsgDlg)
    {
        CmdMsgDlg->SetCmpnScreen(this);
    }

    if (CmpSceneDlg)
    {
        CmpSceneDlg->SetManager(this);
    }

    if (CmpLoadDlg)
    {
        CmpLoadDlg->SetCmpnScreen(this);
    }
}

const FS_CampaignMission* UCmpnScreen::FindCampaignMissionByScene(const FString& SceneName) const
{
    if (SceneName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] FindCampaignMissionByScene: SceneName is empty"));
        return nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] FindCampaignMissionByScene: World is null"));
        return nullptr;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] FindCampaignMissionByScene: GameInstance is null"));
        return nullptr;
    }

    UStarshatterGameDataSubsystem* DataSys = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSys)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] FindCampaignMissionByScene: Data subsystem is null"));
        return nullptr;
    }

    const FS_Campaign* CampaignRow = nullptr;

    if (DataSys->SelectedCampaignRowName != NAME_None)
    {
        CampaignRow = DataSys->GetCampaignByRow(DataSys->SelectedCampaignRowName);
    }

    if (!CampaignRow)
    {
        CampaignRow = DataSys->GetActiveCampaignPtr();
    }

    if (!CampaignRow)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] No campaign available for Scene=%s"),
            *SceneName);
        return nullptr;
    }

    for (const FS_CampaignMission& MissionRow : CampaignRow->Missions)
    {
        if (MissionRow.Scene.Equals(SceneName, ESearchCase::IgnoreCase))
        {
            UE_LOG(LogTemp, Log,
                TEXT("[CmpnScreen] Found scene mission: MissionId=%d Scene=%s MissionName=%s"),
                MissionRow.MissionId,
                *MissionRow.Scene,
                *MissionRow.MissionName);

            return &MissionRow;
        }
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmpnScreen] No campaign mission found for Scene=%s"),
        *SceneName);

    return nullptr;
}

CombatEvent* UCmpnScreen::FindFirstPendingSceneEvent() const
{
    if (!CampaignPtr || !CampaignPtr->IsActive() || CampaignPtr->GetEvents().isEmpty())
    {
        return nullptr;
    }

    ListIter<CombatEvent> Iter = CampaignPtr->GetEvents();
    while (++Iter)
    {
        CombatEvent* Event = Iter.value();
        if (Event && !Event->Visited() && Event->SceneFile() && *Event->SceneFile())
        {
            return Event;
        }
    }

    return nullptr;
}

void UCmpnScreen::SetMenuManager(UMenuScreen* InManager)
{
    MenuManager = InManager;
    ApplyManagerToChildren();
}

void UCmpnScreen::InitializeDlg(UMenuScreen* InManager)
{
    MenuManager = InManager;
    ApplyManagerToChildren();
}

void UCmpnScreen::Setup()
{
    if (bSetupComplete)
    {
        return;
    }

    RefreshRuntimePointers();

    EnsureDialog<UCmdDlg>(CmdDlgClass, CmdDlg, 400);
    EnsureDialog<UCmpFileDlg>(CmpFileDlgClass, CmpFileDlg, 500);
    EnsureDialog<UCmdMsgDlg>(CmdMsgDlgClass, CmdMsgDlg, 510);
    EnsureDialog<UCmpCompleteDlg>(CmpCompleteDlgClass, CmpCompleteDlg, 520);
    EnsureDialog<UCampaignSceneDlg>(CmpSceneDlgClass, CmpSceneDlg, 530);
    EnsureDialog<UCmpLoadDlg>(CmpLoadDlgClass, CmpLoadDlg, 525);

    ApplyManagerToChildren();

    HideAll();

    CompletionStage = 0;
    TimeTilChange = 0.0;
    bExitLatch = false;
    bShowMissionsRequested = false;
    bCampaignPaused = false;

    ClearPendingSceneTransition();

    bSetupComplete = true;
}

void UCmpnScreen::TearDown()
{
    auto Kill = [](UUserWidget*& W)
    {
        if (W)
        {
            W->RemoveFromParent();
            W = nullptr;
        }
    };

    UUserWidget* W = nullptr;

    W = CmdDlg.Get();         Kill(W); CmdDlg = nullptr;
    W = CmpFileDlg.Get();     Kill(W); CmpFileDlg = nullptr;
    W = CmdMsgDlg.Get();      Kill(W); CmdMsgDlg = nullptr;
    W = CmpCompleteDlg.Get(); Kill(W); CmpCompleteDlg = nullptr;
    W = CmpSceneDlg.Get();    Kill(W); CmpSceneDlg = nullptr;
    W = CmpLoadDlg.Get();     Kill(W); CmpLoadDlg = nullptr;

    ActiveSceneStreamingLevel = nullptr;

    bSetupComplete = false;
    bIsShown = false;
    bShowMissionsRequested = false;
    bExitLatch = false;
    bCampaignPaused = false;
    TimeTilChange = 0.0;
    CompletionStage = 0;

    DefaultFallbackFOV = 90.0f;
    DesiredFieldOfView = 90.0f;

    ActiveSceneName.Empty();
    ActiveSceneDurationSeconds = 0.0f;
    ActiveCampaignName.Empty();

    ClearPendingSceneTransition();
}

void UCmpnScreen::Show()
{
    if (bIsShown)
    {
        return;
    }

    RefreshRuntimePointers();
    ApplyManagerToChildren();

    bIsShown = true;
    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
    SetDialogInputEnabled(true);

    CompletionStage = 0;
    DesiredFieldOfView = GetFieldOfView();
    bCampaignPaused = false;

    CombatEvent* StartupSceneEvent = FindFirstPendingSceneEvent();
    if (StartupSceneEvent && PrimeSceneTransitionForEvent(StartupSceneEvent))
    {
        return;
    }

    ShowCmdDlg();
}

void UCmpnScreen::Hide()
{
    if (!bIsShown)
    {
        return;
    }

    HideAll();
    SetDialogInputEnabled(false);
    SetVisibility(ESlateVisibility::Hidden);
    bIsShown = false;
}

void UCmpnScreen::HideAll()
{
    bHidingAll = true;

    HideCmdDlg();
    HideCmpFileDlg();
    HideCmdMsgDlg();
    HideCmpCompleteDlg();
    HideCmpSceneDlg();
    HideCmpLoadDlg();

    bHidingAll = false;
}

bool UCmpnScreen::CloseTopmost()
{
    if (IsCmdMsgShown())
    {
        HideCmdMsgDlg();
        return true;
    }

    if (IsCmpFileShown())
    {
        HideCmpFileDlg();
        return true;
    }

    return false;
}

void UCmpnScreen::ExecFrame(double DeltaTime)
{
    RefreshRuntimePointers();

    if (bSceneTransitionActive && bHasPendingSceneMission)
    {
        ContinuePendingSceneTransition();
    }
    else if (!IsCmpSceneShown())
    {
        CombatEvent* PendingEvent = FindFirstPendingSceneEvent();
        if (PendingEvent)
        {
            PrimeSceneTransitionForEvent(PendingEvent);
        }
    }

    if (!Stars)
    {
        return;
    }

    AdvanceCampaignScene();

    Mouse::SetCursor(Mouse::ARROW);

    if (TimeTilChange > 0.0)
    {
        TimeTilChange -= DeltaTime;
        if (TimeTilChange < 0.0)
        {
            TimeTilChange = 0.0;
        }
    }

    const bool bInCutscene = (CmpSceneDlg && CmpSceneDlg->IsSceneRunning());

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    }

    const bool bExitPressed = PC && PC->IsInputKeyDown(EKeys::Escape);

#if WITH_EDITOR
    const bool bDebugSkipPressed = PC && PC->IsInputKeyDown(EKeys::SpaceBar);
#else
    const bool bDebugSkipPressed = false;
#endif

    const bool bFastForwardPressed = PC && PC->IsInputKeyDown(EKeys::F);

    bExitLatch = (bExitPressed || bDebugSkipPressed);

    Sim* sim = Sim::GetSim();
    Ship* PlayerShip = sim ? sim->GetPlayerShip() : nullptr;
    const bool bHasPlayerShip = (PlayerShip != nullptr);

    if (bInCutscene && bHasPlayerShip)
    {
        const float WarpFactor = PlayerShip->GetWarpFactor();

        if (WarpFactor > 1.0f)
        {
            if (WarpFactor > DesiredFieldOfView)
                SetFieldOfView(WarpFactor);
            else
                SetFieldOfView(DesiredFieldOfView);
        }
        else
        {
            if (GetFieldOfView() != DesiredFieldOfView)
                SetFieldOfView(DesiredFieldOfView);
        }
    }

    if (bInCutscene && bFastForwardPressed && CmpSceneDlg)
    {
        const float NowSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
        CmpSceneDlg->AdvanceSceneFromTimer(NowSeconds + 10.0f);
    }

    if (bInCutscene && bExitLatch)
    {
        TimeTilChange = 0.25;
        HideCmpSceneDlg();
        SetFieldOfView(DesiredFieldOfView);
        ShowCmdDlg();
    }
    else if (TimeTilChange <= 0.0 && bExitLatch)
    {
        TimeTilChange = 1.0;

        if (!CloseTopmost())
        {
            if (UGameInstance* GI = GetGameInstance())
            {
                if (USSWRuntimeSubsystem* RuntimeSS =
                    GI->GetSubsystem<USSWRuntimeSubsystem>())
                {
                    RuntimeSS->SetGameMode(EGameMode::MENU);
                }
            }
        }
    }
    UGameInstance* GI = GetGameInstance();
    USSWRuntimeSubsystem* RuntimeSS = GI
        ? GI->GetSubsystem<USSWRuntimeSubsystem>()
        : nullptr;

    if (RuntimeSS && RuntimeSS->GetGameMode() == EGameMode::CMPN)
    {
        if (TimeTilChange <= 0.0)
        {
            if (Keyboard::KeyDown(KEY_PAUSE))
            {
                TimeTilChange = 1.0;
                bCampaignPaused = !bCampaignPaused;
                RuntimeSS->SetPaused(bCampaignPaused);
            }
            else if (Keyboard::KeyDown(KEY_TIME_COMPRESS))
            {
                TimeTilChange = 1.0;

                switch ((int)RuntimeSS->GetTimeCompression())
                {
                case 1:  RuntimeSS->SetTimeCompression(2); break;
                case 2:  RuntimeSS->SetTimeCompression(4); break;
                case 4:  RuntimeSS->SetTimeCompression(8); break;
                default: break;
                }
            }
            else if (Keyboard::KeyDown(KEY_TIME_EXPAND))
            {
                TimeTilChange = 1.0;

                switch ((int)RuntimeSS->GetTimeCompression())
                {
                case 8:  RuntimeSS->SetTimeCompression(4); break;
                case 4:  RuntimeSS->SetTimeCompression(2); break;
                default: RuntimeSS->SetTimeCompression(1); break;
                }
            }
        }
    }

    if (bShowMissionsRequested && !bInCutscene)
    {
        if (CmdDlg)
        {
            ShowCmdDlg();
            CmdDlg->ShowMissionsPanel();
        }

        bShowMissionsRequested = false;
    }

    if (!CampaignPtr)
    {
        return;
    }

    if (IsCmdMsgShown())
    {
        // modal active
    }
    else if (IsCmpCompleteShown())
    {
        CompletionStage = 2;
    }
    else if (IsCmpSceneShown())
    {
        if (CompletionStage > 0)
        {
            CompletionStage = 2;
        }
    }
    else
    {
        if (CompletionStage == 0)
        {
            PlayerCharacter* PlayerPtr = PlayerCharacter::GetCurrentPlayer();
            if (!PlayerPtr)
            {
                return;
            }

            if (CampaignPtr->IsTraining())
            {
                const int32 AllMissionsMask = (1 << CampaignPtr->GetMissionList().size()) - 1;

                if (PlayerPtr->Trained() >= AllMissionsMask && PlayerPtr->Trained() < 255)
                {
                    PlayerPtr->SetTrained(255);

                    if (CmdMsgDlg)
                    {
                        CmdMsgDlg->SetTitleText(TEXT("TRAINING"));
                        CmdMsgDlg->SetMessageText(TEXT("Congratulations. Training complete."));
                        ShowCmdMsgDlg();
                    }

                    CompletionStage = 1;
                }
            }
            else if (CampaignPtr->IsComplete() || CampaignPtr->IsFailed())
            {
                bool bSceneStarted = false;
                CombatEvent* Event = CampaignPtr->GetLastEvent();

                if (Event && !Event->Visited() && Event->SceneFile() && *Event->SceneFile())
                {
                    bSceneStarted = TryStartSceneForEvent(Event);
                }

                if (!bSceneStarted)
                {
                    ShowCmpCompleteDlg();
                }

                if (CampaignPtr->IsComplete())
                    MusicManager::SetMode(MusicMode::VICTORY);
                else
                    MusicManager::SetMode(MusicMode::DEFEAT);

                CompletionStage = 1;
            }
        }
        else if (CompletionStage > 1)
        {
            CompletionStage = 0;

            GI = GetGameInstance();
            RuntimeSS = GI
                ? GI->GetSubsystem<USSWRuntimeSubsystem>()
                : nullptr;

            if (!RuntimeSS)
            {
                UE_LOG(LogTemp, Warning, TEXT("[Campaign] Runtime subsystem not found."));
                return;
            }

            if (CampaignPtr->IsTraining())
            {
                List<Campaign>& CampaignList = Campaign::GetAllCampaigns();
                Campaign* NextCampaign = CampaignList[1];

                if (NextCampaign)
                {
                    NextCampaign->Load();
                    Campaign::SelectCampaign(NextCampaign->GetName());

                    RuntimeSS->SetGameMode(EGameMode::CLOD);
                    return;
                }
            }

            if (CampaignPtr->GetCampaignId() < Campaign::GetLastCampaignId())
            {
                RuntimeSS->StartOrResumeGame();
            }
            else
            {
                Mouse::Show(false);
                MusicManager::SetMode(MusicMode::MENU);

                RuntimeSS->SetGameMode(EGameMode::MENU);
                return;
            }
        }
    }

    if (CompletionStage < 1)
    {
        MusicManager::SetMode(MusicMode::MENU);
        Mouse::Show(!IsCmpSceneShown());
    }
}

void UCmpnScreen::ShowCmdDlg()
{
    HideAll();

    if (CmdDlg)
    {
        CmdDlg->SetVisibility(ESlateVisibility::Visible);
        CmdDlg->SetIsEnabled(true);
        CmdDlg->SetIsFocusable(true);
        CmdDlg->SetDialogInputEnabled(true);
        CmdDlg->ShowCmdDlg();

        if (APlayerController* PC = GetOwningPlayer())
        {
            PC->bShowMouseCursor = true;
            PC->bEnableClickEvents = true;
            PC->bEnableMouseOverEvents = true;

            FInputModeGameAndUI Mode;
            Mode.SetWidgetToFocus(CmdDlg->TakeWidget());
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Mode.SetHideCursorDuringCapture(false);
            PC->SetInputMode(Mode);
        }

        Mouse::Show(true);
    }
}

void UCmpnScreen::HideCmdDlg()
{
    if (CmdDlg)
    {
        CmdDlg->SetDialogInputEnabled(false);
        CmdDlg->SetVisibility(ESlateVisibility::Hidden);
    }
}

bool UCmpnScreen::IsCmdShown() const
{
    return CmdDlg && CmdDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpFileDlg()
{
    if (CmpFileDlg)
    {
        CmpFileDlg->SetVisibility(ESlateVisibility::Visible);
        CmpFileDlg->SetIsEnabled(true);
        Mouse::Show(true);
    }
}

void UCmpnScreen::HideCmpFileDlg()
{
    if (CmpFileDlg)
    {
        CmpFileDlg->SetVisibility(ESlateVisibility::Hidden);
    }
}

bool UCmpnScreen::IsCmpFileShown() const
{
    return CmpFileDlg && CmpFileDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmdMsgDlg()
{
    if (!CmdMsgDlg)
    {
        return;
    }

    CmdMsgDlg->SetCmpnScreen(this);
    CmdMsgDlg->ShowMsgDlg();

    Mouse::Show(true);
}

void UCmpnScreen::HideCmdMsgDlg()
{
    if (CmdMsgDlg)
    {
        CmdMsgDlg->HideMsgDlg();
    }

    if (!bHidingAll && bIsShown && !IsCmpCompleteShown() && !IsCmpSceneShown())
    {
        ShowCmdDlg();
    }
}

bool UCmpnScreen::IsCmdMsgShown() const
{
    return CmdMsgDlg && CmdMsgDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpCompleteDlg()
{
    HideAll();

    if (CmpCompleteDlg)
    {
        CmpCompleteDlg->SetVisibility(ESlateVisibility::Visible);
        CmpCompleteDlg->SetIsEnabled(true);
        Mouse::Show(true);
    }
}

void UCmpnScreen::HideCmpCompleteDlg()
{
    if (CmpCompleteDlg)
    {
        CmpCompleteDlg->SetVisibility(ESlateVisibility::Hidden);
    }
}

bool UCmpnScreen::IsCmpCompleteShown() const
{
    return CmpCompleteDlg && CmpCompleteDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpSceneDlg()
{
    HideAll();

    if (CmpSceneDlg)
    {
        CmpSceneDlg->SetIsEnabled(true);
        CmpSceneDlg->Show();
        Mouse::Show(false);
    }
    else
    {
        ShowCmdDlg();
    }
}

void UCmpnScreen::HideCmpSceneDlg()
{
    if (CmpSceneDlg)
    {
        CmpSceneDlg->Hide();
    }
}

bool UCmpnScreen::IsCmpSceneShown() const
{
    return CmpSceneDlg && CmpSceneDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::ShowCmpLoadDlg()
{
    EnsureDialog(CmpLoadDlgClass, CmpLoadDlg, 95);

    HideAll();

    if (!CmpLoadDlg)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmpnScreen] ShowCmpLoadDlg: CmpLoadDlg is null"));
        return;
    }

    if (!ActiveCampaignName.IsEmpty())
    {
        CmpLoadDlg->SetCampaignName(ActiveCampaignName);
    }

    CmpLoadDlg->SetLoadingStage(TEXT("LOADING..."), 0.0f);
    CmpLoadDlg->SetVisibility(ESlateVisibility::Visible);
    CmpLoadDlg->SetIsEnabled(true);
    CmpLoadDlg->SetIsFocusable(true);
    CmpLoadDlg->SetDialogInputEnabled(true);
    CmpLoadDlg->Show();

    Mouse::Show(false);

    UE_LOG(LogTemp, Warning, TEXT("[CmpnScreen] ShowCmpLoadDlg"));
}

void UCmpnScreen::HideCmpLoadDlg()
{
    if (!CmpLoadDlg)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CmpnScreen] HideCmpLoadDlg: CmpLoadDlg is null"));
        return;
    }

    CmpLoadDlg->ClearManualLoadingStage();
    CmpLoadDlg->Hide();

    UE_LOG(LogTemp, Warning, TEXT("[CmpnScreen] HideCmpLoadDlg"));
}

bool UCmpnScreen::IsCmpLoadShown() const
{
    return CmpLoadDlg && CmpLoadDlg->GetVisibility() == ESlateVisibility::Visible;
}

void UCmpnScreen::SetFieldOfView(float InFOV)
{
    DesiredFieldOfView = InFOV;
    DefaultFallbackFOV = InFOV;
}

float UCmpnScreen::GetFieldOfView() const
{
    return DefaultFallbackFOV;
}

float UCmpnScreen::GetSceneDurationSeconds(const FString& SceneName) const
{
    if (SceneName.Equals(TEXT("01-News-Start"), ESearchCase::IgnoreCase))        return 95.0f;
    if (SceneName.Equals(TEXT("02-Coup-Failure"), ESearchCase::IgnoreCase))      return 75.0f;
    if (SceneName.Equals(TEXT("03-Blockade-Broken"), ESearchCase::IgnoreCase))   return 65.0f;
    if (SceneName.Equals(TEXT("04-Harmony-Risk"), ESearchCase::IgnoreCase))      return 50.0f;
    if (SceneName.Equals(TEXT("05-Foothill-Ridge"), ESearchCase::IgnoreCase))    return 60.0f;
    if (SceneName.Equals(TEXT("06-Renser-Buildup"), ESearchCase::IgnoreCase))    return 60.0f;
    if (SceneName.Equals(TEXT("07-Research-Lab"), ESearchCase::IgnoreCase))      return 75.0f;
    if (SceneName.Equals(TEXT("08-Renser-Accusation"), ESearchCase::IgnoreCase)) return 75.0f;
    if (SceneName.Equals(TEXT("09-Senate-Resolution"), ESearchCase::IgnoreCase)) return 75.0f;
    if (SceneName.Equals(TEXT("10-Renser-Arrival"), ESearchCase::IgnoreCase))    return 60.0f;
    if (SceneName.Equals(TEXT("11-Dantari-Pullback"), ESearchCase::IgnoreCase))  return 60.0f;
    if (SceneName.Equals(TEXT("12-Cease-Fire"), ESearchCase::IgnoreCase))        return 60.0f;
    if (SceneName.Equals(TEXT("13-Renser-Invasion"), ESearchCase::IgnoreCase))   return 60.0f;

    return 30.0f;
}

bool UCmpnScreen::TryStartSceneForEvent(CombatEvent* Event)
{
    if (CmpSceneDlg && CmpSceneDlg->IsSceneRunning())
    {
        return true;
    }

    if (bSceneTransitionActive)
    {
        return ContinuePendingSceneTransition();
    }

    return PrimeSceneTransitionForEvent(Event);
}

bool UCmpnScreen::PrimeSceneTransitionForEvent(CombatEvent* Event)
{
    if (!Event)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: Event is null"));
        return false;
    }

    EnsureDialog(CmpSceneDlgClass, CmpSceneDlg, 90);
    EnsureDialog(CmpLoadDlgClass, CmpLoadDlg, 95);

    if (!CmpSceneDlg)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: CmpSceneDlg is null"));
        return false;
    }

    if (!CmpLoadDlg)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: CmpLoadDlg is null"));
        return false;
    }

    if (!Event->SceneFile() || !*Event->SceneFile())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: SceneFile is empty"));
        return false;
    }

    const FString SceneName = UTF8_TO_TCHAR(Event->SceneFile());
    if (SceneName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: SceneName is empty"));
        return false;
    }

    const FS_CampaignMission* SceneMission = FindCampaignMissionByScene(SceneName);
    if (!SceneMission)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: No mission found for scene '%s'"),
            *SceneName);
        return false;
    }

    PendingSceneEvent = Event;
    PendingSceneMission = *SceneMission;
    bHasPendingSceneMission = true;

    ActiveSceneName = SceneName;
    ActiveSceneDurationSeconds = GetSceneDurationSeconds(SceneName);

    UE_LOG(LogTemp, Warning,
        TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: BEGIN Scene='%s' System='%s'"),
        *SceneName,
        *PendingSceneMission.MissionSystem);

    BeginSceneTransition();
    ShowCmpLoadDlg();

    if (CmpLoadDlg)
    {
        CmpLoadDlg->SetLoadingStage(TEXT("LOADING SYSTEM..."), 0.10f);
    }

    if (!StreamSceneSystemLevel(PendingSceneMission))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: FAILED TO STREAM Scene='%s' Mission='%s' System='%s'"),
            *SceneName,
            *PendingSceneMission.MissionName,
            *PendingSceneMission.MissionSystem);

        if (CmpLoadDlg)
        {
            CmpLoadDlg->SetLoadingStage(TEXT("FAILED TO LOAD SYSTEM"), 1.0f);
        }

        // DO NOT hide dialog or clear transition
        // Leave it visible for debugging

        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmpnScreen] PrimeSceneTransitionForEvent: STREAM STARTED for '%s'"),
        *SceneName);

    return true;
}
bool UCmpnScreen::ContinuePendingSceneTransition()
{
    if (!bSceneTransitionActive || !bHasPendingSceneMission)
    {
        return false;
    }

    if (CmpLoadDlg && CmpLoadDlg->GetVisibility() != ESlateVisibility::Visible)
    {
        ShowCmpLoadDlg();
    }

    if (!StreamSceneSystemLevel(PendingSceneMission))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] ContinuePendingSceneTransition: FAILED streaming Mission='%s' System='%s'"),
            *PendingSceneMission.MissionName,
            *PendingSceneMission.MissionSystem);

        if (CmpLoadDlg)
        {
            CmpLoadDlg->SetLoadingStage(TEXT("FAILED TO LOAD SYSTEM"), 1.0f);
        }

        return false;
    }

    if (!bSceneStreamingBlockedOnce)
    {
        if (CmpLoadDlg)
        {
            CmpLoadDlg->SetLoadingStage(TEXT("PREPARING TEXTURES..."), 0.60f);
        }

        if (UWorld* World = GetWorld())
        {
            World->FlushLevelStreaming(EFlushLevelStreamingType::Full);

            FStreamingManagerCollection& StreamingManager = FStreamingManagerCollection::Get();
            StreamingManager.BlockTillAllRequestsFinished(0.0f, true);
        }

        bSceneStreamingBlockedOnce = true;
        bSceneWarmupStarted = true;

        // Increased hold to suppress shader/material flash
        SceneWarmupReadyTime =
            UGameplayStatics::GetRealTimeSeconds(GetWorld()) + 4.0f;

        return true;
    }

    const float Now = UGameplayStatics::GetRealTimeSeconds(GetWorld());

    if (Now < SceneWarmupReadyTime)
    {
        if (CmpLoadDlg)
        {
            CmpLoadDlg->SetLoadingStage(TEXT("WARMING UP SCENE..."), 0.85f);
        }

        return true;
    }

    if (CmpLoadDlg)
    {
        CmpLoadDlg->SetLoadingStage(TEXT("FINALIZING SCENE..."), 0.95f);
    }

    if (ActiveSceneStreamingLevel)
    {
        ActiveSceneStreamingLevel->SetShouldBeLoaded(true);
        ActiveSceneStreamingLevel->SetShouldBeVisible(true);
    }

    if (UWorld* World = GetWorld())
    {
        World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
    }

    if (CampaignPtr)
    {
        CmpSceneDlg->SetCampaignNumber(CampaignPtr->GetCampaignId());
    }
    else
    {
        CmpSceneDlg->SetCampaignNumber(2);
    }

    CmpSceneDlg->LoadSceneFromMissionData(PendingSceneMission);

    HideCmpLoadDlg();
    ShowCmpSceneDlg();
    CmpSceneDlg->BeginSceneByName(ActiveSceneName, ActiveSceneDurationSeconds);

    if (PendingSceneEvent)
    {
        PendingSceneEvent->SetVisited(true);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[CmpnScreen] ContinuePendingSceneTransition: COMPLETE"));

    ClearPendingSceneTransition();
    return true;
}
void UCmpnScreen::ClearPendingSceneTransition()
{
    PendingSceneEvent = nullptr;
    PendingSceneMission = FS_CampaignMission();
    bHasPendingSceneMission = false;

    bSceneTransitionActive = false;
    bSceneStreamingBlockedOnce = false;
    bSceneWarmupStarted = false;

    SceneLoadScreenStartTime = 0.0f;
    SceneWarmupReadyTime = 0.0f;
    SceneReadyFrameCount = 0;

    ActiveSceneStreamingLevel = nullptr;
}

void UCmpnScreen::AdvanceCampaignScene()
{
    if (!CmpSceneDlg || !CmpSceneDlg->IsSceneRunning())
    {
        return;
    }

    const float NowSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    CmpSceneDlg->AdvanceSceneFromTimer(NowSeconds);
}

bool UCmpnScreen::StreamSceneSystemLevel(const FS_CampaignMission& SceneMission)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] StreamSceneSystemLevel: World is null"));
        return false;
    }

    const FString SystemName = SceneMission.MissionSystem.TrimStartAndEnd();
    if (SystemName.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] StreamSceneSystemLevel: MissionSystem is empty for Scene='%s' Mission='%s'"),
            *SceneMission.Scene,
            *SceneMission.MissionName);
        return false;
    }

    const FString PackagePath = FString::Printf(TEXT("/Game/Maps/%s"), *SystemName);

    FString ResolvedPackageFilename;
    if (!FPackageName::DoesPackageExist(PackagePath, &ResolvedPackageFilename))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] StreamSceneSystemLevel: Map package does not exist: %s (Scene='%s' Mission='%s')"),
            *PackagePath,
            *SceneMission.Scene,
            *SceneMission.MissionName);
        return false;
    }

    if (ActiveSceneStreamingLevel)
    {
        ActiveSceneStreamingLevel->SetShouldBeLoaded(true);
        ActiveSceneStreamingLevel->SetShouldBeVisible(false);

        UE_LOG(LogTemp, Log,
            TEXT("[CmpnScreen] StreamSceneSystemLevel: reusing existing streaming level for %s"),
            *PackagePath);

        return true;
    }

    bool bSuccess = false;

    ActiveSceneStreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(
        World,
        PackagePath,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        bSuccess);

    if (!bSuccess || !ActiveSceneStreamingLevel)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CmpnScreen] StreamSceneSystemLevel: failed to dynamically stream %s"),
            *PackagePath);

        ActiveSceneStreamingLevel = nullptr;
        return false;
    }

    ActiveSceneStreamingLevel->SetShouldBeLoaded(true);
    ActiveSceneStreamingLevel->SetShouldBeVisible(false);

    UE_LOG(LogTemp, Warning,
        TEXT("[CmpnScreen] StreamSceneSystemLevel: started hidden stream for System='%s' Package='%s'"),
        *SystemName,
        *PackagePath);

    return true;
}

bool UCmpnScreen::IsSceneVisualReady() const
{
    if (!ActiveSceneStreamingLevel)
    {
        return false;
    }

    return ActiveSceneStreamingLevel->IsLevelLoaded() &&
           !ActiveSceneStreamingLevel->IsStreamingStatePending();
}

void UCmpnScreen::BeginSceneTransition()
{
    bSceneTransitionActive = true;
    bSceneStreamingBlockedOnce = false;
    bSceneWarmupStarted = false;

    SceneLoadScreenStartTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
    SceneWarmupReadyTime = 0.0f;
    SceneReadyFrameCount = 0;
}

bool UCmpnScreen::AreShadersReadyForReveal() const
{
    return FShaderPipelineCache::NumPrecompilesRemaining() == 0;
}

bool UCmpnScreen::CanRevealSceneNow() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const float Now = UGameplayStatics::GetRealTimeSeconds(World);

    const bool bMinTimeSatisfied =
        (Now - SceneLoadScreenStartTime) >= SceneMinLoadScreenSeconds;

    const bool bLevelReady = IsSceneVisualReady();
    const bool bWarmupSatisfied =
        bSceneWarmupStarted && (Now >= SceneWarmupReadyTime);
    const bool bShadersReady = AreShadersReadyForReveal();

    return bMinTimeSatisfied &&
           bLevelReady &&
           bWarmupSatisfied &&
           bShadersReady &&
           (SceneReadyFrameCount >= SceneRequiredReadyFrames);
}

void UCmpnScreen::SetActiveCampaignName(const FString& InName)
{
    ActiveCampaignName = InName;
    UE_LOG(LogTemp, Log, TEXT("[CmpnScreen] ActiveCampaignName set to: %s"), *ActiveCampaignName);
}