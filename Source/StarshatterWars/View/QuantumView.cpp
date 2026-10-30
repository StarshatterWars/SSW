/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025–2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         QuantumView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    QuantumView
    - Quantum Drive destination HUD overlay
    - Displays selectable quantum destinations
    - Numeric hotkey selection (1–9)
    - Ported from Starshatter 4.5 QuantumView
    - Uses legacy View + Menu system (non-UObject)
*/

#include "QuantumView.h"

#include "QuantumDrive.h"
#include "QuantumTravel.h"
#include "HUDView.h"
#include "View.h"
#include "Ship.h"
#include "Sim.h"
#include "SimRegion.h"
#include "StarSystem.h"
#include "FormatUtil.h"

#include "ActiveWindow.h"
#include "Game.h"
#include "Menu.h"
#include "Keyboard.h"
#include "SystemFont.h"
#include "GameStructs.h"

// ------------------------------------------------------------
// Static state (matches legacy behavior)
// ------------------------------------------------------------
static FColor HudColor = FColor::Black;

static Menu* QuantumMenu = nullptr;
static bool  bShowMenu = false;

QuantumView* QuantumView::QuantumViewInstance = nullptr;

// ------------------------------------------------------------
// Lifecycle
// ------------------------------------------------------------
void QuantumView::Initialize()
{
    if (!QuantumMenu) QuantumMenu = new Menu("Quantum Drive");
}

void QuantumView::Close()
{
    delete QuantumMenu;
    QuantumMenu = nullptr;
    bShowMenu = false;
}

// ------------------------------------------------------------
// Ctor / Dtor
// ------------------------------------------------------------
QuantumView::QuantumView(View* InParent, ActiveWindow* InActiveWindow)
    : View(
        InParent,
        0,
        0,
        InActiveWindow ? InActiveWindow->Width() : 1024,
        InActiveWindow ? InActiveWindow->Height() : 768
    )
{
    QuantumViewInstance = this;

    // Bind rendering context (View owns this):
    window = InActiveWindow;

    SimPtr = Sim::GetSim();
    ShipPtr = nullptr;

    WidthPx = window ? window->Width() : rect.w;
    HeightPx = window ? window->Height() : rect.h;

    XCenter = (WidthPx / 2.0) - 0.5;
    YCenter = (HeightPx / 2.0) + 0.5;

    // Use View-provided fonts
    HudFont = HUDFont;

    HUDView* Hud = HUDView::GetInstance();
    if (Hud)
        SetColor(Hud->GetTextColor());
}

QuantumView::~QuantumView()
{
    QuantumViewInstance = nullptr;
}

// ------------------------------------------------------------
// View hooks
// ------------------------------------------------------------
void QuantumView::OnWindowMove()
{
    WidthPx = window ? window->Width() : rect.w;
    HeightPx = window ? window->Height() : rect.h;

    XCenter = (WidthPx / 2.0) - 0.5;
    YCenter = (HeightPx / 2.0) + 0.5;
}

bool QuantumView::Update(SimObject* Obj)
{
    if (Obj == ShipPtr)
        ShipPtr = nullptr;

    return SimObserver::Update(Obj);
}

const char* QuantumView::GetObserverName() const
{
    return "QuantumView";
}

void QuantumView::Refresh()
{
    SimPtr = Sim::GetSim();

    if (SimPtr && ShipPtr != SimPtr->GetPlayerShip()) {
        ShipPtr = SimPtr->GetPlayerShip();

        if (ShipPtr) {
            if (ShipPtr->GetLife() == 0 || ShipPtr->IsDying() || ShipPtr->IsDead()) {
                ShipPtr = nullptr;
            }
            else {
                Observe(ShipPtr);
            }
        }
    }

    if (!IsMenuShown() || !QuantumMenu || !HudFont || !window)
        return;

    Rect MenuRect(WidthPx - 115, 10, 115, 12);

    HudFont->SetColor(HudColor);
    HudFont->SetAlpha(1);

    SetFont(HudFont);

    // ----- Title -----
    {
        const FString TitleStr = QuantumMenu->GetTitle();
        FTCHARToUTF8 TitleUtf8(*TitleStr);
        DrawTextRect(TitleUtf8.Get(), -1, MenuRect, DT_LEFT);
    }

    // ----- Items -----
    MenuRect.y += 15;

    const TArray<MenuItem*>& Items = QuantumMenu->GetItems();
    for (MenuItem* Item : Items)
    {
        if (!Item)
            continue;

        Item->SetEnabled(true);

        const FString ItemStr = Item->GetText();
        FTCHARToUTF8 ItemUtf8(*ItemStr);
        DrawTextRect(ItemUtf8.Get(), -1, MenuRect, DT_LEFT);

        MenuRect.y += 10;
    }
}

void QuantumView::ExecFrame()
{
    HUDView* Hud = HUDView::GetInstance();
    if (Hud) {
        const FColor NewColor = Hud->GetTextColor();
        if (HudColor != NewColor) {
            HudColor = NewColor;
            SetColor(HudColor);
        }
    }

    static double TimeTilChange = 0.0;

    if (TimeTilChange > 0.0)
        TimeTilChange -= Game::GUITime();

    if (TimeTilChange > 0.0)
        return;

    TimeTilChange = 0.0;

    if (!bShowMenu || !QuantumMenu)
        return;

    QuantumDrive* QDrive = nullptr;
    if (ShipPtr)
        QDrive = ShipPtr->GetQuantumDrive();

    if (QDrive && QDrive->ActiveState() != QuantumDrive::ACTIVE_READY) {
        bShowMenu = false;
        return;
    }

    const int32 MaxItems = FMath::Min(9, QuantumMenu->NumItems());
    for (int32 i = 0; i < MaxItems; i++) {
        if (Keyboard::KeyDown('1' + i)) {
            MenuItem* Item = QuantumMenu->GetItem(i);
            if (Item && Item->IsEnabled()) {

                // Stored pointer data:
                SimRegion* Region = reinterpret_cast<SimRegion*>(Item->GetData());

                if (Region && QDrive && SimPtr && QuantumTravel::GetRegions(SimPtr, ShipPtr).Contains(Region)) {
                    QDrive->SetDestination(Region, FVector::ZeroVector);
                    QDrive->Engage();
                }

                bShowMenu = false;
                TimeTilChange = 0.3;
                break;
            }
        }
    }
}

void QuantumView::SetColor(const FColor& InColor)
{
    SetHUDColor(InColor);
}

bool QuantumView::IsMenuShown() const
{
    return bShowMenu;
}

void QuantumView::ShowMenu()
{
    SimPtr = Sim::GetSim();
    Ship* CurrentShip = SimPtr ? SimPtr->GetPlayerShip() : nullptr;
    if (ShipPtr != CurrentShip)
    {
        if (ShipPtr) Ignore(ShipPtr);
        ShipPtr = CurrentShip;
        if (ShipPtr) Observe(ShipPtr);
    }
    if (!ShipPtr) return;

    if (!bShowMenu) {
        if (ShipPtr->IsStarship() && ShipPtr->GetQuantumDrive()) {
            GetQuantumMenu(ShipPtr);
            bShowMenu = true;
        }

        // Clear stale key presses:
        for (int32 i = 0; i < 10; i++) {
            if (Keyboard::KeyDown('1' + i)) {
                // intentionally empty (legacy behavior)
            }
        }
    }
}

void QuantumView::CloseMenu()
{
    bShowMenu = false;
}

// ------------------------------------------------------------
// Menu builder
// ------------------------------------------------------------
Menu* QuantumView::GetQuantumMenu(Ship* InShip)
{
    SimPtr = Sim::GetSim();
    Initialize();
    if (!InShip || !InShip->IsStarship() || !SimPtr || !QuantumMenu) return nullptr;
    QuantumMenu->ClearItems();
    int Index = 1;
    for (SimRegion* Region : QuantumTravel::GetRegions(SimPtr, InShip))
    {
        const FString Label = Region->GetSystem() == InShip->GetRegion()->GetSystem()
            ? FString::Printf(TEXT("%d. %hs"), Index++, Region->GetName())
            : FString::Printf(TEXT("%d. %hs/%hs"), Index++, Region->GetSystem()->GetName(), Region->GetName());
        QuantumMenu->AddItem(TCHAR_TO_UTF8(*Label), reinterpret_cast<uintptr_t>(Region));
    }
    return QuantumMenu;
}
