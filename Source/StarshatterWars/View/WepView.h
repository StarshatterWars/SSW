/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         WepView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Weapons Overlay View (Unreal-friendly port)
    - Plain C++ (NOT a UObject)
    - Uses View (combined Window+View layer) for drawing and input routing
    - Mouse handling via View::OnMouseButtonDown/Up/Move
    - Minimal dependencies; simulation logic stays in Ship/SimObject
*/

#pragma once

#include "CoreMinimal.h"
#include "View.h"
#include "Geometry.h" // Rect
#include "Math/Vector2D.h"
#include "Math/Color.h"

class Sim;
class Ship;
class HUDView;
class Weapon;
class SimObject;

class WepView : public View
{
public:
    static const char* TYPENAME() { return "WepView"; }

    // NOTE:
    // Window.h is gone in your port, so this view is constructed as a child of another View.
    // Typically you create it under your game root view / screen view.
    WepView(View* InParent, int32 InWidth, int32 InHeight);
    virtual ~WepView();

    // Operations:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;
    virtual void ExecFrame() override;

    // Input (UE-routed via your View system):
    virtual bool OnMouseButtonDown(int32 Button, const FVector2D& ScreenPos) override;
    virtual bool OnMouseButtonUp(int32 Button, const FVector2D& ScreenPos) override;
    virtual bool OnMouseMove(const FVector2D& ScreenPos) override;

    // Legacy-ish UI:
    void ShowOverlay(bool bShow) { bShownOverlay = bShow; }
    bool IsOverlayShown() const { return bShownOverlay; }
    void CycleOverlayMode();
    void SetOverlayMode(int InMode);
    int  GetOverlayMode() const { return mode; }

    // Color handling:
    void SetColor(FColor c);

    // Mouse latch (so MapView, HUDView, etc can query):
    static bool IsMouseLatched() { return bMouseLatched; }

    // Singleton-ish helpers (matches how you’ve been doing QuitView/QuantumView):
    static void     Initialize(View* Parent, int32 W, int32 H);
    static void     Close();
    static WepView* GetInstance() { return wep_view; }

protected:
    // Interaction helpers:
    void HandlePressAt(int32 X, int32 Y);
    void HandleReleaseAt(int32 X, int32 Y);

    bool CheckButton(int32 ButtonIndex, int32 X, int32 Y) const;
    void CycleSubTarget(int32 Direction);

    // Layout helpers:
    Rect GetOverlayRect() const;
    Rect GetButtonRect(int32 ButtonIndex) const;

protected:
    // Cached geometry:
    int32 width = 0;
    int32 height = 0;

    // Sim pointers:
    Sim* sim = nullptr;
    Ship* ship = nullptr;

    // State:
    bool  bShownOverlay = true;
    bool  mouse_in = false;
    bool  mouse_down = false;

    FVector2D MouseDownPos = FVector2D::ZeroVector;

    // If you need these later:
    bool transition = false;
    int32 mode = 0;

    // HUD colors:
    FColor LocalHudColor = FColor::Black;
    FColor LocalTextColor = FColor::White;

    // Constants:
    static constexpr int32 MAX_WEP = 16; // legacy-ish cap; tune if your ships carry more

    // Singleton:
    static WepView* wep_view;

    // Global latch:
    static bool bMouseLatched;
};
