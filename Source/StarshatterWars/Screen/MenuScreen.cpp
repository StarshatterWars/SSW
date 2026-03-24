#include "MenuScreen.h"

// UE:
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// Dialogs:
#include "MenuDlg.h"
#include "ExitDlg.h"
#include "ConfirmDlg.h"
#include "FirstTimeDlg.h"
#include "PlayerDlg.h"
#include "AwardShowDlg.h"
#include "MissionSelectDlg.h"
#include "CmdMissionsDlg.h"
#include "CampaignSelectDlg.h"
#include "MissionEditorDlg.h"
#include "MissionElementDlg.h"
#include "MissionEventDlg.h"
#include "MissionEditorNavDlg.h"
#include "OptionsScreen.h"
#include "LoadDlg.h"
#include "CmdDlg.h"
#include "TacRefDlg.h"
#include "StarshatterPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameStructs.h"

#include "StarshatterAssetRegistrySubsystem.h"

// ------------------------------------------------------------

void UMenuScreen::Initialize(UGameInstance* InGI)
{
    if (!InGI) return;

    UStarshatterAssetRegistrySubsystem* Assets = InGI->GetSubsystem<UStarshatterAssetRegistrySubsystem>();
    if (!Assets) return;

    // Only assign if currently unset (preserve BP defaults):
    if (!OptionsScreenClass)
        OptionsScreenClass = Assets->GetWidgetClass(TEXT("UI.OptionsScreenClass"), true);

    if (!CampaignSelectScreenClass)
        CampaignSelectScreenClass = Assets->GetWidgetClass(TEXT("UI.CampaignSelectScreenClass"), true);

    if (!MissionSelectScreenClass)
        MissionSelectScreenClass = Assets->GetWidgetClass(TEXT("UI.MissionSelectScreenClass"), true);

    if (!OperationsScreenClass)
        OperationsScreenClass = Assets->GetWidgetClass(TEXT("UI.OperationscreenClass"), true);

    if (!FirstTimeDlgClass)
        FirstTimeDlgClass = Assets->GetWidgetClass(TEXT("UI.FirstTimeDlgClass"), true);

    if (!ExitDlgClass)
        ExitDlgClass = Assets->GetWidgetClass(TEXT("UI.ExitDlgClass"), true);

    if (!PlayerDlgClass)
        PlayerDlgClass = Assets->GetWidgetClass(TEXT("UI.PlayerLogbookScreenClass"), true);

    if (!TacRefDlgClass)
        TacRefDlgClass = Assets->GetWidgetClass(TEXT("UI.TacRefScreenClass"), true);
    // IMPORTANT: do NOT touch MenuDlgClass unless you actually bind it in registry:
    // if (!MenuDlgClass)
    //     MenuDlgClass = Assets->GetWidgetClass(TEXT("UI.MenuDlgClass"), true);

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] Initialize: MenuDlgClass=%s Options=%s FirstTime=%s Exit=%s"),
        *GetNameSafe(MenuDlgClass.Get()),
        *GetNameSafe(OptionsScreenClass.Get()),
        *GetNameSafe(FirstTimeDlgClass.Get()),
        *GetNameSafe(ExitDlgClass.Get()));
}

static void ApplyUIFocus(APlayerController* PC, UUserWidget* FocusWidget)
{
    if (!PC || !FocusWidget)
        return;

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    // Use GameAndUI for reliability (especially with Enhanced Input / legacy code)
    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(FocusWidget->TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);

    PC->SetInputMode(Mode);
}

UMenuScreen::UMenuScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UMenuScreen::NativeConstruct()
{
    Super::NativeConstruct();

    if (UGameInstance* GI = GetGameInstance())
    {
        Initialize(GI);
    }
}

// ------------------------------------------------------------
// Dialog creation (GC-safe via UPROPERTY storage)
// ------------------------------------------------------------

template<typename TDialog>
TDialog* UMenuScreen::EnsureDialog(TSubclassOf<TDialog> ClassToSpawn, TObjectPtr<TDialog>& Storage)
{
    if (Storage)
        return Storage.Get();

    if (!ClassToSpawn)
        return nullptr;

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureDialog: OwningPlayer is null (%s)"), *GetName());
        return nullptr;
    }

    TDialog* Created = CreateWidget<TDialog>(PC, ClassToSpawn);
    if (!Created)
        return nullptr;

    // Router assignment (BaseScreen owns MenuManager)
    Created->SetMenuManager(this);

    // Park hidden; real Z-order is applied by ShowDialog(...)
    Created->AddToViewport(10);
    Created->SetVisibility(ESlateVisibility::Hidden);
    Created->SetIsEnabled(false);

    Storage = Created;
    return Created;
}

// ------------------------------------------------------------
// Show/Hide helpers
// ------------------------------------------------------------

void UMenuScreen::ShowDialog(UBaseScreen* Dialog, int32 ZOrder)
{
    if (!Dialog) {
        return;
    }

    if (Dialog->IsInViewport())
        Dialog->RemoveFromParent();

    Dialog->AddToViewport(ZOrder);
    Dialog->SetVisibility(ESlateVisibility::Visible);
    Dialog->SetDialogInputEnabled(true);

    CurrentDialog = Dialog;

    // Always restore UI input to the visible dialog
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableMouseOverEvents = true;

        FInputModeGameAndUI Mode;
        Mode.SetWidgetToFocus(Dialog->TakeWidget());
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        PC->SetInputMode(Mode);
    }
}

void UMenuScreen::HideDialog(UBaseScreen* Dialog)
{
    if (!Dialog)
        return;

    Dialog->SetDialogInputEnabled(false);
    Dialog->SetVisibility(ESlateVisibility::Collapsed);


    if (CurrentDialog == Dialog)
        CurrentDialog = nullptr;
}

void UMenuScreen::HideAll()
{
    HideDialog(MenuDlg);
    HideDialog(ExitDlg);
    HideDialog(ConfirmDlg);

    HideDialog(FirstTimeDlg);
    HideDialog(PlayerDlg);
    HideDialog(AwardDlg);

    HideDialog(MissionSelectDlg);
    HideDialog(CmdMissionsDlg);
    HideDialog(CmpSelectDlg);
    HideDialog(CmdDlg);

    HideDialog(MsnEditDlg);
    HideDialog(MsnElemDlg);
    HideDialog(MsnEventDlg);
    HideDialog(MsnEditNavDlg);

    HideDialog(LoadDlg);
    HideDialog(TacRefDlg);

    HideDialog(OptionsScreen);

    CurrentDialog = nullptr;
}

// ------------------------------------------------------------
// Setup / TearDown
// ------------------------------------------------------------

void UMenuScreen::Setup()
{
    EnsureDialog<UMenuDlg>(MenuDlgClass, MenuDlg);
    EnsureDialog<UExitDlg>(ExitDlgClass, ExitDlg);

    EnsureDialog<UOptionsScreen>(OptionsScreenClass, OptionsScreen);
    EnsureDialog<UConfirmDlg>(ConfirmDlgClass, ConfirmDlg);

    EnsureDialog<UFirstTimeDlg>(FirstTimeDlgClass, FirstTimeDlg);
    EnsureDialog<UPlayerDlg>(PlayerDlgClass, PlayerDlg);
    EnsureDialog<UAwardShowDlg>(AwardDlgClass, AwardDlg);

    EnsureDialog<UMissionSelectDlg>(MsnSelectDlgClass, MissionSelectDlg);
    EnsureDialog<UCampaignSelectDlg>(CmpSelectDlgClass, CmpSelectDlg);
    EnsureDialog<UCmdMissionsDlg>(CmdMissionsDlgClass, CmdMissionsDlg);
    EnsureDialog<UCmdDlg>(CmdDlgClass, CmdDlg);

    EnsureDialog<UMissionEditorDlg>(MsnEditDlgClass, MsnEditDlg);
    EnsureDialog<UMissionElementDlg>(MsnElemDlgClass, MsnElemDlg);
    EnsureDialog<UMissionEventDlg>(MsnEventDlgClass, MsnEventDlg);
    EnsureDialog<UMissionEditorNavDlg>(MsnEditNavDlgClass, MsnEditNavDlg);

    EnsureDialog<ULoadDlg>(LoadDlgClass, LoadDlg);
    EnsureDialog<UTacRefDlg>(TacRefDlgClass, TacRefDlg);

    ShowMenuDlg();
}

void UMenuScreen::TearDown()
{
    HideAll();

    // If you truly want to destroy them, RemoveFromParent and null them.
    // Usually unnecessary; a menu screen lifetime is short.
    auto Destroy = [](UBaseScreen*& W)
        {
            if (W)
            {
                if (W->IsInViewport())
                    W->RemoveFromParent();
                W = nullptr;
            }
        };

    Destroy(reinterpret_cast<UBaseScreen*&>(MenuDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(ExitDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(ConfirmDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(FirstTimeDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(PlayerDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(AwardDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(MissionSelectDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmpSelectDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmdMissionsDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(CmdDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEditDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnElemDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEventDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(MsnEditNavDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(LoadDlg));
    Destroy(reinterpret_cast<UBaseScreen*&>(TacRefDlg));

    Destroy(reinterpret_cast<UBaseScreen*&>(OptionsScreen));

    CurrentDialog = nullptr;
    bIsShown = false;
}

// ------------------------------------------------------------

void UMenuScreen::ExecFrame(double DeltaTime)
{
    (void)DeltaTime;
}

// ------------------------------------------------------------
// CloseTopmost
// ------------------------------------------------------------

bool UMenuScreen::CloseTopmost()
{
    // Mission editor overlays (top-most first)
    if (MsnElemDlg && MsnElemDlg->GetVisibility() == ESlateVisibility::Visible) { HideMsnElemDlg(); return true; }
    if (MsnEventDlg && MsnEventDlg->GetVisibility() == ESlateVisibility::Visible) { HideMsnEventDlg(); return true; }

    // Options hub
    if (OptionsScreen && OptionsScreen->GetVisibility() == ESlateVisibility::Visible)
    {
        ReturnFromOptions();
        return true;
    }

    if (CmpSelectDlg && CmpSelectDlg->GetVisibility() == ESlateVisibility::Visible)
    {
        ShowMenuDlg();
        return true;
    }

    if (MenuDlg && MenuDlg->GetVisibility() != ESlateVisibility::Visible)
    {
        ShowMenuDlg();
        return true;
    }

    return false;
}

// ------------------------------------------------------------

/* --------------------------------------------------------------------
   Show
   -------------------------------------------------------------------- */

void UMenuScreen::Show()
{
   UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] UMenuScreen::Show()"));
   if (bIsShown)
        return;

    bool bHasSaveNow = false;

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterPlayerSubsystem* PlayerSS =
            GI->GetSubsystem<UStarshatterPlayerSubsystem>())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] UStarshatterPlayerSubsystem"));
           
           if (!PlayerSS->HasLoaded())
            {
                PlayerSS->LoadFromBoot();
            }

            bHasSaveNow = PlayerSS->DoesSaveExistNow();

            UE_LOG(LogTemp, Warning,
                TEXT("[MenuScreen] SaveExistsNow=%d"),
                bHasSaveNow ? 1 : 0);
        }
    }

    if (bHasSaveNow)
    {
        ShowMenuDlg();
    }
    else
    {
        ShowFirstTimeDlg();
    }

    bIsShown = true;
}

void UMenuScreen::Hide()
{
    if (!bIsShown)
        return;

    HideAll();
    bIsShown = false;
}

// ------------------------------------------------------------
// Show* routing
// ------------------------------------------------------------

void UMenuScreen::ShowMenuDlg()
{
    // Optional: single-dialog policy
    HideAll();

    if (!MenuDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: MenuDlgClass is NULL (set it in WBP_MenuScreen defaults)"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: OwningPlayer is NULL"));
        return;
    }

    // Lazy create once
    if (!MenuDlg)
    {
        MenuDlg = CreateWidget<UMenuDlg>(PC, MenuDlgClass);
        if (!MenuDlg)
        {
            UE_LOG(LogTemp, Error, TEXT("UMenuScreen::ShowMenuDlg: Failed to create MenuDlg"));
            return;
        }

        MenuDlg->Manager = this;
        MenuDlg->AddToViewport((int32) EMenuZOrder::Z_MENU_BASE);                    // added once; Z handled below
        MenuDlg->SetDialogInputEnabled(false);
        MenuDlg->SetVisibility(ESlateVisibility::Hidden);
    }

    // Always re-assert manager in case of re-instancing
    MenuDlg->Manager = this;

    // Bring to front (no SetZOrderInViewport dependency)
    ++ZCounter;
    if (MenuDlg->IsInViewport())
        MenuDlg->RemoveFromParent();
    MenuDlg->AddToViewport((int32)EMenuZOrder::Z_MENU_BASE);

    MenuDlg->SetIsEnabled(true);
    MenuDlg->SetIsFocusable(true);
    MenuDlg->SetVisibility(ESlateVisibility::Visible);
    MenuDlg->SetDialogInputEnabled(true);

    CurrentDialog = MenuDlg;
}


void UMenuScreen::ShowCampaignSelectDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowCampaignSelectDlg: BEGIN"));

    if (!CmpSelectDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCampaignSelectDlg: CmpSelectDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCampaignSelectDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UCampaignSelectDlg>(CmpSelectDlgClass, CmpSelectDlg);
    if (!CmpSelectDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowCampaignSelectDlg: EnsureDialog failed (CmpSelectDlg is NULL)"));
        return;
    }

    HideAll();

    CmpSelectDlg->SetMenuManager(this);
    CmpSelectDlg->InitializeDlg(this);

    if (CmpSelectDlg->IsInViewport())
    {
        CmpSelectDlg->RemoveFromParent();
    }

    CmpSelectDlg->AddToViewport(200);

    CmpSelectDlg->SetVisibility(ESlateVisibility::Visible);
    CmpSelectDlg->SetIsEnabled(true);
    CmpSelectDlg->SetIsFocusable(true);
    CmpSelectDlg->SetDialogInputEnabled(true);

    CurrentDialog = CmpSelectDlg;

    ApplyUIFocus(PC, CmpSelectDlg);

    CmpSelectDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowCampaignSelectDlg: SHOWN InViewport=%d Vis=%d"),
        CmpSelectDlg->IsInViewport() ? 1 : 0,
        (int32)CmpSelectDlg->GetVisibility());
}

void UMenuScreen::ShowOperationsDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowOperationsDlg: BEGIN"));

    if (!CmdDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOperationsDlg: CmdMissionsDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOperationsDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UCmdDlg>(CmdDlgClass, CmdDlg);
    if (!CmdDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOperationsDlg: EnsureDialog failed (CmdDlg is NULL)"));
        return;
    }

    HideAll();

    CmdDlg->SetMenuManager(this);
    CmdDlg->InitializeDlg(this);

    if (CmdDlg->IsInViewport())
    {
        CmdDlg->RemoveFromParent();
    }

    CmdDlg->AddToViewport(200);

    CmdDlg->SetVisibility(ESlateVisibility::Visible);
    CmdDlg->SetIsEnabled(true);
    CmdDlg->SetIsFocusable(true);
    CmdDlg->SetDialogInputEnabled(true);

    CurrentDialog = CmdDlg;

    ApplyUIFocus(PC, CmdDlg);

    CmdDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowCmdDlg: SHOWN InViewport=%d Vis=%d"),
        CmdDlg->IsInViewport() ? 1 : 0,
        (int32)CmdDlg->GetVisibility());
}

void UMenuScreen::ShowMissionSelectDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionSelectDlg: BEGIN"));

    if (!MsnSelectDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionSelectDlg: MsnSelectDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionSelectDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UMissionSelectDlg>(MsnSelectDlgClass, MissionSelectDlg);
    if (!MissionSelectDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowMissionSelectDlg: EnsureDialog failed (MissionSelectDlg is NULL)"));
        return;
    }

    HideAll();

    MissionSelectDlg->SetMenuManager(this);
    MissionSelectDlg->InitializeDlg(this);

    if (MissionSelectDlg->IsInViewport())
    {
        MissionSelectDlg->RemoveFromParent();
    }

    MissionSelectDlg->AddToViewport(200);

    MissionSelectDlg->SetVisibility(ESlateVisibility::Visible);
    MissionSelectDlg->SetIsEnabled(true);
    MissionSelectDlg->SetIsFocusable(true);
    MissionSelectDlg->SetDialogInputEnabled(true);

    CurrentDialog = MissionSelectDlg;

    ApplyUIFocus(PC, MissionSelectDlg);

    MissionSelectDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowMissionSelectDlg: SHOWN InViewport=%d Vis=%d"),
        MissionSelectDlg->IsInViewport() ? 1 : 0,
        (int32)MissionSelectDlg->GetVisibility());
}

void UMenuScreen::ShowMissionEditorDlg()
{
    HideAll();
    EnsureDialog<UMissionEditorDlg>(MsnEditDlgClass, MsnEditDlg);
    ShowDialog(MsnEditDlg, true);
}

void UMenuScreen::ShowMsnElemDlg()
{
    EnsureDialog<UMissionElementDlg>(MsnElemDlgClass, MsnElemDlg);
    if (!MsnElemDlg) return;

    // Keep editor + nav visible behind it (non-topmost)
    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, false);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, false);

    ShowDialog(MsnElemDlg, true);
}

void UMenuScreen::HideMsnElemDlg()
{
    HideDialog(MsnElemDlg);

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, true);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, true);
}

void UMenuScreen::ShowMissionEventDlg()
{
    EnsureDialog<UMissionEventDlg>(MsnEventDlgClass, MsnEventDlg);
    if (!MsnEventDlg) return;

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, false);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, false);

    ShowDialog(MsnEventDlg, true);
}

void UMenuScreen::HideMsnEventDlg()
{
    HideDialog(MsnEventDlg);

    if (MsnEditDlg && MsnEditDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditDlg, true);

    if (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible)
        ShowDialog(MsnEditNavDlg, true);
}

void UMenuScreen::ShowNavDlg()
{
    EnsureDialog<UMissionEditorNavDlg>(MsnEditNavDlgClass, MsnEditNavDlg);
    if (!MsnEditNavDlg) return;

    if (MsnEditNavDlg->GetVisibility() != ESlateVisibility::Visible)
    {
        HideAll();
        ShowDialog(MsnEditNavDlg, true);
    }
}

void UMenuScreen::HideNavDlg()
{
    HideDialog(MsnEditNavDlg);
}

bool UMenuScreen::IsNavShown() const
{
    return (MsnEditNavDlg && MsnEditNavDlg->GetVisibility() == ESlateVisibility::Visible);
}

void UMenuScreen::ShowFirstTimeDlg()
{
    // Ensure Menu exists
    if (!MenuDlg)
        ShowMenuDlg();

    // Ensure FirstTimeDlg exists
    if (!FirstTimeDlg && FirstTimeDlgClass)
    {
        FirstTimeDlg = CreateWidget<UFirstTimeDlg>(GetOwningPlayer(), FirstTimeDlgClass);
        if (!FirstTimeDlg)
            return;

        FirstTimeDlg->SetMenuManager(this);
        FirstTimeDlg->AddToViewport((int32)EMenuZOrder::Z_MODAL);
    }

    // ----------------------------------------------------
    // Z-ORDER + INPUT CONTROL
    // ----------------------------------------------------

    // Menu stays visible but cannot receive input
    ShowDialog(MenuDlg, (int32)EMenuZOrder::Z_MENU_BASE);
    MenuDlg->SetDialogInputEnabled(false);

    // FirstTimeDlg is modal and interactive
    ShowDialog(FirstTimeDlg, (int32)EMenuZOrder::Z_MODAL);
    FirstTimeDlg->SetDialogInputEnabled(true);

    CurrentDialog = FirstTimeDlg;
}

void UMenuScreen::ShowPlayerDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowPlayerDlg: BEGIN"));

    if (!PlayerDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: PlayerDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UPlayerDlg>(PlayerDlgClass, PlayerDlg);
    if (!PlayerDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowPlayerDlg: EnsureDialog failed (PlayerDlg is NULL)"));
        return;
    }

    HideAll();

    PlayerDlg->SetMenuManager(this);
    PlayerDlg->InitializeDlg(this);

    if (PlayerDlg->IsInViewport())
        PlayerDlg->RemoveFromParent();

 
    PlayerDlg->AddToViewport(200);

    PlayerDlg->SetVisibility(ESlateVisibility::Visible);
    PlayerDlg->SetIsEnabled(true);
    PlayerDlg->SetIsFocusable(true);
    PlayerDlg->SetDialogInputEnabled(true);

    CurrentDialog = PlayerDlg;

    ApplyUIFocus(PC, PlayerDlg);

    // TEMP: comment this out if you still see "nothing shows"
    // PlayerDlg->ShowDlg();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowPlayerDlg: SHOWN InViewport=%d Vis=%d"),
        PlayerDlg->IsInViewport() ? 1 : 0,
        (int32)PlayerDlg->GetVisibility());
}

void UMenuScreen::ShowTacRefDlg()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowTacRefDlg: BEGIN"));

    if (!TacRefDlgClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: TacRefDlgClass is NULL"));
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UTacRefDlg>(TacRefDlgClass, TacRefDlg);
    if (!TacRefDlg)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowTacRefDlg: EnsureDialog failed (TacRefDlg is NULL)"));
        return;
    }

    HideAll();

    TacRefDlg->SetMenuManager(this);
    TacRefDlg->InitializeDlg(this);

    if (TacRefDlg->IsInViewport())
        TacRefDlg->RemoveFromParent();

    // Pick a Z-order consistent with your other screens:
    TacRefDlg->AddToViewport(200);

    TacRefDlg->SetVisibility(ESlateVisibility::Visible);
    TacRefDlg->SetIsEnabled(true);
    TacRefDlg->SetIsFocusable(true);
    TacRefDlg->SetDialogInputEnabled(true);

    CurrentDialog = TacRefDlg;

    ApplyUIFocus(PC, TacRefDlg);

    // If your dialogs require a manual "ShowDlg" or "Show" call:
    TacRefDlg->Show();

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowTacRefDlg: SHOWN InViewport=%d Vis=%d"),
        TacRefDlg->IsInViewport() ? 1 : 0,
        (int32)TacRefDlg->GetVisibility());
}

void UMenuScreen::ShowAwardDlg()
{
    HideAll();
    EnsureDialog<UAwardShowDlg>(AwardDlgClass, AwardDlg);
    ShowDialog(AwardDlg, true);
}

void UMenuScreen::ShowExitDlg()
{
    EnsureDialog<UMenuDlg>(MenuDlgClass, MenuDlg);
    EnsureDialog<UExitDlg>(ExitDlgClass, ExitDlg);

    if (!MenuDlg || !ExitDlg) return;

    MenuDlg->SetMenuManager(this);
    ExitDlg->SetMenuManager(this);

    // Menu behind, inert:
    MenuDlg->SetDialogInputEnabled(false);
    ShowDialog(MenuDlg, 100);

    // ExitDlg on top, interactive:
    ExitDlg->SetDialogInputEnabled(true);
    ShowDialog(ExitDlg, 200);
}

void UMenuScreen::ShowConfirmDlg()
{
    EnsureDialog<UConfirmDlg>(ConfirmDlgClass, ConfirmDlg);
    if (!ConfirmDlg) return;

    ShowDialog(ConfirmDlg, true);
}

void UMenuScreen::HideConfirmDlg()
{
    HideDialog(ConfirmDlg);
}

void UMenuScreen::ShowLoadDlg()
{
    EnsureDialog<ULoadDlg>(LoadDlgClass, LoadDlg);
    if (!LoadDlg) return;

    ShowDialog(LoadDlg, true);
}

void UMenuScreen::HideLoadDlg()
{
    HideDialog(LoadDlg);
}

// ------------------------------------------------------------
// Options hub
// ------------------------------------------------------------

void UMenuScreen::ShowOptionsScreen()
{
    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowOptionsScreen: BEGIN"));

    // DO NOT HideAll yet — if creation fails you'll blank the UI.

    if (!OptionsScreenClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: OptionsScreenClass is NULL"));
        // Keep whatever is currently visible:
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: OwningPlayer is NULL"));
        return;
    }

    EnsureDialog<UOptionsScreen>(OptionsScreenClass, OptionsScreen);
    if (!OptionsScreen)
    {
        UE_LOG(LogTemp, Error, TEXT("[MenuScreen] ShowOptionsScreen: EnsureDialog failed (OptionsScreen is NULL)"));
        return;
    }

    // Now that we know we have a valid widget, we can hide everything else:
    HideAll();

    OptionsScreen->SetMenuManager(this);

    // HARD FORCE visibility + viewport, bypass any weird state:
    if (OptionsScreen->IsInViewport())
        OptionsScreen->RemoveFromParent();

    OptionsScreen->AddToViewport(200);                 // force top
    OptionsScreen->SetVisibility(ESlateVisibility::Visible);
    OptionsScreen->SetIsEnabled(true);
    OptionsScreen->SetIsFocusable(true);
    OptionsScreen->SetDialogInputEnabled(true);

    CurrentDialog = OptionsScreen;

    UE_LOG(LogTemp, Warning, TEXT("[MenuScreen] ShowOptionsScreen: SHOWN InViewport=%d Vis=%d"),
        OptionsScreen->IsInViewport() ? 1 : 0,
        (int32)OptionsScreen->GetVisibility());
}

void UMenuScreen::HideOptionsScreen()
{
    HideDialog(OptionsScreen);
}

void UMenuScreen::ReturnFromOptions()
{
    ShowMenuDlg();
}

void UMenuScreen::ReturnFromPlayerDlg()
{
    ShowMenuDlg();
}

// ------------------------------------------------------------
// BaseScreen unified key handling targets
// ------------------------------------------------------------

void UMenuScreen::HandleAccept()
{
    if (CurrentDialog && CurrentDialog != this)
    {
        CurrentDialog->HandleAccept();
        return;
    }

    Super::HandleAccept();
}

void UMenuScreen::HandleCancel()
{
    if (CloseTopmost())
        return;

    Super::HandleCancel();
}



