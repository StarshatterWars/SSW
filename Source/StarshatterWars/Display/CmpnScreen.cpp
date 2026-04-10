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

// Dialogs:
#include "CmdDlg.h"
#include "CmdMsgDlg.h"
#include "CmpFileDlg.h"
#include "CmpCompleteDlg.h"
#include "CampaignSceneDlg.h"
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

#include "Engine/DataTable.h"
#include "StarshatterGameDataSubsystem.h"

#include "GameStructs.h"

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
        ExecFrame((double)InDeltaTime);
    }

    // ------------------------------------------------------------
    // Force timer-driven campaign scene advancement even if
    // the normal ExecFrame flow is not progressing as expected.
    // ------------------------------------------------------------
    /*if (CmpSceneDlg && CmpSceneDlg->IsSceneRunning())
    {
        const float NowSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());

        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] NativeTick driving scene now=%.2f"),
            NowSeconds);

        CmpSceneDlg->AdvanceSceneFromTimer(NowSeconds);
    }*/
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
        UWorld* World = GetWorld();
        if (World)
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

    ApplyManagerToChildren();

    HideAll();

    CompletionStage = 0;
    TimeTilChange = 0.0;
    bExitLatch = false;
    bShowMissionsRequested = false;
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

    bSetupComplete = false;
    bIsShown = false;
    bShowMissionsRequested = false;
    bExitLatch = false;
    TimeTilChange = 0.0;
    CompletionStage = 0;
    ActiveSceneName.Empty();
    ActiveSceneDurationSeconds = 0.0f;
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

    bool bStartedScene = false;

    if (CampaignPtr && CampaignPtr->IsActive() && !CampaignPtr->GetEvents().isEmpty())
    {
        ListIter<CombatEvent> Iter = CampaignPtr->GetEvents();
        while (++Iter)
        {
            CombatEvent* Event = Iter.value();

            if (Event && !Event->Visited() && Event->SceneFile() && *Event->SceneFile())
            {
                bStartedScene = TryStartSceneForEvent(Event);
                if (bStartedScene)
                {
                    break;
                }
            }
        }
    }

    if (!bStartedScene)
    {
        ShowCmdDlg();
    }
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
        const float WarpFactor = PlayerShip->WarpFactor();

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
            Stars->SetGameMode(EGameMode::MENU);
        }
    }
    else if (Stars->GetGameMode() == EGameMode::CMPN)
    {
        if (TimeTilChange <= 0.0)
        {
            if (Keyboard::KeyDown(KEY_PAUSE))
            {
                TimeTilChange = 1.0;
                bCampaignPaused = !bCampaignPaused;
                Stars->Pause(bCampaignPaused);
            }

            else if (Keyboard::KeyDown(KEY_TIME_COMPRESS))
            {
                TimeTilChange = 1.0;

                switch (Stars->TimeCompression())
                {
                case 1:  Stars->SetTimeCompression(2); break;
                case 2:  Stars->SetTimeCompression(4); break;
                case 4:  Stars->SetTimeCompression(8); break;
                }
            }

            else if (Keyboard::KeyDown(KEY_TIME_EXPAND))
            {
                TimeTilChange = 1.0;

                switch (Stars->TimeCompression())
                {
                case 8:  Stars->SetTimeCompression(4); break;
                case 4:  Stars->SetTimeCompression(2); break;
                default: Stars->SetTimeCompression(1); break;
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

            if (CampaignPtr->IsTraining())
            {
                List<Campaign>& CampaignList = Campaign::GetAllCampaigns();
                Campaign* NextCampaign = CampaignList[1];

                if (NextCampaign)
                {
                    NextCampaign->Load();
                    Campaign::SelectCampaign(NextCampaign->GetName());
                    Stars->SetGameMode(EGameMode::CLOD);
                    return;
                }
            }

            if (CampaignPtr->GetCampaignId() < Campaign::GetLastCampaignId())
            {
                Stars->StartOrResumeGame();
            }
            else
            {
                Mouse::Show(false);
                MusicManager::SetMode(MusicMode::MENU);
                Stars->SetGameMode(EGameMode::MENU);
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

void UCmpnScreen::SetFieldOfView(float InFOV)
{
    DesiredFieldOfView = InFOV;

    if (CmpSceneDlg)
    {
        //CmpSceneDlg->SetFieldOfView(InFOV);
    }
    else
    {
        DefaultFallbackFOV = InFOV;
    }
}

float UCmpnScreen::GetFieldOfView() const
{
    if (CmpSceneDlg)
    {
        //return CmpSceneDlg->GetFieldOfView();
    }

    return DefaultFallbackFOV;
}

float UCmpnScreen::GetSceneDurationSeconds(const FString& SceneName) const
{
    if (SceneName.Equals(TEXT("01-News-Start"), ESearchCase::IgnoreCase))      return 95.0f;
    if (SceneName.Equals(TEXT("02-Coup-Failure"), ESearchCase::IgnoreCase))    return 75.0f;
    if (SceneName.Equals(TEXT("03-Blockade-Broken"), ESearchCase::IgnoreCase)) return 65.0f;
    if (SceneName.Equals(TEXT("04-Harmony-Risk"), ESearchCase::IgnoreCase))    return 50.0f;
    if (SceneName.Equals(TEXT("05-Foothill-Ridge"), ESearchCase::IgnoreCase))  return 60.0f;
    if (SceneName.Equals(TEXT("06-Renser-Buildup"), ESearchCase::IgnoreCase))  return 60.0f;
    if (SceneName.Equals(TEXT("07-Research-Lab"), ESearchCase::IgnoreCase))    return 75.0f;
    if (SceneName.Equals(TEXT("08-Renser-Accusation"), ESearchCase::IgnoreCase)) return 75.0f;
    if (SceneName.Equals(TEXT("09-Senate-Resolution"), ESearchCase::IgnoreCase)) return 75.0f;
    if (SceneName.Equals(TEXT("10-Renser-Arrival"), ESearchCase::IgnoreCase))  return 60.0f;
    if (SceneName.Equals(TEXT("11-Dantari-Pullback"), ESearchCase::IgnoreCase)) return 60.0f;
    if (SceneName.Equals(TEXT("12-Cease-Fire"), ESearchCase::IgnoreCase))      return 60.0f;
    if (SceneName.Equals(TEXT("13-Renser-Invasion"), ESearchCase::IgnoreCase)) return 60.0f;

    return 30.0f;
}

bool UCmpnScreen::TryStartSceneForEvent(CombatEvent* Event)
{
    if (!Event || !CmpSceneDlg)
    {
        return false;
    }

    if (!Event->SceneFile() || !*Event->SceneFile())
    {
        return false;
    }

    const FString SceneName = UTF8_TO_TCHAR(Event->SceneFile());
    if (SceneName.IsEmpty())
    {
        return false;
    }

    ActiveSceneName = SceneName;
    ActiveSceneDurationSeconds = GetSceneDurationSeconds(SceneName);

    const FS_CampaignMission* SceneMission = FindCampaignMissionByScene(ActiveSceneName);
    if (SceneMission)
    {
        CmpSceneDlg->LoadSceneFromMissionData(*SceneMission);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CmpnScreen] TryStartSceneForEvent: No mission row found for scene %s"),
            *ActiveSceneName);
    }

    ShowCmpSceneDlg();
    CmpSceneDlg->BeginSceneByName(ActiveSceneName, ActiveSceneDurationSeconds);

    Event->SetVisited(true);

    UE_LOG(LogTemp, Log,
        TEXT("[CmpnScreen] Started campaign scene: %s (%.2fs)"),
        *ActiveSceneName,
        ActiveSceneDurationSeconds);

    return true;
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

    // ------------------------------------------------------------
    // Resolve campaign source (selected first, fallback to active)
    // ------------------------------------------------------------
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

    // ------------------------------------------------------------
    // Search missions
    // ------------------------------------------------------------
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