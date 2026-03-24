/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         WepView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Weapons Overlay View (Unreal-friendly port)
*/

#include "WepView.h"

#include "HUDView.h"
#include "Sim.h"
#include "Ship.h"
#include "Weapon.h"
#include "WeaponGroup.h"
#include "SimObject.h"

WepView* WepView::wep_view = nullptr;
bool     WepView::bMouseLatched = false;

// ------------------------------------------------------------
// Construction
// ------------------------------------------------------------

WepView::WepView(View* InParent, int32 InWidth, int32 InHeight)
    : View(InParent, 0, 0, InWidth, InHeight)
{
    wep_view = this;

    width = InWidth;
    height = InHeight;

    sim = Sim::GetSim();
    ship = nullptr;

    // Default font:
    if (HUDFont)
        SetFont(HUDFont);

    // Default color from HUDView if available:
    if (HUDView* hud = HUDView::GetInstance())
        SetColor(hud->GetHUDColor());
    else
        SetColor(FColor::Green);

    OnWindowMove();
}

WepView::~WepView()
{
    if (wep_view == this)
        wep_view = nullptr;

    // If we own the latch:
    if (bMouseLatched)
        bMouseLatched = false;
}

// ------------------------------------------------------------
// Singleton helpers
// ------------------------------------------------------------

void WepView::Initialize(View* Parent, int32 W, int32 H)
{
    if (!wep_view && Parent)
        new WepView(Parent, W, H);
}

void WepView::Close()
{
    if (wep_view) {
        delete wep_view;
        wep_view = nullptr;
    }
}

// ------------------------------------------------------------
// Colors
// ------------------------------------------------------------

void WepView::CycleOverlayMode()
{
    // Legacy Starshatter behavior:
    // 0 = hidden
    // 1 = compact
    // 2 = expanded

    mode = (mode + 1) % 3;

    // Force layout refresh if needed
    OnWindowMove();
}

void WepView::SetColor(FColor c)
{
    if (HUDView* hud = HUDView::GetInstance()) {
        LocalHudColor = hud->GetHUDColor();
        LocalTextColor = hud->GetTextColor();
    }
    else {
        LocalHudColor = c;
        LocalTextColor = c;
    }

    // Optionally also drive base View colors:
    SetHUDColor(LocalHudColor);
    SetTextColor(LocalTextColor);
}

// ------------------------------------------------------------
// View lifecycle
// ------------------------------------------------------------

void WepView::OnWindowMove()
{
    // In your View system rect is already stored; keep width/height in sync:
    width = rect.w;
    height = rect.h;
}

void WepView::ExecFrame()
{
    // Keep the ship pointer fresh:
    sim = Sim::GetSim();
    if (sim)
        ship = sim->GetPlayerShip();
    else
        ship = nullptr;
}

void WepView::Refresh()
{
    if (!IsShown() || !bShownOverlay)
        return;

    // Update ship each draw as well (safe redundancy):
    sim = Sim::GetSim();
    ship = sim ? sim->GetPlayerShip() : nullptr;

    // Draw a simple overlay frame for now:
    const Rect ovr = GetOverlayRect();
    FillRect(ovr, FColor(0, 0, 0, 160), 0);
    DrawRect(ovr, LocalHudColor, 0);

    // Title:
    Rect titleR = ovr;
    titleR.h = 16;
    DrawTextRect(FString(TEXT("WEAPONS")), -1, titleR, DT_CENTER);

    if (!ship)
        return;

    // Weapon rows:
    int32 max_wep = (int32)ship->GetWeapons().size();
    if (max_wep > MAX_WEP)
        max_wep = MAX_WEP;

    // Draw each weapon line + 4 buttons:
    for (int32 i = 0; i < max_wep; ++i) {
        // Row label area:
        Rect row = ovr;
        row.y += 20 + i * 18;
        row.h = 16;

        // Left label:
        Rect labelR = row;
        labelR.x += 10;
        labelR.w = 120;

        const FString Label = FString::Printf(TEXT("WEP %02d"), i + 1);
        DrawTextRect(Label, -1, labelR, DT_LEFT);

        // Buttons (Fire / MAN / AUTO / DEF):
        const int32 baseIndex = i * 4;

        Rect b0 = GetButtonRect(baseIndex + 0);
        Rect b1 = GetButtonRect(baseIndex + 1);
        Rect b2 = GetButtonRect(baseIndex + 2);
        Rect b3 = GetButtonRect(baseIndex + 3);

        DrawRect(b0, LocalHudColor, 0);
        DrawRect(b1, LocalHudColor, 0);
        DrawRect(b2, LocalHudColor, 0);
        DrawRect(b3, LocalHudColor, 0);

        DrawTextRect(FString(TEXT("FIRE")), -1, b0, DT_CENTER);
        DrawTextRect(FString(TEXT("MAN")), -1, b1, DT_CENTER);
        DrawTextRect(FString(TEXT("AUTO")), -1, b2, DT_CENTER);
        DrawTextRect(FString(TEXT("DEF")), -1, b3, DT_CENTER);
    }

    // Subtarget arrows if target exists:
    if (ship->GetTarget()) {
        Rect leftArrow(rect.w / 2 + 50, 70, 20, 20);
        Rect rightArrow(rect.w / 2 + 180, 70, 20, 20);

        DrawRect(leftArrow, LocalHudColor, 0);
        DrawRect(rightArrow, LocalHudColor, 0);

        DrawTextRect(FString(TEXT("<")), -1, leftArrow, DT_CENTER);
        DrawTextRect(FString(TEXT(">")), -1, rightArrow, DT_CENTER);
    }
}

// ------------------------------------------------------------
// Input routing (UE-friendly)
// ------------------------------------------------------------

bool WepView::OnMouseMove(const FVector2D& ScreenPos)
{
    if (!bShownOverlay)
        return false;

    const int32 X = (int32)ScreenPos.X;
    const int32 Y = (int32)ScreenPos.Y;

    mouse_in = (GetOverlayRect().Contains(X, Y) != 0);

    // If we latched, we “own” movement until release:
    return bMouseLatched;
}

bool WepView::OnMouseButtonDown(int32 Button, const FVector2D& ScreenPos)
{
    if (!bShownOverlay)
        return false;

    // Left mouse only:
    if (Button != 0)
        return false;

    const int32 X = (int32)ScreenPos.X;
    const int32 Y = (int32)ScreenPos.Y;

    // Only latch if pressed inside overlay:
    if (!GetOverlayRect().Contains(X, Y))
        return false;

    mouse_down = true;
    MouseDownPos = ScreenPos;

    bMouseLatched = true;

    HandlePressAt(X, Y);
    return true;
}

bool WepView::OnMouseButtonUp(int32 Button, const FVector2D& ScreenPos)
{
    if (!bShownOverlay)
        return false;

    if (Button != 0)
        return false;

    const int32 X = (int32)ScreenPos.X;
    const int32 Y = (int32)ScreenPos.Y;

    const bool bWasLatched = bMouseLatched;

    mouse_down = false;
    bMouseLatched = false;

    HandleReleaseAt(X, Y);

    // If we had the latch, we handled it:
    return bWasLatched;
}

// ------------------------------------------------------------
// Click logic
// ------------------------------------------------------------

void WepView::HandlePressAt(int32 X, int32 Y)
{
    if (!ship)
        return;

    int32 max_wep = (int32)ship->GetWeapons().size();
    if (max_wep > MAX_WEP)
        max_wep = MAX_WEP;

    for (int32 i = 0; i < max_wep; i++) {
        const int32 index = i * 4;

        if (CheckButton(index + 0, X, Y)) {
            ship->FireWeapon(i);
            return;
        }
        else if (CheckButton(index + 1, X, Y)) {
            ship->GetWeapons()[i]->SetFiringOrders(WeaponsOrders::MANUAL);
            return;
        }
        else if (CheckButton(index + 2, X, Y)) {
            ship->GetWeapons()[i]->SetFiringOrders(WeaponsOrders::AUTO);
            return;
        }
        else if (CheckButton(index + 3, X, Y)) {
            ship->GetWeapons()[i]->SetFiringOrders(WeaponsOrders::POINT_DEFENSE);
            return;
        }
    }
}

void WepView::HandleReleaseAt(int32 X, int32 Y)
{
    if (!ship || !ship->GetTarget())
        return;

    Rect r(rect.w / 2 + 50, 70, 20, 20);
    if (r.Contains(X, Y)) {
        CycleSubTarget(-1);
        return;
    }

    r.x = rect.w / 2 + 180;
    if (r.Contains(X, Y)) {
        CycleSubTarget(1);
        return;
    }
}

void WepView::SetOverlayMode(int InMode)
{
    // Clamp to valid legacy range
    // 0 = hidden
    // 1 = compact
    // 2 = expanded
    if (InMode < 0)
        InMode = 0;
    else if (InMode > 2)
        InMode = 2;

    if (mode == InMode)
        return;

    mode = InMode;

    // Force layout / redraw update
    OnWindowMove();
}

// ------------------------------------------------------------
// Button layout + hit test
// ------------------------------------------------------------

Rect WepView::GetOverlayRect() const
{
    // Matches your legacy “centered 512px wide” test with a top band.
    // Tune these if your HUD scaling differs.
    const int32 OverlayW = 512;
    const int32 OverlayH = 90;

    return Rect((rect.w / 2) - (OverlayW / 2), 0, OverlayW, OverlayH);
}

Rect WepView::GetButtonRect(int32 ButtonIndex) const
{
    // ButtonIndex maps: weapon i => base = i*4:
    //  base+0 FIRE, base+1 MAN, base+2 AUTO, base+3 DEF
    const int32 weaponIndex = ButtonIndex / 4;
    const int32 which = ButtonIndex % 4;

    const Rect ovr = GetOverlayRect();

    // Row baseline:
    const int32 rowY = ovr.y + 20 + weaponIndex * 18;

    // Right-side buttons:
    const int32 btnW = 48;
    const int32 btnH = 16;

    // Start near the right edge of overlay:
    const int32 rightEdge = ovr.x + ovr.w - 10;

    // 4 buttons packed right-to-left:
    const int32 x3 = rightEdge - btnW;
    const int32 x2 = x3 - 6 - btnW;
    const int32 x1 = x2 - 6 - btnW;
    const int32 x0 = x1 - 6 - btnW;

    int32 bx = x0;
    switch (which) {
    case 0: bx = x0; break;
    case 1: bx = x1; break;
    case 2: bx = x2; break;
    case 3: bx = x3; break;
    }

    return Rect(bx, rowY, btnW, btnH);
}

bool WepView::CheckButton(int32 ButtonIndex, int32 X, int32 Y) const
{
    return GetButtonRect(ButtonIndex).Contains(X, Y) != 0;
}

// ------------------------------------------------------------
// Sub-target cycling (simulation responsibility)
// ------------------------------------------------------------

void
WepView::CycleSubTarget(int direction)
{
    if (ship->GetTarget() == 0 || ship->GetTarget()->Type() != SimObject::SIM_SHIP)
        return;

    ship->CycleSubTarget(direction);
}
