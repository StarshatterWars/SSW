/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe (legacy) -> Starshatter Wars (UE port)
    FILE:         MapView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Star Map class (UE port, plain C++ class)
    - Keeps Starshatter core container/string types (Text, List, etc.)
    - Uses Unreal math types for vectors (FVector) and colors (FColor)
*/

#pragma once

#include "Math/Vector.h"               // FVector
#include "Math/Color.h"                // FColor
#include "Math/UnrealMathUtility.h"    // Math

// Starshatter core includes (match legacy surface area):
#include "Types.h"
#include "SimObject.h"
#include "View.h"
#include "EventTarget.h"
#include "Bitmap.h"
#include "List.h"
#include "Text.h"
#include "GameStructs.h" 

// Forward declarations (aggressive):
class ActiveWindow;
class StarSystem;
class Orbital;
class OrbitalRegion;
class Ship;
class Instruction;
class Mission;
class MissionElement;
class Campaign;
class Combatant;
class CombatGroup;
class Menu;
class MenuItem;
class MenuView;
class SystemFont; // was Font



// +--------------------------------------------------------------------+

const int EID_MAP_CLICK = 1000;

// +--------------------------------------------------------------------+

class MapView : public View, public EventTarget, public SimObserver
{
public:
    MapView(View* InParent, ActiveWindow* InActiveWindow);
    virtual ~MapView();

    // Operations:
    virtual void      Refresh() override;
    virtual void      OnWindowMove() override;
    virtual void      OnShow();
    virtual void      OnHide();

    virtual void      DrawTitle();
    virtual void      DrawGalaxy();
    virtual void      DrawSystem();
    virtual void      DrawRegion();

    virtual void      DrawGrid();
    virtual void      DrawOrbital(Orbital& orbital, int index);
    virtual void      DrawShip(Ship& ship, bool current = false, int rep = 3);
    virtual void      DrawElem(MissionElement& elem, bool current = false, int rep = 3);

    virtual void      DrawNavRoute(OrbitalRegion* rgn,
        List<Instruction>& route,
        FColor             smarker,
        Ship* ship = 0,
        MissionElement* elem = 0);

    virtual void      DrawCombatantSystem(Combatant* c, Orbital* rgn, int x, int y, int rr);
    virtual void      DrawCombatGroupSystem(CombatGroup* g, Orbital* rgn, int x1, int x2, int& y, int a);
    virtual void      DrawCombatGroup(CombatGroup* g, int rep = 3);

    virtual int       GetViewMode() const { return view_mode; }
    virtual void      SetViewMode(int mode);

    virtual void      SetSelectionMode(int mode);
    virtual int       GetSelectionMode() const { return seln_mode; }

    virtual void      SetSelection(int index);
    virtual void      SetSelectedShip(Ship* ship);
    virtual void      SetSelectedElem(MissionElement* elem);

    virtual void      SetRegion(OrbitalRegion* rgn);
    virtual void      SetRegionByName(const char* rgn_name);

    virtual void      SelectAt(int x, int y);

    virtual Orbital* GetSelection();
    virtual Ship* GetSelectedShip();
    virtual MissionElement* GetSelectedElem();
    virtual int             GetSelectionIndex();

    virtual void      SetShipFilter(uint32 f) { ship_filter = f; }

    // Event Target Interface:
    virtual bool      OnMouseMove(int32 x, int32 y) override;
    virtual int       OnLButtonDown(int x, int y) override;
    virtual int       OnLButtonUp(int x, int y) override;
    virtual int       OnClick() override;
    virtual int       OnRButtonDown(int x, int y) override;
    virtual int       OnRButtonUp(int x, int y) override;

    virtual bool      IsEnabled() const;
    virtual bool      IsVisible() const;
    virtual bool      IsFormActive() const;
    virtual Rect      TargetRect() const;

    void              ZoomIn();
    void              ZoomOut();

    void              SetGalaxy(List<StarSystem>& systems);
    void              SetSystem(StarSystem* s);
    void              SetMission(Mission* m);
    void              SetShip(Ship* s);
    void              SetCampaign(Campaign* c);

    // World visibility test (legacy Point -> FVector):
    bool              IsVisible(const FVector& loc);

    // Accessors:
    virtual void      GetClickLoc(double& x, double& y) { x = click_x; y = click_y; }
    List<StarSystem>& GetGalaxy() { return system_list; }
    StarSystem* GetSystem() const { return system; }
    OrbitalRegion* GetRegion() const;

    // SimObserver:
    virtual bool         Update(SimObject* obj) override;
    virtual const char* GetObserverName() const { return "MapWin"; }

    bool              GetEditorMode() const { return editor; }
    void              SetEditorMode(bool b) { editor = b; }

protected:
    virtual void      BuildMenu();
    virtual void      ClearMenu();
    virtual void      ProcessMenuItem(int action);

    virtual bool      SetCapture();
    virtual bool      ReleaseCapture();

    virtual void      DrawTabbedText(SystemFont* font, const char* text);

    bool              IsClutter(Ship& s);
    bool              IsCrowded(Ship& s);
    bool              IsCrowded(MissionElement& elem);

    // Legacy POINT -> FVector (pixel coords in X/Y):
    void              GetShipLoc(Ship& s, FVector& loc);
    void              GetElemLoc(MissionElement& s, FVector& loc);

    void              SelectShip(Ship* selship);
    void              SelectElem(MissionElement* selelem);
    void              SelectNavpt(Instruction* navpt);

    void              FindShips(bool friendly,
        bool station,
        bool starship,
        bool dropship,
        List<Text>& result);

    void              SetupScroll(Orbital* s);
    double            GetMinRadius(int type);

protected:
    Text              title;
    Rect              rect;

    Campaign* campaign = 0;
    Mission* mission = 0;

    List<StarSystem>  system_list;
    StarSystem* system = 0;

    List<Orbital>     stars;
    List<Orbital>     planets;
    List<Orbital>     regions;

    Ship* ship = 0;

    // Legacy stored this by value:
    Bitmap            galaxy_image;

    bool              editor = false;

    int               current_star = -1;
    int               current_planet = -1;
    int               current_region = -1;

    Ship* current_ship = 0;
    MissionElement* current_elem = 0;
    Instruction* current_navpt = 0;
    INSTRUCTION_STATUS  current_status = INSTRUCTION_STATUS::PENDING;

    int               view_mode = 0;
    int               seln_mode = 0;

    bool              captured = false;
    bool              dragging = false;
    bool              adding_navpt = false;
    bool              moving_navpt = false;
    bool              moving_elem = false;

    int               scrolling = 0;
    int               mouse_x = 0;
    int               mouse_y = 0;

    uint32            ship_filter = 0;

    double            zoom = 1.0;
    double            view_zoom[3] = { 1.0, 1.0, 1.0 };

    double            offset_x = 0.0;
    double            offset_y = 0.0;

    double            view_offset_x[3] = { 0.0, 0.0, 0.0 };
    double            view_offset_y[3] = { 0.0, 0.0, 0.0 };

    double            c = 0.0;
    double            r = 0.0;

    double            scroll_x = 0.0;
    double            scroll_y = 0.0;

    double            click_x = 0.0;
    double            click_y = 0.0;

    SystemFont* font = 0;
    SystemFont* title_font = 0;

    ActiveWindow* active_window = 0;
    Menu* active_menu = 0;

    Menu* map_menu = 0;
    Menu* map_system_menu = 0;
    Menu* map_sector_menu = 0;
    Menu* ship_menu = 0;
    Menu* nav_menu = 0;
    Menu* action_menu = 0;
    Menu* objective_menu = 0;
    Menu* formation_menu = 0;
    Menu* speed_menu = 0;
    Menu* hold_menu = 0;
    Menu* farcast_menu = 0;

    MenuView* menu_view = 0;
};
