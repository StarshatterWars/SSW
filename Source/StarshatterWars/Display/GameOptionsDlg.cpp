/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe (Unreal Port)
    FILE:           GameOptionsDlg.cpp
    AUTHOR:         Carlos Bott
=============================================================================*/

#include "GameOptionsDlg.h"

// UMG
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

// Router
#include "OptionsScreen.h"

// Legacy
#include "Ship.h"
#include "HUDView.h"
#include "PlayerCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogGameOptionsDlg, Log, All);

UGameOptionsDlg::UGameOptionsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetDialogInputEnabled(true);
}

void UGameOptionsDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    ApplyButton = ApplyBtn;
    CancelButton = CancelBtn;

    BindDelegates();
}

void UGameOptionsDlg::NativePreConstruct()
{
    Super::NativePreConstruct();

    EnsureAutoVerticalBox();
    if (AutoVBox)
        AutoVBox->ClearChildren();
}

void UGameOptionsDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BindDelegates();

    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
    {
        UE_LOG(LogGameOptionsDlg, Error, TEXT("[GameOptionsDlg] BuildRows FAILED: AutoVBox is NULL"));
        return;
    }

    VBox->SetVisibility(ESlateVisibility::Visible);

    BuildListsIfNeeded();
    BuildGameRows();

    UE_LOG(LogGameOptionsDlg, Warning, TEXT("[GameOptionsDlg] AutoVBox children after BuildGameRows: %d"),
        VBox->GetChildrenCount());

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }
}

void UGameOptionsDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    ExecFrame((double)InDeltaTime);
}

void UGameOptionsDlg::ExecFrame(double /*DeltaTime*/)
{
    // UE-only: no polling required.
}

void UGameOptionsDlg::BindFormWidgets() {}
FString UGameOptionsDlg::GetLegacyFormText() const { return FString(); }

void UGameOptionsDlg::HandleAccept() { OnApplyClicked(); }
void UGameOptionsDlg::HandleCancel() { OnCancelClicked(); }

void UGameOptionsDlg::BindDelegates()
{
    if (bDelegatesBound)
        return;

    // Apply/Cancel
    if (ApplyBtn)  ApplyBtn->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnApplyClicked);
    if (CancelBtn) CancelBtn->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnCancelClicked);

    // Tabs
    if (AudTabButton) AudTabButton->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnAudioClicked);
    if (VidTabButton) VidTabButton->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnVideoClicked);
    if (CtlTabButton) CtlTabButton->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnControlsClicked);
    if (OptTabButton) OptTabButton->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnGameClicked);
    if (ModTabButton) ModTabButton->OnClicked.AddUniqueDynamic(this, &UGameOptionsDlg::OnModClicked);

    // Combos
    if (FlightModelCombo)  FlightModelCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnFlightModelChanged);
    if (FlyingStartCombo)  FlyingStartCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnFlyingStartChanged);
    if (LandingsCombo)     LandingsCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnLandingsChanged);
    if (AIDifficultyCombo) AIDifficultyCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnAIDifficultyChanged);
    if (HudModeCombo)      HudModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnHudModeChanged);
    if (HudColorCombo)     HudColorCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnHudColorChanged);
    if (FriendlyFireCombo) FriendlyFireCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnFriendlyFireChanged);
    if (GridModeCombo)     GridModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnGridModeChanged);
    if (GunsightCombo)     GunsightCombo->OnSelectionChanged.AddUniqueDynamic(this, &UGameOptionsDlg::OnGunsightChanged);

    bDelegatesBound = true;
}

void UGameOptionsDlg::Show()
{
    SetVisibility(ESlateVisibility::Visible);

    if (bClosed)
    {
        RefreshFromModel();
        bClosed = false;
        bDirty = false;
    }

    SetKeyboardFocus();
}

// ------------------------------------------------------------
// Layout build
// ------------------------------------------------------------

void UGameOptionsDlg::BuildGameRows()
{
    UVerticalBox* VBox = EnsureAutoVerticalBox();
    if (!VBox)
        return;

    VBox->ClearChildren();

    auto Require = [&](UWidget* W, const TCHAR* Name) -> bool
        {
            if (!W)
            {
                UE_LOG(LogGameOptionsDlg, Error,
                    TEXT("[GameOptionsDlg] Widget '%s' is NULL (BP name mismatch or not IsVariable)."), Name);
                return false;
            }
            return true;
        };

    const bool bOK =
        Require(FlightModelCombo, TEXT("FlightModelCombo")) &
        Require(LandingsCombo, TEXT("LandingsCombo")) &
        Require(FlyingStartCombo, TEXT("FlyingStartCombo")) &
        Require(AIDifficultyCombo, TEXT("AIDifficultyCombo")) &
        Require(HudModeCombo, TEXT("HudModeCombo")) &
        Require(HudColorCombo, TEXT("HudColorCombo")) &
        Require(FriendlyFireCombo, TEXT("FriendlyFireCombo")) &
        Require(GridModeCombo, TEXT("GridModeCombo")) &
        Require(GunsightCombo, TEXT("GunsightCombo"));

    if (!bOK)
    {
        UE_LOG(LogGameOptionsDlg, Error, TEXT("[GameOptionsDlg] BuildGameRows aborted due to NULL controls."));
        return;
    }

    constexpr float W = 520.f;

    AddLabeledRow(TEXT("FLIGHT MODEL"), FlightModelCombo, W);
    AddLabeledRow(TEXT("LANDINGS"), LandingsCombo, W);
    AddLabeledRow(TEXT("FLYING START"), FlyingStartCombo, W);
    AddLabeledRow(TEXT("AI DIFFICULTY"), AIDifficultyCombo, W);
    AddLabeledRow(TEXT("HUD MODE"), HudModeCombo, W);
    AddLabeledRow(TEXT("HUD COLOR"), HudColorCombo, W);
    AddLabeledRow(TEXT("FRIENDLY FIRE"), FriendlyFireCombo, W);
    AddLabeledRow(TEXT("GRID MODE"), GridModeCombo, W);
    AddLabeledRow(TEXT("GUNSIGHT"), GunsightCombo, W);
}

void UGameOptionsDlg::BuildListsIfNeeded()
{
    BuildFlightModelListIfNeeded();
    BuildLandingModelListIfNeeded();
    BuildFlyingStartListIfNeeded();
    BuildAIDifficultyListIfNeeded();
    BuildHudModeListIfNeeded();
    BuildHudColorListIfNeeded();
    BuildFriendlyFireListIfNeeded();
    BuildGridModeListIfNeeded();
    BuildGunsightListIfNeeded();
}

static void EnsureComboEmptyOrClear(UComboBoxString* Combo)
{
    if (!Combo) return;
    if (Combo->GetOptionCount() > 0) return;
    Combo->ClearOptions();
}

void UGameOptionsDlg::BuildFlightModelListIfNeeded()
{
    if (!FlightModelCombo) return;
    if (FlightModelCombo->GetOptionCount() > 0) return;

    FlightModelCombo->ClearOptions();
    FlightModelCombo->AddOption(TEXT("ARCADE"));
    FlightModelCombo->AddOption(TEXT("FLIGHTSIM"));
    FlightModelCombo->AddOption(TEXT("EXPERT"));
    FlightModelCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildLandingModelListIfNeeded()
{
    if (!LandingsCombo) return;
    if (LandingsCombo->GetOptionCount() > 0) return;

    LandingsCombo->ClearOptions();
    LandingsCombo->AddOption(TEXT("EASY"));
    LandingsCombo->AddOption(TEXT("STANDARD"));
    LandingsCombo->AddOption(TEXT("REALISTIC"));
    LandingsCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildFlyingStartListIfNeeded()
{
    if (!FlyingStartCombo) return;
    if (FlyingStartCombo->GetOptionCount() > 0) return;

    FlyingStartCombo->ClearOptions();
    FlyingStartCombo->AddOption(TEXT("HANGAR"));
    FlyingStartCombo->AddOption(TEXT("LAUNCH"));
    FlyingStartCombo->AddOption(TEXT("IN FLIGHT"));
    FlyingStartCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildAIDifficultyListIfNeeded()
{
    if (!AIDifficultyCombo) return;
    if (AIDifficultyCombo->GetOptionCount() > 0) return;

    // IMPORTANT: your legacy mapping does:
    // AIIndex = OptionCount - AILevel - 1  (in RefreshFromModel + PushToModel)
    // So we must order from HARDEST -> EASIEST (or vice versa) consistently.
    AIDifficultyCombo->ClearOptions();
    AIDifficultyCombo->AddOption(TEXT("ACE"));
    AIDifficultyCombo->AddOption(TEXT("VETERAN"));
    AIDifficultyCombo->AddOption(TEXT("REGULAR"));
    AIDifficultyCombo->AddOption(TEXT("ROOKIE"));
    AIDifficultyCombo->SetSelectedIndex(2);
}

void UGameOptionsDlg::BuildHudModeListIfNeeded()
{
    if (!HudModeCombo) return;
    if (HudModeCombo->GetOptionCount() > 0) return;

    HudModeCombo->ClearOptions();
    HudModeCombo->AddOption(TEXT("REALISTIC"));
    HudModeCombo->AddOption(TEXT("ARCADE"));
    HudModeCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildHudColorListIfNeeded()
{
    if (!HudColorCombo) return;
    if (HudColorCombo->GetOptionCount() > 0) return;

    HudColorCombo->ClearOptions();
    HudColorCombo->AddOption(TEXT("GREEN"));
    HudColorCombo->AddOption(TEXT("AMBER"));
    HudColorCombo->AddOption(TEXT("CYAN"));
    HudColorCombo->AddOption(TEXT("WHITE"));
    HudColorCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildFriendlyFireListIfNeeded()
{
    if (!FriendlyFireCombo) return;
    if (FriendlyFireCombo->GetOptionCount() > 0) return;

    // FriendlyFireIndex maps to: Ship::SetFriendlyFireLevel(Index / 4.0f)
    FriendlyFireCombo->ClearOptions();
    FriendlyFireCombo->AddOption(TEXT("OFF"));
    FriendlyFireCombo->AddOption(TEXT("LOW"));
    FriendlyFireCombo->AddOption(TEXT("MEDIUM"));
    FriendlyFireCombo->AddOption(TEXT("HIGH"));
    FriendlyFireCombo->AddOption(TEXT("FULL"));
    FriendlyFireCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildGridModeListIfNeeded()
{
    if (!GridModeCombo) return;
    if (GridModeCombo->GetOptionCount() > 0) return;

    GridModeCombo->ClearOptions();
    GridModeCombo->AddOption(TEXT("OFF"));
    GridModeCombo->AddOption(TEXT("ON"));
    GridModeCombo->SetSelectedIndex(0);
}

void UGameOptionsDlg::BuildGunsightListIfNeeded()
{
    if (!GunsightCombo) return;
    if (GunsightCombo->GetOptionCount() > 0) return;

    GunsightCombo->ClearOptions();
    GunsightCombo->AddOption(TEXT("OFF"));
    GunsightCombo->AddOption(TEXT("LEAD"));
    GunsightCombo->AddOption(TEXT("PIP"));
    GunsightCombo->SetSelectedIndex(1);
}

// ------------------------------------------------------------
// Model -> UI
// ------------------------------------------------------------

void UGameOptionsDlg::RefreshFromModel()
{
    // Pull from legacy globals
    FlightModelIndex = Ship::GetFlightModel();
    LandingModelIndex = Ship::GetLandingModel();

    HudModeIndex = HUDView::IsArcade() ? 1 : 0;
    HudColorIndex = HUDView::DefaultColorSet();

    FriendlyFireIndex = (int32)(Ship::GetFriendlyFireLevel() * 4.0f);

    if (PlayerCharacter* Player = PlayerCharacter::GetCurrentPlayer())
    {
        FlyingStartIndex = Player->FlyingStart();

        if (AIDifficultyCombo)
            AIDifficultyIndex = AIDifficultyCombo->GetOptionCount() - Player->AILevel() - 1;

        GridModeIndex = Player->GridMode();
        GunsightIndex = Player->Gunsight();
    }

    if (FlightModelCombo)  FlightModelCombo->SetSelectedIndex(FlightModelIndex);
    if (LandingsCombo)     LandingsCombo->SetSelectedIndex(LandingModelIndex);

    if (HudModeCombo)      HudModeCombo->SetSelectedIndex(HudModeIndex);
    if (HudColorCombo)     HudColorCombo->SetSelectedIndex(HudColorIndex);
    if (FriendlyFireCombo) FriendlyFireCombo->SetSelectedIndex(FriendlyFireIndex);

    if (FlyingStartCombo)  FlyingStartCombo->SetSelectedIndex(FlyingStartIndex);
    if (AIDifficultyCombo) AIDifficultyCombo->SetSelectedIndex(AIDifficultyIndex);
    if (GridModeCombo)     GridModeCombo->SetSelectedIndex(GridModeIndex);
    if (GunsightCombo)     GunsightCombo->SetSelectedIndex(GunsightIndex);
}

// ------------------------------------------------------------
// UI -> Model
// ------------------------------------------------------------

void UGameOptionsDlg::PushToModel()
{
    if (PlayerCharacter* Player = PlayerCharacter::GetCurrentPlayer())
    {
        if (FlightModelCombo)  Player->SetFlightModel(FlightModelCombo->GetSelectedIndex());
        if (FlyingStartCombo)  Player->SetFlyingStart(FlyingStartCombo->GetSelectedIndex());
        if (LandingsCombo)     Player->SetLandingModel(LandingsCombo->GetSelectedIndex());

        if (AIDifficultyCombo)
            Player->SetAILevel(AIDifficultyCombo->GetOptionCount() - AIDifficultyCombo->GetSelectedIndex() - 1);

        if (HudModeCombo)      Player->SetHUDMode(HudModeCombo->GetSelectedIndex());
        if (HudColorCombo)     Player->SetHUDColor(HudColorCombo->GetSelectedIndex());
        if (FriendlyFireCombo) Player->SetFriendlyFire(FriendlyFireCombo->GetSelectedIndex());
        if (GridModeCombo)     Player->SetGridMode(GridModeCombo->GetSelectedIndex());
        if (GunsightCombo)     Player->SetGunsight(GunsightCombo->GetSelectedIndex());

        PlayerCharacter::Save();
    }

    if (FlightModelCombo) Ship::SetFlightModel(FlightModelCombo->GetSelectedIndex());
    if (LandingsCombo)    Ship::SetLandingModel(LandingsCombo->GetSelectedIndex());

    if (HudModeCombo)  HUDView::SetArcade(HudModeCombo->GetSelectedIndex() > 0);
    if (HudColorCombo) HUDView::SetDefaultColorSet(HudColorCombo->GetSelectedIndex());

    if (FriendlyFireCombo)
        Ship::SetFriendlyFireLevel((float)FriendlyFireCombo->GetSelectedIndex() / 4.0f);

    if (HUDView* Hud = HUDView::GetInstance())
        if (HudColorCombo) Hud->SetHUDColorSet(HudColorCombo->GetSelectedIndex());

    bDirty = false;
}

void UGameOptionsDlg::Apply()
{
    if (!bDirty)
        return;

    PushToModel();
    bClosed = true;
}

void UGameOptionsDlg::Cancel()
{
    RefreshFromModel();
    bClosed = true;
    bDirty = false;
}

// ------------------------------------------------------------
// Apply / Cancel
// ------------------------------------------------------------

void UGameOptionsDlg::OnApplyClicked()
{
    if (OptionsManager) OptionsManager->ApplyOptions();
    else Apply();
}

void UGameOptionsDlg::OnCancelClicked()
{
    if (OptionsManager) OptionsManager->CancelOptions();
    else Cancel();
}

// ------------------------------------------------------------
// Tabs
// ------------------------------------------------------------

void UGameOptionsDlg::OnAudioClicked() { if (OptionsManager) OptionsManager->ShowAudDlg(); }
void UGameOptionsDlg::OnVideoClicked() { if (OptionsManager) OptionsManager->ShowVidDlg(); }
void UGameOptionsDlg::OnControlsClicked() { if (OptionsManager) OptionsManager->ShowCtlDlg(); }
void UGameOptionsDlg::OnGameClicked() { /* already here */ }
void UGameOptionsDlg::OnModClicked() { if (OptionsManager) OptionsManager->ShowModDlg(); }

// ------------------------------------------------------------
// Combo handlers
// ------------------------------------------------------------

void UGameOptionsDlg::OnFlightModelChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnFlyingStartChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnLandingsChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnAIDifficultyChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnHudModeChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnHudColorChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnFriendlyFireChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnGridModeChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
void UGameOptionsDlg::OnGunsightChanged(FString, ESelectInfo::Type) { bDirty = true; bClosed = false; }
