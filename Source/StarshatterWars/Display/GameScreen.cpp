/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         GameScreen.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UGameScreen
    - In-game HUD + overlays only (Nav/Eng/Flt).
    - Options moved to MenuScreen/OptionsScreen.
*/

#include "GameScreen.h"

// UMG:
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Overlay widget headers:
#include "NavDlg.h"
#include "EngineeringDlg.h"
#include "FlightOpsDlg.h"
#include "OptionsScreen.h"

// Legacy:
#include "Sim.h"
#include "Ship.h"
#include "Starshatter.h"
#include "CameraManager.h"
#include "DisplayView.h"
#include "HUDView.h"
#include "WepView.h"
#include "QuantumView.h"
#include "RadioView.h"
#include "TacticalView.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "MFDView.h"
#include "Sensor.h"
#include "DataLoader.h"

#include "Components/Border.h"
#include "InputCoreTypes.h"

FReply UMfdHitButton::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    (void)InGeometry;
    if (OnHit)
        OnHit(MfdIndex, Corner, InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
    return FReply::Handled();
}

FReply UMfdHitButton::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    (void)InGeometry;
    (void)InMouseEvent;
    return FReply::Handled();
}

void UMfdHitButton::NativeConstruct()
{
    Super::NativeConstruct();
    if (!WidgetTree || WidgetTree->RootWidget)
        return;
    UBorder* Hit = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Hit"));
    Hit->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.01f));
    WidgetTree->RootWidget = Hit;
}

#include "Blueprint/WidgetTree.h"

void UGameScreen::EnsureMfdHits()
{
    if (MfdHitCanvas)
        return;

    UUserWidget* Host = CreateWidget<UUserWidget>(this, UUserWidget::StaticClass());
    if (!Host || !Host->WidgetTree)
        return;

    MfdHitCanvas = Host->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MfdHitCanvas"));
    Host->WidgetTree->RootWidget = MfdHitCanvas;
    MfdHitCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Host->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    Host->AddToViewport(80);
    MfdHitHost = Host;

    for (int32 Mfd = 0; Mfd < 3; ++Mfd)
    {
        for (int32 Corner = 0; Corner < 4; ++Corner)
        {
            UMfdHitButton* Hit = CreateWidget<UMfdHitButton>(this, UMfdHitButton::StaticClass());
            if (!Hit) continue;
            Hit->MfdIndex = Mfd;
            Hit->Corner = Corner;
            Hit->OnHit = [this](int32 Index, int32 Which, bool bRight) { HandleMfdHit(Index, Which, bRight); };
            Hit->SetVisibility(ESlateVisibility::Visible);
            if (UCanvasPanelSlot* HitSlot = MfdHitCanvas->AddChildToCanvas(Hit))
            {
                HitSlot->SetAutoSize(false);
                HitSlot->SetSize(FVector2D(44.f, 16.f));
                HitSlot->SetZOrder(50);
            }
            MfdHits.Add(Hit);
        }
    }
}

void UGameScreen::LayoutMfdHits()
{
    if (!MfdHitCanvas || MfdHits.Num() < 12)
        return;

    int32 Width = 0, Height = 0;
    if (APlayerController* PC = GetOwningPlayer())
        PC->GetViewportSize(Width, Height);
    if (Width <= 0 || Height <= 0)
        return;

    const FIntPoint Origins[3] = {
        FIntPoint(8, Height - 136),
        FIntPoint(Width - 136, Height - 136),
        FIntPoint(8, 8)
    };

    for (int32 Mfd = 0; Mfd < 3; ++Mfd)
    {
        const FIntPoint O = Origins[Mfd];
        const FVector2D Corners[4] = {
            FVector2D((float)O.X + 2.f, (float)O.Y + 2.f),
            FVector2D((float)O.X + 2.f, (float)O.Y + 112.f),
            FVector2D((float)O.X + 86.f, (float)O.Y + 2.f),
            FVector2D((float)O.X + 86.f, (float)O.Y + 112.f)
        };
        for (int32 Corner = 0; Corner < 4; ++Corner)
        {
            UMfdHitButton* Hit = MfdHits[Mfd * 4 + Corner];
            if (!Hit) continue;
            if (UCanvasPanelSlot* HitSlot = Cast<UCanvasPanelSlot>(Hit->Slot))
                HitSlot->SetPosition(Corners[Corner]);
        }
    }
}

void UGameScreen::HandleMfdHit(int32 MfdIndex, int32 Corner, bool bRight)
{
    if (!hud_view) hud_view = HUDView::GetInstance();
    MFDView* Mfd = hud_view ? hud_view->GetMFD(MfdIndex) : nullptr;
    Ship* ShipPtr = Mfd ? Mfd->GetShip() : nullptr;
    Sensor* SensorPtr = ShipPtr ? ShipPtr->GetSensor() : nullptr;

    if (Corner == 0 && SensorPtr)
        SensorPtr->SetMode(Sensor::GetNextSensorMode(SensorPtr->GetMode()));
    else if (Corner == 1 && SensorPtr)
        bRight ? SensorPtr->DecreaseRange() : SensorPtr->IncreaseRange();
    else if (Corner == 2 && hud_view)
        hud_view->CycleMFDMode(MfdIndex);
    else if (Corner == 3 && ShipPtr && ShipPtr->GetProbeLauncher())
        ShipPtr->LaunchProbe();
}
#include "Bitmap.h"
#include "HUDSounds.h"
#include "Game.h"

UGameScreen* UGameScreen::GameScreenInstance = nullptr;

UGameScreen::UGameScreen(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    GameScreenInstance = this;
}

void UGameScreen::NativeDestruct()
{
    TearDown();

    if (GameScreenInstance == this)
        GameScreenInstance = nullptr;

    Super::NativeDestruct();
}

void UGameScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    EnsureMfdHits();
    LayoutMfdHits();
    ExecFrame(InDeltaTime);
}

// ------------------------------------------------------------

UUserWidget* UGameScreen::MakeDlg(TSubclassOf<UUserWidget> Class, int32 ZOrder)
{
    if (!Class)
        return nullptr;

    UWorld* World = GetWorld();
    if (!World)
        return nullptr;

    UUserWidget* W = CreateWidget<UUserWidget>(World, Class);
    if (W)
    {
        W->AddToViewport(ZOrder);
        W->SetVisibility(ESlateVisibility::Hidden);
    }

    return W;
}

void UGameScreen::SetDlgVisible(UUserWidget* W, bool bVisible) const
{
    if (!W)
        return;

    W->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    W->SetIsEnabled(bVisible);
    if (UBaseScreen* Screen = Cast<UBaseScreen>(W))
    {
        Screen->SetIsFocusable(bVisible);
        Screen->SetDialogInputEnabled(bVisible);
    }
}

void UGameScreen::FocusOverlay(UUserWidget* W)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
        return;

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    FInputModeGameAndUI Mode;
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    if (W)
        Mode.SetWidgetToFocus(W->TakeWidget());
    PC->SetInputMode(Mode);
}

bool UGameScreen::IsDlgVisible(const UUserWidget* W) const
{
    return W && W->GetVisibility() == ESlateVisibility::Visible;
}

bool UGameScreen::IsOverlayShown() const
{
    return IsDlgVisible(NavDlg) || IsDlgVisible(EngDlg) || IsDlgVisible(FltDlg);
}

// ------------------------------------------------------------

void UGameScreen::Setup()
{
    // Cache legacy singletons/pointers (raw):
    sim = Sim::GetSim();
    loader = DataLoader::GetLoader();
    cam_dir = CameraManager::GetInstance();
    disp_view = DisplayView::GetInstance();

    // Spawn overlays:
    NavDlg = Cast<UNavDlg>(MakeDlg(NavDlgClass, 40));
    EngDlg = Cast<UEngineeringDlg>(MakeDlg(EngDlgClass, 40));
    FltDlg = Cast<UFlightOpsDlg>(MakeDlg(FltDlgClass, 40));

    HideAllOverlays();

    bIsShown = false;
}

void UGameScreen::TearDown()
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

    W = NavDlg; Kill(W); NavDlg = nullptr;
    W = EngDlg; Kill(W); EngDlg = nullptr;
    W = FltDlg; Kill(W); FltDlg = nullptr;

    // Clear raw legacy pointers (not owned here):
    sim = nullptr;
    cam_dir = nullptr;
    disp_view = nullptr;
    loader = nullptr;

    bIsShown = false;
}

// ------------------------------------------------------------

void UGameScreen::Show()
{
    if (bIsShown)
        return;

    bIsShown = true;
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    EnsureMfdHits();
    FocusOverlay(nullptr);
}

void UGameScreen::Hide()
{
    if (!bIsShown)
        return;

    HideAllOverlays();

    // Stop HUD alert sound (legacy parity):
    HUDSounds::StopSound(HUDSounds::SND_RED_ALERT);

    SetVisibility(ESlateVisibility::Hidden);
    bIsShown = false;
}

void UGameScreen::HideAllOverlays()
{
    SetDlgVisible(NavDlg, false);
    SetDlgVisible(EngDlg, false);
    SetDlgVisible(FltDlg, false);
}

// ------------------------------------------------------------
// Nav / Eng / Flt overlays
// ------------------------------------------------------------

void UGameScreen::ShowNavDlg()
{
    if (!NavDlg)
        return;

    if (!IsDlgVisible(NavDlg))
    {
        HideAllOverlays();

        sim = Sim::GetSim();
        if (sim)
        {
            NavDlg->SetSystem(sim->GetStarSystem());
            NavDlg->SetShip(sim->GetPlayerShip());
        }

        SetDlgVisible(NavDlg, true);
        FocusOverlay(NavDlg);
        Starshatter::GetInstance()->Pause(true);
    }
    else
    {
        HideNavDlg();
    }
}

void UGameScreen::HideNavDlg()
{
    if (NavDlg && IsDlgVisible(NavDlg))
    {
        SetDlgVisible(NavDlg, false);
        FocusOverlay(nullptr);
        Starshatter::GetInstance()->Pause(false);
    }
}

bool UGameScreen::IsNavShown() const { return IsDlgVisible(NavDlg); }

// ------------------------------------------------------------

void UGameScreen::ShowEngDlg()
{
    if (!EngDlg)
        return;

    if (!IsDlgVisible(EngDlg))
    {
        HideAllOverlays();

        sim = Sim::GetSim();
        if (sim)
            EngDlg->SetShip(sim->GetPlayerShip());

        SetDlgVisible(EngDlg, true);
        FocusOverlay(EngDlg);
        Starshatter::GetInstance()->Pause(true);
    }
    else
    {
        HideEngDlg();
    }
}

void UGameScreen::HideEngDlg()
{
    if (EngDlg && IsDlgVisible(EngDlg))
    {
        SetDlgVisible(EngDlg, false);
        FocusOverlay(nullptr);
        Starshatter::GetInstance()->Pause(false);
    }
}

bool UGameScreen::IsEngShown() const { return IsDlgVisible(EngDlg); }

// ------------------------------------------------------------

void UGameScreen::ShowFltDlg()
{
    if (!FltDlg)
        return;

    if (!IsDlgVisible(FltDlg))
    {
        HideAllOverlays();

        sim = Sim::GetSim();
        if (sim)
            FltDlg->SetShip(sim->GetPlayerShip());

        SetDlgVisible(FltDlg, true);
        FocusOverlay(FltDlg);
        Starshatter::GetInstance()->Pause(true);
    }
    else
    {
        HideFltDlg();
    }
}

void UGameScreen::HideFltDlg()
{
    if (FltDlg && IsDlgVisible(FltDlg))
    {
        SetDlgVisible(FltDlg, false);
        FocusOverlay(nullptr);
        Starshatter::GetInstance()->Pause(false);
    }
}

bool UGameScreen::IsFltShown() const { return IsDlgVisible(FltDlg); }

// ------------------------------------------------------------
// Weapons overlay (WepView overlay mode)
// ------------------------------------------------------------

void UGameScreen::ShowWeaponsOverlay()
{
    if (wep_view)
        wep_view->CycleOverlayMode();
}

void UGameScreen::HideWeaponsOverlay()
{
    if (wep_view)
        wep_view->SetOverlayMode(0);
}

// ------------------------------------------------------------
// CloseTopmost
// ------------------------------------------------------------

bool UGameScreen::CloseTopmost()
{
    if (NavDlg && IsDlgVisible(NavDlg)) { HideNavDlg(); return true; }
    if (EngDlg && IsDlgVisible(EngDlg)) { HideEngDlg(); return true; }
    if (FltDlg && IsDlgVisible(FltDlg)) { HideFltDlg(); return true; }

    // Menu views (legacy):
    if (quantum_view && quantum_view->IsMenuShown()) { quantum_view->CloseMenu(); return true; }
    if (radio_view && radio_view->IsMenuShown()) { radio_view->CloseMenu();   return true; }

    return false;
}

// ------------------------------------------------------------
// Camera/HUD helpers
// ------------------------------------------------------------

void UGameScreen::FrameRate(double F) { frame_rate = F; }

void UGameScreen::SetFieldOfView(double Fov)
{
    if (cam_view)
        cam_view->SetFieldOfView(Fov);
}

double UGameScreen::GetFieldOfView() const
{
    return cam_view ? cam_view->GetFieldOfView() : 0.0;
}

void UGameScreen::CycleMFDMode(int Mfd) { if (hud_view) hud_view->CycleMFDMode(Mfd); }
void UGameScreen::CycleHUDMode() { if (hud_view) hud_view->CycleHUDMode(); }
void UGameScreen::CycleHUDColor() { if (hud_view) hud_view->CycleHUDColor(); }
void UGameScreen::CycleHUDWarn() { if (hud_view) hud_view->CycleHUDWarn(); }

// ------------------------------------------------------------
// ExecFrame
// ------------------------------------------------------------

void UGameScreen::ExecFrame(float DeltaTime)
{
    sim = Sim::GetSim();
    if (!sim)
        return;

    Ship* player = sim->GetPlayerShip();
    if (!player)
        return;

    const bool bOverlayShowing = IsOverlayShown();

    // HUD always updates:
    if (hud_view)
    {
        hud_view->UseCameraView(cam_view);
        hud_view->ExecFrame();
    }

    // Overlay ticks:
    if (NavDlg && IsDlgVisible(NavDlg))
    {
        NavDlg->SetShip(player);
        NavDlg->ExecFrame();
    }

    if (EngDlg && IsDlgVisible(EngDlg))
    {
        EngDlg->SetShip(player);
        EngDlg->ExecFrame(DeltaTime);
    }

    if (FltDlg && IsDlgVisible(FltDlg))
    {
        FltDlg->SetShip(player);
        FltDlg->ExecFrame();
    }

    // Only run these when no overlay is blocking:
    if (!bOverlayShowing)
    {
        if (quantum_view) quantum_view->ExecFrame();
        if (radio_view)   radio_view->ExecFrame();
        if (wep_view)     wep_view->ExecFrame();
        if (tac_view)     tac_view->ExecFrame();
    }

    if (disp_view)
        disp_view->ExecFrame();
}

// ------------------------------------------------------------
// External/Internal view toggles
// ------------------------------------------------------------

void UGameScreen::ShowExternal()
{
    bExternalVisible = true;
    ApplyInputModeForScreen(true);
}

void UGameScreen::ShowInternal()
{
    bExternalVisible = false;
    ApplyInputModeForScreen(false);
}

void UGameScreen::ApplyInputModeForScreen(bool /*bExternal*/)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC)
        return;

    FInputModeGameAndUI Mode;
    Mode.SetWidgetToFocus(TakeWidget());
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    Mode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(Mode);
    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;
}

bool UGameScreen::IsFormShown() const
{
    return IsDlgVisible(NavDlg) ||
        IsDlgVisible(EngDlg) ||
        IsDlgVisible(FltDlg) ||
        IsDlgVisible(OptDlg);
}
