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
    CompletionStage = 0;
    DesiredFieldOfView = GetFieldOfView();
    bCampaignPaused = false;

    bool bCutscene = false;

    if (CampaignPtr && CampaignPtr->IsActive() && !CampaignPtr->GetEvents().isEmpty())
    {
        ListIter<CombatEvent> Iter = CampaignPtr->GetEvents();
        while (++Iter)
        {
            CombatEvent* Event = Iter.value();

            if (Event && !Event->Visited() && Event->SceneFile() && *Event->SceneFile())
            {
                //Stars->ExecCutscene(Event->SceneFile(), CampaignPtr->Path());

                //if (Stars->InCutscene())
                //{
                //    bCutscene = true;
                //    ShowCmpSceneDlg();
                //}

                Event->SetVisited(true);
                break;
            }
        }
    }

    if (!bCutscene)
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

    Mouse::SetCursor(Mouse::ARROW);

    if (TimeTilChange > 0.0)
    {
        TimeTilChange -= DeltaTime;
        if (TimeTilChange < 0.0)
        {
            TimeTilChange = 0.0;
        }
    }

    const bool bInCutscene = Stars->InCutscene();
    bExitLatch = Keyboard::KeyDown(KEY_EXIT) ? true : false;

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

    if (bInCutscene && bExitLatch)
    {
        TimeTilChange = 1.0;
        Stars->EndCutscene();
        Stars->EndMission();
        SetFieldOfView(DesiredFieldOfView);
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
                bool bCutscene = false;
                CombatEvent* Event = CampaignPtr->GetLastEvent();

                if (Event && !Event->Visited() && Event->SceneFile() && *Event->SceneFile())
                {
                    Stars->ExecCutscene(Event->SceneFile(), CampaignPtr->Path());

                    if (Stars->InCutscene())
                    {
                        bCutscene = true;
                        ShowCmpSceneDlg();
                    }
                }

                if (!bCutscene)
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

// +-------------------------------------------------------------------+

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

// +-------------------------------------------------------------------+

void UCmpnScreen::HideCmdMsgDlg()
{
    if (CmdMsgDlg)
    {
        CmdMsgDlg->HideMsgDlg();
    }

    // Restore the command hub only when this is a real close,
    // not when HideAll() is sweeping the screen.
    if (!bHidingAll && bIsShown && !IsCmpCompleteShown() && !IsCmpSceneShown())
    {
        ShowCmdDlg();
    }
}

// +-------------------------------------------------------------------+

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
        CmpSceneDlg->SetVisibility(ESlateVisibility::Visible);
        CmpSceneDlg->SetIsEnabled(true);
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
        CmpSceneDlg->SetVisibility(ESlateVisibility::Hidden);
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