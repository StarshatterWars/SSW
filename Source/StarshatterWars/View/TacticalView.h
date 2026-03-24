/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    StarshatterWars
    FILE:         TacticalView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    TacticalView (UE port, plain C++)
    - Tactical data readout / selection overlay
    - Ported from Starshatter 5.0 TacticalView
    - Uses UE-friendly primitives (FVector/FColor/FString)
    - No Window.h dependency (View carries render context)
*/

#pragma once

#include "CoreMinimal.h"
#include "List.h"
#include "View.h"
#include "SimObject.h"
#include "GameStructs.h"

// Forward declarations (ported sim/UI types):
class Ship;
class SimContact;
class Instruction;
class Sim;
class SimRegion;
class SimElement;

class RadioMessage;
class RadioTraffic;

class UGameScreen;

class Menu;
class MenuItem;
class MenuView;
class SimProjector;

class TacticalView : public View, public SimObserver
{
public:
    TacticalView(View* InParent, UGameScreen* InParentScreen);
    virtual ~TacticalView();

    // View overrides:
    virtual void Refresh() override;
    virtual void OnWindowMove() override;
    void ExecFrame();

    // Wiring:
    virtual void UseProjector(SimProjector* InProjector);

    // Input:
    virtual void DoMouseFrame();

    // SimObserver:
    virtual bool        Update(SimObject* Obj) override;
    virtual const char* GetObserverName() const override;

    // Global:
    static void Initialize();
    static void Close();
    static TacticalView* GetInstance() { return TacView; }

    static void SetColor(const FColor& InHudColor, const FColor& InTextColor);

protected:
    // Selection:
    virtual bool  SelectAt(int32 X, int32 Y);
    virtual bool  SelectRect(const Rect& R);
    virtual Ship* WillSelectAt(int32 X, int32 Y);
    virtual void  SetHelm(bool bApproach);

    // Drawing:
    virtual void DrawMouseRect();
    virtual void DrawSelection(Ship* SelectedShip);
    virtual void DrawSelectionInfo(Ship* SelectedShip);
    virtual void DrawSelectionList(ListIter<Ship>& SelectionIter);

    // Menu:
    virtual void BuildMenu();
    virtual void DrawMenu();
    virtual void ProcessMenuItem(int32 action);

    // Move/Action:
    virtual void DrawMove();
    virtual void SendMove();
    virtual bool GetMouseLoc3D();

    virtual void DrawAction();
    virtual void SendAction();

protected:
    // Parent:
    UGameScreen* GameParent = nullptr;

    // Projector:
    SimProjector* ProjectorPtr = nullptr;

    // Cached geometry:
    int32   WidthPx = 0;
    int32   HeightPx = 0;
    double  XCenter = 0.0;
    double  YCenter = 0.0;

    // Input state:
    int32   bShiftDown = 0;
    int32   bMouseDown = 0;
    int32   bRightDown = 0;
    int32   bShowMove = 0;
    RadioMessageAction ShowAction = RadioMessageAction::NONE;

    // Move state:
    FVector MoveLoc = FVector::ZeroVector;
    double  BaseAlt = 0.0;
    double  MoveAlt = 0.0;

    // Mouse points/rect:
    FIntPoint MouseAction = FIntPoint::ZeroValue;
    FIntPoint MouseStart = FIntPoint::ZeroValue;
    Rect      MouseRect;

    // Sim pointers:
    Sim* SimPtr = nullptr;

    Ship* PlayerShip = nullptr;
    Ship* MsgShip = nullptr;

    FString CurrentSector;

    // Menu:
    Menu* ActiveMenu = nullptr;
    MenuView* MenuViewPtr = nullptr;
    MenuItem* MenuItemPtr = nullptr;

protected:
   
   static TacticalView* TacView;
   UGameScreen* gamescreen;
   MenuView*    menu_view;

    // Colors (legacy had static Color hud_color/txt_color):
    static FColor HudColor;
    static FColor TxtColor;

    int                        show_move;
    RadioMessageAction         ShowRadioAction;

    double      base_alt;
    double      move_alt;

    Sim*        sim;
    Ship*       msg_ship;
    Ship*       ship;


private:
    TacticalView(const TacticalView&) = delete;
    TacticalView& operator=(const TacticalView&) = delete;

    MenuItem* MI;

    void ProcessRadioAction(RadioMessageAction action);
    void ProcessViewAction(TacticalViewMenu action);
};
