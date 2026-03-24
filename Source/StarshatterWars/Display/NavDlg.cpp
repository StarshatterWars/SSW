/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright © 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         NavDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    UNREAL PORT:
    - Converted from FormWindow/AWEvent mapping to UBaseScreen (UUserWidget-derived).
    - Preserves original member names and intent where applicable.
    - Replaces Print-style debugging with UE_LOG.
    - Removes MemDebug and allocation tags.
*/

#include "NavDlg.h"

#include "GameStructs.h"

// Unreal:
#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"

// Starshatter (ported core/gameplay):
#include "MapView.h"
#include "MissionElementDlg.h"
#include "BaseScreen.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "Galaxy.h"
#include "StarSystem.h"
#include "OrbitalBody.h"
#include "Orbital.h"
#include "Instruction.h"
#include "NavSystem.h"
#include "FormatUtil.h"
#include "Campaign.h"
#include "SimContact.h"
#include "Mission.h"

#include "Game.h"
#include "Keyboard.h"
#include "Mouse.h"

// +--------------------------------------------------------------------+

static const char* filter_name[] = {
    "SYSTEM",   "PLANET",
    "SECTOR",   "STATION",
    "STARSHIP", "FIGHTER"
};

// commit/cancel colors (ported to UE FColor):
static const FColor commit_color(53, 159, 67);
static const FColor cancel_color(160, 8, 8);

// Supported Selection Modes:
static const int SELECT_NONE = -1;
static const int SELECT_SYSTEM = 0;
static const int SELECT_PLANET = 1;
static const int SELECT_REGION = 2;
static const int SELECT_STATION = 3;
static const int SELECT_STARSHIP = 4;
static const int SELECT_FIGHTER = 5;

static const int VIEW_GALAXY = 0;
static const int VIEW_SYSTEM = 1;
static const int VIEW_REGION = 2;

// +--------------------------------------------------------------------+

UNavDlg::UNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // All state is initialized in-header (recommended for UUserWidget ports).
}

void
UNavDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    RegisterControls();
}

void
UNavDlg::NativeConstruct()
{
    Super::NativeConstruct();
}

void
UNavDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    ExecFrame();
}

// +--------------------------------------------------------------------+

void
UNavDlg::RegisterControls()
{
    // NOTE:
    // This port assumes you have named widgets in the UMG designer that match the
    // UPROPERTY BindWidgetOptional names in NavDlg.h:
    //
    // view_btn_0, view_btn_1, view_btn_2
    // filter_btn_0 ... filter_btn_5
    // commit_btn, zoom_in_btn, zoom_out_btn, close_btn
    // map_win, loc_labels, dst_labels, loc_data, dst_data
    // seln_list, info_list
    //
    // Starshatter MapView originally wrapped an ActiveWindow. In UE, MapView remains a
    // ported class; here we retain the pointer and initialize if the panel exists.

    if (map_win) {
        // MapView is a ported Starshatter class (non-UObject).
        // In the classic code it is created with allocation tags; those are removed.
        star_map = new MapView(nullptr, nullptr);

        // If your MapView can accept a UE widget/panel, update MapView to take a UCanvasPanel*
        // or a lightweight shim. For now, we keep construction minimal and rely on MapView
        // being able to operate without immediate UI binding.
    }

    // Bind buttons:
    if (view_btn_0) view_btn_0->OnClicked.AddDynamic(this, &UNavDlg::OnView);
    if (view_btn_1) view_btn_1->OnClicked.AddDynamic(this, &UNavDlg::OnView);
    if (view_btn_2) view_btn_2->OnClicked.AddDynamic(this, &UNavDlg::OnView);

    if (close_btn)    close_btn->OnClicked.AddDynamic(this, &UNavDlg::OnClose);
    if (commit_btn)   commit_btn->OnClicked.AddDynamic(this, &UNavDlg::OnEngage);

    if (filter_btn_0) filter_btn_0->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);
    if (filter_btn_1) filter_btn_1->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);
    if (filter_btn_2) filter_btn_2->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);
    if (filter_btn_3) filter_btn_3->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);
    if (filter_btn_4) filter_btn_4->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);
    if (filter_btn_5) filter_btn_5->OnClicked.AddDynamic(this, &UNavDlg::OnFilter);

    // ListView selection:
    // UListView selection binding differs from ListBox; you will likely drive selection via
    // OnItemSelectionChanged and your entry object types. For now, we keep the call surface
    // and rely on CoordinateSelection/SelectObject being invoked by your list item logic.

    if (star_map) {
        star_map->SetViewMode(VIEW_SYSTEM);
        // Button state visuals are handled by UMG styles, not SetButtonState.
        star_map->SetSelectionMode(2);
    }

    UpdateSelection();
}

// +--------------------------------------------------------------------+

void
UNavDlg::SetSystem(StarSystem* s)
{
    if (star_system == s)
        return;

    star_system = s;

    if (star_map) {
        Campaign* c = Campaign::GetCampaign();
        Sim* sim = Sim::GetSim();

        if (sim && sim->GetSystemList().size()) {
            star_map->SetGalaxy(sim->GetSystemList());
        }
        else if (mission && mission->GetSystemList().size()) {
            star_map->SetGalaxy(mission->GetSystemList());
        }
        else if (c && c->GetSystemList().size()) {
            star_map->SetGalaxy(c->GetSystemList());
        }
        else {
            Galaxy* g = Galaxy::GetInstance();
            if (g)
                star_map->SetGalaxy(g->GetSystemList());
        }

        star_map->SetSystem(s);
    }

    // flush old object pointers:
    stars.clear();
    planets.clear();
    regions.clear();
    contacts.clear();

    if (star_system) {
        // insert objects from star system:
        ListIter<OrbitalBody> star = star_system->Bodies();
        while (++star) {
            switch (star->Type()) {
            case Orbital::STAR:
                stars.append(star.value());
                break;
            case Orbital::PLANET:
            case Orbital::MOON:
                planets.append(star.value());
                break;
            }

            ListIter<OrbitalBody> planet = star->Satellites();
            while (++planet) {
                planets.append(planet.value());

                ListIter<OrbitalBody> moon = planet->Satellites();
                while (++moon) {
                    planets.append(moon.value());
                }
            }
        }

        ListIter<OrbitalRegion> rgn = star_system->AllRegions();
        while (++rgn)
            regions.append(rgn.value());
    }

    // sort region list by distance from the star:
    regions.sort();
}

// +--------------------------------------------------------------------+

void
UNavDlg::SetShip(Ship* s)
{
    if (ship == s)
        return;

    ship = s;

    if (ship)
        SetSystem(ship->GetRegion()->GetSystem());

    if (star_map) {
        Sim* sim = Sim::GetSim();

        if (sim && sim->GetSystemList().size())
            star_map->SetGalaxy(sim->GetSystemList());

        star_map->SetShip(ship);

        if (ship) {
            star_map->SetRegion(ship->GetRegion()->GetOrbitalRegion());
            UseViewMode(VIEW_REGION);
            star_map->SetSelectedShip(ship);
        }
    }

    // Button state visuals handled in UMG; selection mode still driven:
    UseFilter(SELECT_STARSHIP);
    UpdateSelection();
}

// +--------------------------------------------------------------------+

void
UNavDlg::SetMission(Mission* m)
{
    if (!m && mission == m)
        return;

    if (mission == m && m && star_system == m->GetStarSystem())
        return;

    mission = m;

    if (mission) {
        SetSystem(mission->GetStarSystem());
    }

    if (star_map) {
        Campaign* c = Campaign::GetCampaign();
        Sim* sim = Sim::GetSim();

        star_map->SetMission(0); // prevent building map menu twice

        if (sim && sim->GetSystemList().size()) {
            star_map->SetGalaxy(sim->GetSystemList());
        }
        else if (mission && mission->GetSystemList().size()) {
            star_map->SetGalaxy(mission->GetSystemList());
        }
        else if (c && c->GetSystemList().size()) {
            star_map->SetGalaxy(c->GetSystemList());
        }
        else {
            Galaxy* g = Galaxy::GetInstance();
            if (g)
                star_map->SetGalaxy(g->GetSystemList());
        }

        if (mission) {
            star_map->SetMission(mission);
            star_map->SetRegionByName(mission->GetRegion());

            if (star_map->GetViewMode() == VIEW_REGION) {
                ListIter<MissionElement> elem = mission->GetElements();
                while (++elem) {
                    MissionElement* e = elem.value();

                    if (e->IsPlayer())
                        star_map->SetSelectedElem(e);
                }
            }
        }
    }

    bool updated = false;

    if (mission) {
        Orbital* rgn = 0;
        rgn = mission->GetStarSystem()->FindOrbital(mission->GetRegion());

        if (rgn) {
            SelectRegion((OrbitalRegion*)rgn);
            updated = true;
        }
    }

    if (!updated)
        UpdateSelection();
}

// +--------------------------------------------------------------------+

void
UNavDlg::SetEditorMode(bool e)
{
    editor = e;

    if (star_map)
        star_map->SetEditorMode(editor);
}

// +--------------------------------------------------------------------+

void UNavDlg::ExecFrame()
{
    Sim* sim = Sim::GetSim();

    if (ship && star_system && sim)
    {
        // Build numeric strings:
        char x[16];
        char y[16];
        char z[16];
        char d[16];

        // Classic coordinate swap:
        // x = -loc.x, y = loc.z, z = loc.y
        const FVector Loc = ship->Location();
        FormatNumber(x, -Loc.X);
        FormatNumber(y, Loc.Z);
        FormatNumber(z, Loc.Y);

        // Commit button behavior (UMG visuals handled elsewhere):
        if (commit_btn)
        {
            NavSystem* navsys = ship->GetNavSystem();

            if (ship->GetNextNavPoint() == nullptr || !navsys)
            {
                // Disable commit
            }
            else
            {
                if (navsys->AutoNavEngaged())
                {
                    // cancel state
                }
                else
                {
                    // commit state
                }
            }
        }

        // Destination block (ported):
        Instruction* navpt = ship->GetNextNavPoint();
        if (navpt && navpt->Region())
        {
            const FVector NavLoc = navpt->Location();
            FormatNumber(x, NavLoc.X);
            FormatNumber(y, NavLoc.Y);
            FormatNumber(z, NavLoc.Z);

            FVector Npt = navpt->Region()->GetLocation() + NavLoc;

            if (sim->GetActiveRegion())
                Npt -= sim->GetActiveRegion()->GetLocation();

            // If you have a handedness helper, apply it here:
            // Npt = OtherHand(Npt);

            const double distance = FVector::Dist(Npt, ship->Location());
            FormatNumber(d, distance);

            UE_LOG(LogTemp, Verbose, TEXT("NavDlg DST: %s %s %s dist=%s"),
                UTF8_TO_TCHAR(x), UTF8_TO_TCHAR(y), UTF8_TO_TCHAR(z), UTF8_TO_TCHAR(d));
        }
    }

    UpdateSelection();
    UpdateLists();

    // Zoom controls:
    if (star_map)
    {
        if (Keyboard::KeyDown(VK_ADD) || (zoom_in_btn /* && pressed state via UMG */))
        {
            star_map->ZoomIn();
        }
        else if (Keyboard::KeyDown(VK_SUBTRACT) || (zoom_out_btn /* && pressed state via UMG */))
        {
            star_map->ZoomOut();
        }
        else if (star_map->TargetRect().Contains(Mouse::X(), Mouse::Y()))
        {
            if (Mouse::Wheel() > 0)
            {
                star_map->ZoomIn();
                star_map->ZoomIn();
                star_map->ZoomIn();
            }
            else if (Mouse::Wheel() < 0)
            {
                star_map->ZoomOut();
                star_map->ZoomOut();
                star_map->ZoomOut();
            }
        }
    }

    // Cursor:
    if (nav_edit_mode == NAV_EDIT_NONE)
        Mouse::SetCursor(Mouse::ARROW);
    else
        Mouse::SetCursor(Mouse::CROSS);
}

// +--------------------------------------------------------------------+
// Button handlers
// +--------------------------------------------------------------------+

void
UNavDlg::OnView()
{
    if (!star_map)
        return;

    int use_filter_mode = -1;

    // Determine which button is currently pressed/focused.
    // In UE, OnClicked does not tell you the sender; use separate handlers per button
    // or bind lambdas. For this generic port, we default to VIEW_SYSTEM behavior.
    //
    // Recommended: create OnViewGalaxy/OnViewSystem/OnViewRegion and bind each directly.

    star_map->SetViewMode(VIEW_SYSTEM);
    use_filter_mode = SELECT_REGION;

    if (use_filter_mode >= 0) {
        UseFilter(use_filter_mode);
    }
}

void
UNavDlg::OnFilter()
{
    // Same limitation as OnView: OnClicked doesn't supply sender.
    // Recommended: create OnFilterSystem/OnFilterPlanet/... and bind individually.
    //
    // Default to SELECT_REGION for safe behavior:
    UseFilter(SELECT_REGION);
}

void
UNavDlg::OnSelect()
{
    // ListView selection is driven by item selection callbacks.
    // Implement OnItemSelectionChanged in your UListView entry object pipeline.
}

void
UNavDlg::OnCommit()
{
    // In classic: manager->ShowNavDlg() toggled visibility.
    // In UE: call into your Menu/Nav manager widget to hide.
    // manager is UObject*; cast to your concrete manager when ready.
}

void
UNavDlg::OnCancel()
{
    // Same as OnCommit.
}

void
UNavDlg::OnEngage()
{
    bool hide = false;

    if (ship) {
        NavSystem* navsys = ship->GetNavSystem();
        if (navsys) {
            if (navsys->AutoNavEngaged()) {
                navsys->DisengageAutoNav();
            }
            else {
                navsys->EngageAutoNav();
                hide = true;
            }

            Sim* sim = Sim::GetSim();
            if (sim)
                ship->SetControls(sim->GetControls());
        }
    }

    if (manager && hide) {
        // manager->ShowNavDlg(); // also hides in classic
        UE_LOG(LogTemp, Verbose, TEXT("NavDlg: EngageAutoNav -> hide requested"));
    }
}

void
UNavDlg::OnMapDown()
{
}

void
UNavDlg::OnMapMove()
{
}

void
UNavDlg::OnMapClick()
{
    static uint32 click_time = 0;

    if (!star_map)
        return;

    SetSystem(star_map->GetSystem());
    CoordinateSelection();

    // double-click:
    if (Game::RealTime() - click_time < 350) {
        MissionElement* elem = star_map->GetSelectedElem();

        // manager->GetMsnElemDlg() in classic. Here, manager is UObject*.
        // Implement your manager cast and dialog show logic when your UI stack is ready.
        if (elem) {
            UE_LOG(LogTemp, Verbose, TEXT("NavDlg: double-click MissionElement"));
        }
    }

    click_time = Game::RealTime();
}

void
UNavDlg::OnClose()
{
    if (manager) {
        // manager->HideNavDlg();
        UE_LOG(LogTemp, Verbose, TEXT("NavDlg: Close"));
    }
}

// +--------------------------------------------------------------------+

void
UNavDlg::UseFilter(int filter_index)
{
    seln_mode = filter_index;

    if (star_map)
        star_map->SetSelectionMode(seln_mode);

    UpdateSelection();
    UpdateLists();
}

void
UNavDlg::UseViewMode(int mode)
{
    if (!star_map)
        return;

    if (mode >= 0 && mode < 3) {
        int use_filter_mode = -1;

        if (mode == VIEW_GALAXY) {
            star_map->SetViewMode(VIEW_GALAXY);
            use_filter_mode = SELECT_SYSTEM;
        }
        else if (mode == VIEW_SYSTEM) {
            star_map->SetViewMode(VIEW_SYSTEM);
            use_filter_mode = SELECT_REGION;
        }
        else if (mode == VIEW_REGION) {
            star_map->SetViewMode(VIEW_REGION);
            use_filter_mode = SELECT_STARSHIP;
        }

        if (use_filter_mode >= 0) {
            UseFilter(use_filter_mode);
        }
    }
}

void
UNavDlg::SelectStar(Orbital* star)
{
    UseViewMode(VIEW_GALAXY);

    if (stars.size()) {
        int sel = 0;

        ListIter<Orbital> iter = stars;
        while (++iter) {
            if (iter.value() == star) {
                int old_seln_mode = seln_mode;
                UseFilter(SELECT_SYSTEM);
                SelectObject(sel);
                UseFilter(old_seln_mode);
                return;
            }

            sel++;
        }
    }
}

void
UNavDlg::SelectPlanet(Orbital* planet)
{
    UseViewMode(VIEW_SYSTEM);

    if (planets.size()) {
        int sel = 0;

        ListIter<Orbital> iter = planets;
        while (++iter) {
            if (iter.value() == planet) {
                int old_seln_mode = seln_mode;
                UseFilter(SELECT_PLANET);
                SelectObject(sel);
                UseFilter(old_seln_mode);
                return;
            }

            sel++;
        }
    }
}

void
UNavDlg::SelectRegion(OrbitalRegion* rgn)
{
    UseViewMode(VIEW_REGION);

    if (regions.size()) {
        int sel = 0;

        ListIter<OrbitalRegion> iter = regions;
        while (++iter) {
            if (iter.value() == rgn) {
                int old_seln_mode = seln_mode;
                UseFilter(SELECT_REGION);
                SelectObject(sel);
                UseFilter(old_seln_mode);
                return;
            }

            sel++;
        }
    }
}

// +--------------------------------------------------------------------+

void
UNavDlg::SelectObject(int index)
{
    // ListBox -> ListView port:
    // Implement based on your entry object and selection model.
    // This stub preserves API surface.
    if (!star_map) {
        UE_LOG(LogTemp, Warning, TEXT("NavDlg::SelectObject called without star_map"));
        return;
    }

    // In classic:
    // Text selected = seln_list->GetItemText(index);
    // int  selection = seln_list->GetItemData(index);
    // star_map->SetSelection(selection);
    //
    // In UE, store selection identity in your UListView item UObject.
    UE_LOG(LogTemp, Verbose, TEXT("NavDlg::SelectObject index=%d (ListView port TODO)"), index);
}

void
UNavDlg::UpdateSelection()
{
    if (!info_list || !star_map)
        return;

    // UListView content population is via SetListItems / AddItem with UObject items.
    // The classic ListBox wrote a two-column table. In UE, represent rows as UObjects
    // with label/value, and render via an entry widget.
    //
    // This method remains as the logical source of truth; implement once your list item
    // object type exists.
}

void
UNavDlg::UpdateLists()
{
    if (!seln_list)
        return;

    // Same note: populate UListView with UObject items instead of raw text.
    // This is intentionally left as a porting seam.
}

void
UNavDlg::CoordinateSelection()
{
    if (!seln_list || !star_map)
        return;

    // Coordinate UListView selection with MapView selection.
    // Implement via item iteration and SetSelectedItem when your item model is defined.
}

void
UNavDlg::SetNavEditMode(int mode)
{
    if (nav_edit_mode != mode) {
        if (star_map) {
            if (mode != NAV_EDIT_NONE) {
                int map_mode = star_map->GetSelectionMode();
                if (map_mode != -1)
                    seln_mode = map_mode;

                star_map->SetSelectionMode(-1);
            }
            else {
                star_map->SetSelectionMode(seln_mode);
            }
        }

        nav_edit_mode = mode;
    }
}
