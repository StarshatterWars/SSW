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


#pragma once

#include "Types.h"
#include "View.h"
#include "SimObject.h"
#include "GameStructs.h"

// Forward decls (keep header light):
class Ship;
class HUDView;
class Menu;
class Sim;
class QuantumDrive;
class SimRegion;
class StarSystem;

class QuantumView : public View, public SimObserver
{
public:
    // UE port style ctor (matches your View.h):
    QuantumView(View* InParent, class ActiveWindow* InActiveWindow);
    virtual ~QuantumView();

    // View hooks:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;
    virtual void ExecFrame() override;

    // Menu controls:
    virtual Menu* GetQuantumMenu(Ship* InShip);
    virtual bool  IsMenuShown() const;
    virtual void  ShowMenu();
    virtual void  CloseMenu();

    // SimObserver:
    virtual bool        Update(SimObject* Obj) override;
    virtual const char* GetObserverName() const override;

    // Static color sync:
    void   SetColor(const FColor& InColor);

    // Lifecycle:
    static void        Initialize();
    static void        Close();
    static QuantumView* GetInstance() { return QuantumViewInstance; }

protected:
    int32   WidthPx = 0;
    int32   HeightPx = 0;
    double  XCenter = 0.0;
    double  YCenter = 0.0;

    SystemFont* HudFont = nullptr;
    Sim* SimPtr = nullptr;
    Ship* ShipPtr = nullptr;

    static QuantumView* QuantumViewInstance;
};
