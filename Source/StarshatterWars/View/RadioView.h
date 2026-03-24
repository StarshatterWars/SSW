/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    StarshatterWars
    FILE:         RadioView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    View class for Radio Communications HUD Overlay (UE port)
    - Plain C++ (NOT a UObject)
    - Inherits your combined View/Window layer (View.h)
    - Draws radio messages + context radio menu overlay
    - Thread-safe static message queue
*/

#pragma once

#include "Types.h"
#include "View.h"
#include "SimObject.h"
#include "Text.h"
#include "GameStructs.h"

#include "Math/Color.h"
#include "HAL/CriticalSection.h"

class SimElement;
class Ship;
class RadioMessage;
class Menu;
class MenuItem;
class Sim;

// legacy helper (you already have it in Starshatter):
class MenuHistory;

class RadioView : public View, public SimObserver
{
public:
    // Ported ctor style: parent-based View
    RadioView(View* InParent, int ax = 0, int ay = 0, int aw = 0, int ah = 0);
    virtual ~RadioView();

    // Operations:
    virtual void      Refresh() override;
    virtual void      OnWindowMove() override;
    virtual void      ExecFrame() override;

    virtual Menu* GetRadioMenu(Ship* ship);
    virtual bool      IsMenuShown();
    virtual void      ShowMenu();
    virtual void      CloseMenu();

    static void       Message(const char* msg);
    static void       ClearMessages();

    virtual bool         Update(SimObject* obj) override;
    virtual const char* GetObserverName() const override;

    static void       SetColor(FColor c);

    static void       Initialize();
    static void       Close();

    static RadioView* GetInstance() { return radio_view; }

protected:
    void              SendRadioMessage(Ship* ship, MenuItem* item);

protected:
    int         width = 0;
    int         height = 0;
    double      xcenter = 0.0;
    double      ycenter = 0.0;

    Sim* sim = nullptr;
    Ship* ship = nullptr;
    SimElement* dst_elem = nullptr;

    enum { MAX_MSG = 6 };
    Text        msg_text[MAX_MSG];
    double      msg_time[MAX_MSG];

    static RadioView* radio_view;
    static FCriticalSection sync;
};
