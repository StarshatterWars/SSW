/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026.

	SUBSYSTEM:    Stars.exe
	FILE:         MapView.cpp
	AUTHOR:       John DiCamillo (original) / UE Port: Carlos Bott

	OVERVIEW
	========
	Star Map class (UE port)
*/

#include "MapView.h"

#include "Galaxy.h"
#include "StarSystem.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Instruction.h"
#include "SimElement.h"      
#include "NavAI.h"
#include "Weapon.h"
#include "Sim.h"
#include "Mission.h"
#include "Campaign.h"
#include "Combatant.h"
#include "CombatGroup.h"
#include "CombatUnit.h"
#include "SimContact.h"     
#include "MenuView.h"

#include "Game.h"
#include "DataLoader.h"
#include "EventDispatch.h"
#include "Video.h"
#include "UIButton.h"
#include "Bitmap.h"
#include "Mouse.h"
#include "FormatUtil.h"
#include "Menu.h"
#include "GameStructs.h"
#include "SystemFont.h"

#include "Math/Vector.h"   // FVector
#include "Math/Color.h"    // FColor

// +--------------------------------------------------------------------+
// Supported Selection Modes:

const int SELECT_NONE = -1;
const int SELECT_SYSTEM = 0;
const int SELECT_PLANET = 1;
const int SELECT_REGION = 2;
const int SELECT_STATION = 3;
const int SELECT_STARSHIP = 4;
const int SELECT_FIGHTER = 5;
const int SELECT_NAVPT = 6;

const int VIEW_GALAXY = 0;
const int VIEW_SYSTEM = 1;
const int VIEW_REGION = 2;

// +--------------------------------------------------------------------+

static FORCEINLINE FColor ShadeTint(const FColor& Tint, uint8 Shade)
{
	// Shade is 0..255-ish. Original code used (Blue/2), so often 0..127.
	const float S = (float)Shade / 255.0f;

	return FColor(
		(uint8)FMath::Clamp(FMath::RoundToInt(Tint.R * S), 0, 255),
		(uint8)FMath::Clamp(FMath::RoundToInt(Tint.G * S), 0, 255),
		(uint8)FMath::Clamp(FMath::RoundToInt(Tint.B * S), 0, 255),
		Tint.A
	);
}

FORCEINLINE FVector ToOtherHand(const FVector& V)
{
	return FVector(V.X, -V.Y, V.Z);
}

MapView::MapView(View* InParent, ActiveWindow* InActiveWindow)
	: View(InParent, 0, 0, InParent ? InParent->Width() : 0, InParent ? InParent->Height() : 0)
	, active_window(InActiveWindow)
	, system(nullptr), zoom(1.1), offset_x(0), offset_y(0), ship(nullptr), campaign(nullptr)
	, captured(false), dragging(false), adding_navpt(false)
	, moving_navpt(false), moving_elem(false)
	, view_mode(VIEW_SYSTEM), seln_mode(SELECT_REGION), ship_filter(0xffffffff)
	, current_star(0), current_planet(0), current_region(0)
	, current_ship(nullptr), current_elem(nullptr), current_navpt(nullptr), mission(nullptr)
	, scrolling(0), scroll_x(0), scroll_y(0), click_x(0), click_y(0)
	, active_menu(nullptr), map_menu(nullptr), map_system_menu(nullptr), map_sector_menu(nullptr)
	, ship_menu(nullptr), editor(false)
	, nav_menu(nullptr), action_menu(nullptr), objective_menu(nullptr), formation_menu(nullptr), speed_menu(nullptr)
	, hold_menu(nullptr), farcast_menu(nullptr), menu_view(nullptr)
{
	for (int i = 0; i < 3; i++) {
		view_zoom[i] = zoom;
		view_offset_x[i] = offset_x;
		view_offset_y[i] = offset_y;
	}

	menu_view = new MenuView(this);  

	if (active_window)
		active_window->AddView(this);
}
// +--------------------------------------------------------------------+

MapView::~MapView()
{
	ClearMenu();

	if (galaxy_image.Width() > 0)  // or galaxy_image.HasImage() / galaxy_image.IsValid()
	{
		galaxy_image.ClearImage();

		delete menu_view;
		menu_view = 0;

	}
}

// +--------------------------------------------------------------------+

void MapView::OnWindowMove()
{
	if (menu_view)
		menu_view->OnWindowMove();
}

// +--------------------------------------------------------------------+

void MapView::SetGalaxy(List<StarSystem>& g)
{
	system_list.clear();
	system_list.append(g);

	if (system_list.size() > 0) {
		SetSystem(system_list[0]);
	}
}

void MapView::SetSystem(StarSystem* s)
{
	if (system != s) {
		system = s;

		// forget invalid selection:
		current_star = 0;
		current_planet = 0;
		current_region = 0;
		current_ship = 0;
		current_elem = 0;
		current_navpt = 0;

		// flush old object pointers:
		stars.clear();
		planets.clear();
		regions.clear();

		// insert objects from star system:
		if (system) {
			ListIter<OrbitalBody> star = system->Bodies();
			while (++star) {
				switch (star->Type()) {
				case Orbital::STAR:       stars.append(star.value());
					break;
				case Orbital::PLANET:
				case Orbital::MOON:       planets.append(star.value());
					break;
				}
			}

			ListIter<OrbitalBody> planet = star->Satellites();
			while (++planet) {
				planets.append(planet.value());

				ListIter<OrbitalBody> moon = planet->Satellites();
				while (++moon) {
					planets.append(moon.value());
				}
			}

			ListIter<OrbitalRegion> rgn = system->AllRegions();
			while (++rgn)
				regions.append(rgn.value());

			// sort region list by distance from the star:
			regions.sort();
		}

		BuildMenu();
	}
}

// +--------------------------------------------------------------------+

void MapView::SetShip(Ship* s)
{
	if (ship != s) {
		ship = s;

		// forget invalid selection:
		current_star = 0;
		current_planet = 0;
		current_region = 0;
		current_ship = 0;
		current_elem = 0;
		current_navpt = 0;

		if (ship && system_list.size() > 0) {
			SimRegion* rgn = ship->GetRegion();

			if (rgn && rgn->GetSystem()) {
				system = 0;
				SetSystem(rgn->GetSystem());
			}
		}

		BuildMenu();
	}
}

// +--------------------------------------------------------------------+

void MapView::SetMission(Mission* m)
{
	if (mission != m) {
		mission = m;

		// forget invalid selection:
		current_star = 0;
		current_planet = 0;
		current_region = 0;
		current_ship = 0;
		current_elem = 0;
		current_navpt = 0;

		if (mission && system_list.size() > 0) {
			system = 0;
			SetSystem(mission->GetStarSystem());
		}

		BuildMenu();
	}
}

// +--------------------------------------------------------------------+

void MapView::SetCampaign(Campaign* InCampaign)
{
	if (campaign != InCampaign) {
		campaign = InCampaign;

		// forget invalid selection:
		current_star = 0;
		current_planet = 0;
		current_region = 0;
		current_ship = 0;
		current_elem = 0;
		current_navpt = 0;

		if (campaign) {
			SetGalaxy(campaign->GetSystemList());
		}
	}
}

// +--------------------------------------------------------------------+

enum MapView_MENU {
	MAP_SYSTEM = 1000,
	MAP_SECTOR = 2000,
	MAP_SHIP = 3000,
	MAP_NAV = 4000,
	MAP_ADDNAV = 4001,
	MAP_DELETE = 4002,
	MAP_CLEAR = 4003,
	MAP_ACTION = 5000,
	MAP_FORMATION = 6000,
	MAP_SPEED = 7000,
	MAP_HOLD = 8000,
	MAP_FARCAST = 8500,
	MAP_OBJECTIVE = 9000
};

void MapView::BuildMenu()
{
	ClearMenu();

	map_system_menu = new Menu("STARSYSTEM");

	if (system_list.size() > 0) {
		int i = 0;
		ListIter<StarSystem> iter = system_list;
		while (++iter) {
			StarSystem* s = iter.value();
			map_system_menu->AddItem(s->GetName(), MAP_SYSTEM + i);
			i++;
		}
	}
	else if (system) {
		map_system_menu->AddItem(system->GetName(), MAP_SYSTEM);
	}

	map_sector_menu = new Menu("SECTOR");
	for (int i = 0; i < regions.size(); i++) {
		Orbital* rgn = regions[i];
		map_sector_menu->AddItem(rgn->Name(), MAP_SECTOR + i);
	}

	map_menu = new Menu("MAP");
	map_menu->AddMenu("System", map_system_menu);
	map_menu->AddMenu("Sector", map_sector_menu);

	if (ship || mission) {
		ship_menu = new Menu("SHIP");
		ship_menu->AddMenu("Star System", map_system_menu);
		ship_menu->AddMenu("Sector", map_sector_menu);

		ship_menu->AddItem("", 0);
		ship_menu->AddItem("Add Navpoint", MAP_ADDNAV);
		ship_menu->AddItem("Clear All", MAP_CLEAR);

		action_menu = new Menu("ACTION");


		const UEnum* ActionEnum = StaticEnum<INSTRUCTION_ACTION>();

		for (int i = 0; i < (int)INSTRUCTION_ACTION::NUM_ACTIONS; i++)
		{
			const INSTRUCTION_ACTION Act = (INSTRUCTION_ACTION)i;

			FString Name = ActionEnum
				? ActionEnum->GetNameStringByValue((int64)Act)
				: FString::FromInt(i);

			// Optional prettify (VECTOR -> Vector, RTB stays RTB, etc.)
			Name = Name.Replace(TEXT("_"), TEXT(" "));
			Name = FText::FromString(Name).ToString(); // keeps it simple; you can do smarter title-casing if you want

			const FString Label = FString::Printf(TEXT("Action %s"), *Name);
			action_menu->AddItem(Label, MAP_ACTION + i);
		}

		speed_menu = new Menu("SPEED");
		speed_menu->AddItem("250", MAP_SPEED + 0);
		speed_menu->AddItem("500", MAP_SPEED + 1);
		speed_menu->AddItem("750", MAP_SPEED + 2);
		speed_menu->AddItem("1000", MAP_SPEED + 3);

		hold_menu = new Menu("HOLD");
		hold_menu->AddItem("None", MAP_HOLD + 0);
		hold_menu->AddItem("1 Minute", MAP_HOLD + 1);
		hold_menu->AddItem("5 Minutes", MAP_HOLD + 2);
		hold_menu->AddItem("10 Minutes", MAP_HOLD + 3);
		hold_menu->AddItem("15 Minutes", MAP_HOLD + 4);

		farcast_menu = new Menu("FARCAST");
		farcast_menu->AddItem("Use Quantum", MAP_FARCAST + 0);
		farcast_menu->AddItem("Use Farcast", MAP_FARCAST + 1);

		objective_menu = new Menu("OBJECTIVE");

		nav_menu = new Menu("NAVPT");
		nav_menu->AddMenu("Action", action_menu);
		nav_menu->AddMenu("Objective", objective_menu);
		nav_menu->AddMenu("Formation", formation_menu);
		nav_menu->AddMenu("Speed", speed_menu);
		nav_menu->AddMenu("Hold", hold_menu);
		nav_menu->AddMenu("Farcast", farcast_menu);
		nav_menu->AddItem("", 0);
		nav_menu->AddItem("Add Nav Point", MAP_ADDNAV);
		nav_menu->AddItem("Del Nav Point", MAP_DELETE);
	}
	else if (campaign) {
		ship_menu = 0;
		speed_menu = 0;
		hold_menu = 0;
		farcast_menu = 0;
		objective_menu = 0;
		formation_menu = 0;
		nav_menu = 0;
	}

	active_menu = map_menu;
}

// +--------------------------------------------------------------------+

void MapView::ClearMenu()
{
	delete map_menu;
	delete map_system_menu;
	delete map_sector_menu;
	delete ship_menu;
	delete nav_menu;
	delete action_menu;
	delete objective_menu;
	delete formation_menu;
	delete speed_menu;
	delete hold_menu;
	delete farcast_menu;

	map_menu = 0;
	map_system_menu = 0;
	map_sector_menu = 0;
	ship_menu = 0;
	nav_menu = 0;
	action_menu = 0;
	objective_menu = 0;
	formation_menu = 0;
	speed_menu = 0;
	hold_menu = 0;
	farcast_menu = 0;
}

// +--------------------------------------------------------------------+

void MapView::ProcessMenuItem(int action)
{
	bool send_nav_data = false;
	bool can_command = true;

	if (ship && current_ship && ship != current_ship) {
		if (ship->GetElement() && current_ship->GetElement()) {
			if (!ship->GetElement()->CanCommand(current_ship->GetElement())) {
				can_command = false;
			}
		}
	}

	if (action >= MAP_OBJECTIVE) {
		const int index = action - MAP_OBJECTIVE;

		if (current_navpt && can_command && objective_menu && objective_menu->GetItem(index)) {
			const FString Tgt = objective_menu->GetItem(index)->GetText(); // must be FString-returning overload
			current_navpt->SetTarget(Tgt);
			send_nav_data = true;
		}
	}

	else if (action >= MAP_FARCAST) {
		if (current_navpt && can_command) {
			current_navpt->SetFarcast(action - MAP_FARCAST);
			send_nav_data = true;
		}
	}

	else if (action >= MAP_HOLD) {
		int hold_time = 0;
		switch (action) {
		default:
		case MAP_HOLD + 0:  hold_time = 0;    break;
		case MAP_HOLD + 1:  hold_time = 60;   break;
		case MAP_HOLD + 2:  hold_time = 300;  break;
		case MAP_HOLD + 3:  hold_time = 600;  break;
		case MAP_HOLD + 4:  hold_time = 900;  break;
		}

		if (current_navpt && can_command) {
			current_navpt->SetHoldTime(hold_time);
			send_nav_data = true;
		}
	}

	else if (action >= MAP_SPEED) {
		if (current_navpt && can_command) {
			current_navpt->SetSpeed((action - MAP_SPEED + 1) * 250);
			send_nav_data = true;
		}
	}

	else if (action >= MAP_FORMATION) {
		if (current_navpt && can_command) {
			const INSTRUCTION_FORMATION formation =
				static_cast<INSTRUCTION_FORMATION>(action - MAP_FORMATION);

			current_navpt->SetFormation(formation);
			send_nav_data = true;
		}
	}


	const int raw = action - MAP_ACTION;

	if (raw >= 0 && raw < (int)INSTRUCTION_ACTION::NUM_ACTIONS) {
		current_navpt->SetAction(
			static_cast<INSTRUCTION_ACTION>(raw)
		);
		SelectNavpt(current_navpt);
		send_nav_data = true;
	}

	else if (action == MAP_ADDNAV) {
		Text         rgn_name = regions[current_region]->Name();
		Instruction* prior = current_navpt;
		Instruction* n = 0;

		if (current_ship && can_command) {
			Sim* sim = Sim::GetSim();
			SimRegion* rgn = sim ? sim->FindRegion(rgn_name) : 0;

			// Point -> FVector (UE):
			FVector init_pt(0.0f, 0.0f, 0.0f);

			if (rgn) {
				if (rgn->IsAirSpace())
					init_pt.Z = 10e3f;

				n = new Instruction(rgn, init_pt);
			}
			else {
				n = new Instruction(rgn_name, init_pt);
			}

			n->SetSpeed(500);

			if (prior) {
				n->SetAction(prior->GetAction());
				n->SetFormation(prior->GetFormation());
				n->SetSpeed(prior->Speed());
				n->SetTarget(prior->GetTarget());
			}

			current_ship->AddNavPoint(n, prior);
		}

		else if (current_elem && can_command) {
			FVector init_pt(0.0f, 0.0f, 0.0f);

			if (regions[current_region]->Type() == Orbital::TERRAIN)
				init_pt.Z = 10e3f;

			n = new Instruction(rgn_name, init_pt);
			n->SetSpeed(500);

			if (prior) {
				n->SetAction(prior->GetAction());
				n->SetFormation(prior->GetFormation());
				n->SetSpeed(prior->Speed());
				n->SetTarget(prior->GetTarget());
			}

			current_elem->AddNavPoint(n, prior);
		}

		if (can_command) {
			current_navpt = n;
			current_status = INSTRUCTION_STATUS::PENDING;
			adding_navpt = true;
			captured = SetCapture();
		}
	}

	else if (action == MAP_DELETE) {
		if (current_navpt && can_command) {
			if (current_ship)
				current_ship->DelNavPoint(current_navpt);
			else if (current_elem && can_command)
				current_elem->DelNavPoint(current_navpt);

			SelectNavpt(0);
		}
	}

	else if (action == MAP_CLEAR) {
		if (current_ship && can_command)
			current_ship->ClearFlightPlan();
		else if (current_elem && can_command)
			current_elem->ClearFlightPlan();

		SelectNavpt(0);
	}

	else if (action >= MAP_NAV) {
		// handled elsewhere
	}

	else if (action >= MAP_SHIP) {
		// handled elsewhere
	}

	else if (action >= MAP_SECTOR) {
		int index = action - MAP_SECTOR;

		if (index < regions.size())
			current_region = index;

		if (view_mode == VIEW_SYSTEM) {
			Orbital* s = regions[current_region];
			SetupScroll(s);
		}
	}

	else if (system_list.size() > 0 && action >= MAP_SYSTEM) {
		int index = action - MAP_SYSTEM;

		if (index < system_list.size())
			SetSystem(system_list[index]);
	}	
}

// +--------------------------------------------------------------------+

bool MapView::SetCapture()
{
	if (EventDispatch* dispatch = EventDispatch::GetInstance())
		return dispatch->CaptureMouse(static_cast<EventTarget*>(this)) != 0;

	return false;
}

bool MapView::ReleaseCapture()
{
	if (EventDispatch* dispatch = EventDispatch::GetInstance())
		return dispatch->ReleaseMouse(static_cast<EventTarget*>(this)) != 0;

	return false;
}

// +--------------------------------------------------------------------+

void MapView::SetViewMode(int mode)
{
	if (mode >= 0 && mode < 3) {
		// save state:
		view_zoom[view_mode] = zoom;
		view_offset_x[view_mode] = offset_x;
		view_offset_y[view_mode] = offset_y;

		// switch mode:
		view_mode = mode;

		// restore state:
		if (view_mode == VIEW_GALAXY) {
			zoom = 1;
			offset_x = 0;
			offset_y = 0;
		}
		else {
			zoom = view_zoom[view_mode];
			offset_x = view_offset_x[view_mode];
			offset_y = view_offset_y[view_mode];
		}

		scrolling = 0;
		scroll_x = 0;
		scroll_y = 0;
	}
}

// +--------------------------------------------------------------------+

bool MapView::Update(SimObject* obj)
{
	if (obj == current_ship) {
		current_ship = 0;
		active_menu = map_menu;
	}

	return SimObserver::Update(obj);
}

// +--------------------------------------------------------------------+

void MapView::SelectShip(Ship* selship)
{
	if (selship != current_ship) {
		current_ship = selship;

		if (current_ship) {
			if (current_ship->Life() == 0 || current_ship->IsDying() || current_ship->IsDead()) {
				current_ship = 0;
			}
			else {
				Observe(current_ship);
			}
		}
	}

	SelectNavpt(0);
}

// +--------------------------------------------------------------------+

void MapView::SelectElem(MissionElement* elem)
{
	if (elem != current_elem) {
		current_elem = elem;

		if (current_elem) {
			if (current_elem->IsStarship()) {
				ship_menu->GetItem(3)->SetEnabled(true);
			}
			else if (current_elem->IsDropship()) {
				ship_menu->GetItem(3)->SetEnabled(true);
			}
			else {
				ship_menu->GetItem(3)->SetEnabled(false);
			}
		}
	}

	SelectNavpt(0);
}

// +--------------------------------------------------------------------+

void MapView::SelectNavpt(Instruction* navpt)
{
	current_navpt = navpt;

	if (current_navpt) {
		current_status = current_navpt->GetStatus();

		List<Text> ships;
		objective_menu->ClearItems();

		switch (current_navpt->GetAction()) {
		case INSTRUCTION_ACTION::VECTOR:
		case INSTRUCTION_ACTION::LAUNCH:
		case INSTRUCTION_ACTION::PATROL:
		case INSTRUCTION_ACTION::SWEEP:
		case INSTRUCTION_ACTION::RECON:
			objective_menu->AddItem("NOT AVAILABLE", 0);
			objective_menu->GetItem(0)->SetEnabled(false);
			break;

		case INSTRUCTION_ACTION::DOCK:
			FindShips(true, true, true, false, ships);
			break;

		case INSTRUCTION_ACTION::DEFEND:
			FindShips(true, true, true, false, ships);
			break;

		case INSTRUCTION_ACTION::ESCORT:
			FindShips(true, false, true, true, ships);
			break;

		case INSTRUCTION_ACTION::INTERCEPT:
			FindShips(false, false, false, true, ships);
			break;

		case INSTRUCTION_ACTION::ASSAULT:
			FindShips(false, false, true, false, ships);
			break;

		case INSTRUCTION_ACTION::STRIKE:
			FindShips(false, true, false, false, ships);
			break;
		}

		for (int i = 0; i < ships.size(); i++)
			objective_menu->AddItem(ships[i]->data(), MAP_OBJECTIVE + i);

		ships.destroy();
	}
	else {
		objective_menu->ClearItems();
		objective_menu->AddItem("NOT AVAILABLE", 0);
		objective_menu->GetItem(0)->SetEnabled(false);
	}
}

// +--------------------------------------------------------------------+

void MapView::FindShips(bool bFriendly, bool bStation, bool bStarship, bool bDropship, List<Text>& OutResult)
{
	if (mission) {
		for (int i = 0; i < mission->GetElements().size(); i++) {
			MissionElement* elem = mission->GetElements().at(i);

			if (elem->IsSquadron())                 continue;
			if (!bStation && elem->IsStatic())     continue;
			if (!bStarship && elem->IsStarship())   continue;
			if (!bDropship && elem->IsDropship())   continue;

			if (!editor && bFriendly && elem->GetIFF() > 0 && elem->GetIFF() != mission->GetTeam())
				continue;

			if (!editor && !bFriendly && (elem->GetIFF() == 0 || elem->GetIFF() == mission->GetTeam()))
				continue;

			OutResult.append(new Text(elem->Name()));
		}
	}
	else if (ship) {
		Sim* sim = Sim::GetSim();
		if (sim) {
			for (int RegionIdx = 0; RegionIdx < sim->GetRegions().size(); RegionIdx++) {
				SimRegion* rgn = sim->GetRegions().at(RegionIdx);

				for (int i = 0; i < rgn->GetShips().size(); i++) {
					Ship* s = rgn->GetShips().at(i);

					if (!bStation && s->IsStatic())     continue;
					if (!bStarship && s->IsStarship())   continue;
					if (!bDropship && s->IsDropship())   continue;

					if (bFriendly && s->GetIFF() > 0 && s->GetIFF() != ship->GetIFF())
						continue;

					if (!bFriendly && (s->GetIFF() == 0 || s->GetIFF() == ship->GetIFF()))
						continue;

					OutResult.append(new Text(s->Name()));
				}
			}
		}
	}
}

// +--------------------------------------------------------------------+

void MapView::SetSelectionMode(int mode)
{
	if (mode <= SELECT_NONE) {
		seln_mode = SELECT_NONE;
		return;
	}

	if (mode != seln_mode && mode <= SELECT_FIGHTER) {
		seln_mode = mode;

		// when changing mode,
		// select the item closest to the current center:
		if (system && view_mode == VIEW_SYSTEM)
			SelectAt(rect.x + rect.w / 2, rect.y + rect.h / 2);
	}
}

void MapView::SetSelection(int index)
{
	if (scrolling) return;
	Orbital* s = 0;

	switch (seln_mode) {
	case SELECT_SYSTEM:
		if (index < system_list.size())
			SetSystem(system_list[index]);
		s = stars[current_star];
		break;

	default:
	case SELECT_PLANET:
		if (index < planets.size())
			current_planet = index;
		s = planets[current_planet];
		break;

	case SELECT_REGION:
		if (index < regions.size())
			current_region = index;
		s = regions[current_region];
		break;

	case SELECT_STATION:
	{
		if (mission) {
			MissionElement* selected_elem = 0;

			ListIter<MissionElement> elem = mission->GetElements();
			while (++elem) {
				if (elem->IsStatic()) {
					if (elem->Identity() == index) {
						selected_elem = elem.value();
						break;
					}
				}
			}

			SelectElem(selected_elem);

			if (selected_elem && regions.size()) {
				ListIter<Orbital> rgn = regions;
				while (++rgn) {
					if (!_stricmp(selected_elem->Region(), rgn->Name())) {
						Orbital* elem_region = rgn.value();
						current_region = regions.index(elem_region);
					}
				}
			}
		}
		else {
			Ship* selship = 0;

			if (ship) {
				SimRegion* simrgn = ship->GetRegion();
				if (simrgn) {
					ListIter<Ship> sIter = simrgn->GetShips();
					while (++sIter) {
						if (sIter->IsStatic()) {
							if (sIter->Identity() == index) {
								selship = sIter.value();
								break;
							}
						}
					}
				}
			}

			SelectShip(selship);

			if (selship) {
				s = selship->GetRegion()->GetOrbitalRegion();
				current_region = regions.index(s);
			}
		}
	}
	break;

	case SELECT_STARSHIP:
	{
		if (mission) {
			MissionElement* selected_elem = 0;

			ListIter<MissionElement> elem = mission->GetElements();
			while (++elem) {
				if (elem->IsStarship()) {
					if (elem->Identity() == index) {
						selected_elem = elem.value();
						break;
					}
				}
			}

			SelectElem(selected_elem);

			if (selected_elem && regions.size()) {
				ListIter<Orbital> rgn = regions;
				while (++rgn) {
					if (!_stricmp(selected_elem->Region(), rgn->Name())) {
						Orbital* elem_region = rgn.value();
						current_region = regions.index(elem_region);
					}
				}
			}
		}
		else {
			Ship* selship = 0;

			if (ship) {
				SimRegion* simrgn = ship->GetRegion();
				if (simrgn) {
					ListIter<Ship> sIter = simrgn->GetShips();
					while (++sIter) {
						if (sIter->IsStarship()) {
							if (sIter->Identity() == index) {
								selship = sIter.value();
								break;
							}
						}
					}
				}
			}

			SelectShip(selship);

			if (selship) {
				s = selship->GetRegion()->GetOrbitalRegion();
				current_region = regions.index(s);
			}
		}
	}
	break;

	case SELECT_FIGHTER:
	{
		if (mission) {
			MissionElement* selected_elem = 0;

			ListIter<MissionElement> elem = mission->GetElements();
			while (++elem) {
				if (elem->IsDropship() && !elem->IsSquadron()) {
					if (elem->Identity() == index) {
						selected_elem = elem.value();
						break;
					}
				}
			}

			SelectElem(selected_elem);

			if (selected_elem && regions.size()) {
				ListIter<Orbital> rgn = regions;
				while (++rgn) {
					if (!_stricmp(selected_elem->Region(), rgn->Name())) {
						Orbital* elem_region = rgn.value();
						current_region = regions.index(elem_region);
					}
				}
			}
		}
		else {
			Ship* selship = 0;

			if (ship) {
				SimRegion* simrgn = ship->GetRegion();
				if (simrgn) {
					ListIter<Ship> sIter = simrgn->GetShips();
					while (++sIter) {
						if (sIter->IsDropship()) {
							if (sIter->Identity() == index) {
								selship = sIter.value();
								break;
							}
						}
					}
				}
			}

			SelectShip(selship);

			if (selship) {
				s = selship->GetRegion()->GetOrbitalRegion();
				current_region = regions.index(s);
			}
		}
	}
	break;
	}

	SetupScroll(s);
}

// ============================================================================
// MapView.cpp (Section 2)
// - Selection helpers (ship/element), scrolling, visibility tests
// - Region selection helpers
// - Click selection (SelectAt)
// - Accessors + Refresh + DrawTitle + DrawGalaxy/DrawSystem/DrawRegion
// ============================================================================

void
MapView::SetSelectedShip(Ship* InShip)
{
	if (scrolling) return;

	Ship* selship = nullptr;

	switch (seln_mode) {
	case SELECT_STATION:
	case SELECT_STARSHIP:
	case SELECT_FIGHTER:
	{
		if (InShip) {
			SimRegion* simrgn = InShip->GetRegion();

			// NOTE: legacy List::find returns a pointer if present.
			if (simrgn && simrgn->GetNumShips()) {
				selship = simrgn->GetShips().find(InShip);
			}
		}

		SelectShip(selship);

		// In VIEW_REGION we scroll to ship/elem location, not an Orbital*.
		// SetupScroll ignores the Orbital* in VIEW_REGION when ship/elem exists,
		// so pass current selection for VIEW_SYSTEM convenience:
		if (selship)
			SetupScroll(GetSelection());
	}
	break;

	case SELECT_SYSTEM:
	case SELECT_PLANET:
	case SELECT_REGION:
	default:
		break;
	}
}

void
MapView::SetSelectedElem(MissionElement* elem)
{
	if (scrolling) return;

	switch (seln_mode) {
	case SELECT_STATION:
	case SELECT_STARSHIP:
	case SELECT_FIGHTER:
	{
		SelectElem(elem);

		if (current_elem)
			SetupScroll(GetSelection());
	}
	break;

	case SELECT_SYSTEM:
	case SELECT_PLANET:
	case SELECT_REGION:
	default:
		break;
	}
}

void
MapView::SetupScroll(Orbital* s)
{
	switch (view_mode) {
	case VIEW_GALAXY:
		zoom = 1;
		offset_x = 0;
		offset_y = 0;
		scrolling = 0;
		break;

	case VIEW_SYSTEM:
		if (s == 0) {
			offset_x = 0;
			offset_y = 0;
			scrolling = 0;
		}
		else {
			scroll_x = (offset_x + s->Location().X) / 5.0;
			scroll_y = (offset_y + s->Location().Y) / 5.0;
			scrolling = 5;
		}
		break;

	case VIEW_REGION:
		if (current_navpt) {
			// don't move the map while moving/previewing a nav point:
			scrolling = 0;
		}
		else if (current_ship) {
			// NOTE: legacy uses OtherHand() (handedness swap) for ship locations.
			FVector sloc = ToOtherHand(current_ship->Location());

			if (!IsVisible(sloc)) {
				scroll_x = (offset_x + sloc.X) / 5.0;
				scroll_y = (offset_y + sloc.Y) / 5.0;
				scrolling = 5;
			}
			else {
				scroll_x = 0;
				scroll_y = 0;
				scrolling = 0;
			}
		}
		else if (current_elem) {
			FVector sloc = current_elem->Location();

			if (!IsVisible(sloc)) {
				scroll_x = (offset_x + sloc.X) / 5.0;
				scroll_y = (offset_y + sloc.Y) / 5.0;
				scrolling = 5;
			}
			else {
				scroll_x = 0;
				scroll_y = 0;
				scrolling = 0;
			}
		}
		else {
			offset_x = 0;
			offset_y = 0;
			scrolling = 0;
		}
		break;
	}
}

bool
MapView::IsVisible(const FVector& loc)
{
	if (view_mode == VIEW_REGION) {
		double scale = c / r;
		double ox = offset_x * scale;
		double oy = offset_y * scale;
		double sx = loc.X * scale;
		double sy = loc.Y * scale;
		double cx = rect.w / 2;
		double cy = rect.h / 2;

		int test_x = (int)(cx + sx + ox);
		int test_y = (int)(cy + sy + oy);

		bool visible = test_x >= 0 && test_x < rect.w &&
			test_y >= 0 && test_y < rect.h;

		return visible;
	}

	return false;
}

// +--------------------------------------------------------------------+

void
MapView::SetRegion(OrbitalRegion* rgn)
{
	if (scrolling || rgn == 0) return;

	int index = regions.index(rgn);

	if (index < 0 || index == current_region || index >= regions.size())
		return;

	current_region = index;
	Orbital* s = regions[current_region];

	if (!s)
		return;

	switch (view_mode) {
	case VIEW_GALAXY:
	case VIEW_SYSTEM:
		scroll_x = (offset_x + s->Location().X) / 5.0;
		scroll_y = (offset_y + s->Location().Y) / 5.0;
		scrolling = 5;
		break;

	case VIEW_REGION:
		offset_x = 0;
		offset_y = 0;
		scrolling = 0;
		break;
	}
}

// +--------------------------------------------------------------------+

void
MapView::SetRegionByName(const char* rgn_name)
{
	OrbitalRegion* rgn = 0;

	for (int i = 0; i < regions.size(); i++) {
		Orbital* ro = regions[i];
		if (!strcmp(rgn_name, ro->Name())) {
			rgn = (OrbitalRegion*)ro;
			break;
		}
	}

	SetRegion(rgn);
}

// +--------------------------------------------------------------------+

void
MapView::SelectAt(int x, int y)
{
	if (scrolling) return;
	if (c == 0) return;
	if (seln_mode < 0) return;

	Orbital* s = 0;

	double scale = r / c;
	double cx = rect.w / 2;
	double cy = rect.h / 2;
	double test_x = (x - rect.x - cx) * scale - offset_x;
	double test_y = (y - rect.y - cy) * scale - offset_y;
	double dist = 1.0e20;
	int    closest = 0;

	if (view_mode == VIEW_GALAXY) {
		c = (cx > cy) ? cx : cy;
		r = 10;

		Galaxy* g = Galaxy::GetInstance();
		if (g)
			r = g->Radius();

		StarSystem* closest_system = 0;

		ListIter<StarSystem> iter = system_list;
		while (++iter) {
			StarSystem* sys = iter.value();

			double dx = (sys->Location().X - test_x);
			double dy = (sys->Location().Y - test_y);
			double d = sqrt(dx * dx + dy * dy);

			if (d < dist) {
				dist = d;
				closest_system = sys;
			}
		}

		if (closest_system)
			SetSystem(closest_system);
	}

	else if (view_mode == VIEW_SYSTEM) {
		switch (seln_mode) {
		case SELECT_SYSTEM: {
			if (stars.isEmpty()) return;
			int index = 0;
			ListIter<Orbital> star = stars;
			while (++star) {
				double dx = (star->Location().X - test_x);
				double dy = (star->Location().Y - test_y);
				double d = sqrt(dx * dx + dy * dy);

				if (d < dist) {
					dist = d;
					closest = index;
				}
				index++;
			}

			current_star = closest;
		}
						  s = stars[current_star];
						  break;

		case SELECT_PLANET: {
			if (planets.isEmpty()) return;
			int index = 0;
			ListIter<Orbital> planet = planets;
			while (++planet) {
				double dx = (planet->Location().X - test_x);
				double dy = (planet->Location().Y - test_y);
				double d = sqrt(dx * dx + dy * dy);

				if (d < dist) {
					dist = d;
					closest = index;
				}
				index++;
			}

			current_planet = closest;
		}
						  s = planets[current_planet];
						  break;

		default:
		case SELECT_REGION: {
			if (regions.isEmpty()) return;
			int index = 0;
			ListIter<Orbital> region = regions;
			while (++region) {
				double dx = (region->Location().X - test_x);
				double dy = (region->Location().Y - test_y);
				double d = sqrt(dx * dx + dy * dy);

				if (d < dist) {
					dist = d;
					closest = index;
				}
				index++;
			}

			current_region = closest;
		}
						  s = regions[current_region];
						  break;
		}
	}

	else if (view_mode == VIEW_REGION) {
		dist = 5.0e3;

		if (mission) {
			Orbital* rgn = regions[current_region];
			MissionElement* sel_elem = 0;
			Instruction* sel_nav = 0;

			if (!rgn) return;

			// check nav points:
			ListIter<MissionElement> elem = mission->GetElements();
			while (++elem) {
				MissionElement* e = elem.value();

				if (!e->IsSquadron() && (editor || e->GetIFF() == mission->GetTeam())) {
					ListIter<Instruction> navpt = e->NavList();
					while (++navpt) {
						Instruction* n = navpt.value();

						if (!_stricmp(n->RegionName(), rgn->Name())) {
							FVector  nloc = n->Location();
							double dx = nloc.X - test_x;
							double dy = nloc.Y - test_y;
							double d = sqrt(dx * dx + dy * dy);

							if (d < dist) {
								dist = d;
								sel_nav = n;
								sel_elem = e;
							}
						}
					}
				}
			}

			if (sel_nav) {
				SelectElem(sel_elem);
				SelectNavpt(sel_nav);
			}

			// check elements:
			else {
				elem.reset();
				while (++elem) {
					MissionElement* e = elem.value();

					if (e->Region() == rgn->Name() && !e->IsSquadron()) {
						FVector  sloc = e->Location();
						double dx = sloc.X - test_x;
						double dy = sloc.Y - test_y;
						double d = sqrt(dx * dx + dy * dy);

						if (d < dist) {
							dist = d;
							sel_elem = e;
						}
					}
				}

				SelectElem(sel_elem);

				if (sel_elem)
					s = rgn;
			}
		}
		else if (ship) {
			Sim* sim = Sim::GetSim();
			Orbital* rgn = regions[current_region];
			SimRegion* simrgn = 0;
			Ship* sel_ship = 0;
			Instruction* sel_nav = 0;

			if (sim && rgn)
				simrgn = sim->FindRegion(rgn->Name());

			// check nav points:
			if (simrgn) {
				for (int rr = 0; rr < sim->GetRegions().size(); rr++) {
					SimRegion* region = sim->GetRegions().at(rr);

					for (int i = 0; i < region->GetShips().size(); i++) {
						Ship* sh = region->GetShips().at(i);

						if (sh->GetIFF() == ship->GetIFF() && sh->GetElementIndex() == 1) {
							ListIter<Instruction> navpt = sh->GetFlightPlan();
							while (++navpt) {
								Instruction* n = navpt.value();

								if (!_stricmp(n->RegionName(), rgn->Name())) {
									FVector  nloc = n->Location();
									double dx = nloc.X - test_x;
									double dy = nloc.Y - test_y;
									double d = sqrt(dx * dx + dy * dy);

									if (d < dist) {
										dist = d;
										sel_nav = n;
										sel_ship = sh;
									}
								}
							}
						}
					}
				}
			}

			if (sel_nav) {
				SelectShip(sel_ship);
				SelectNavpt(sel_nav);
			}

			// check ships:
			else if (simrgn && simrgn->GetNumShips()) {
				ListIter<Ship> shipIter = simrgn->GetShips();
				while (++shipIter) {
					Ship* sh = shipIter.value();

					if (!IsClutter(*sh)) {
						FVector sloc = ToOtherHand(current_ship->Location());
						double dx = sloc.X - test_x;
						double dy = sloc.Y - test_y;
						double d = sqrt(dx * dx + dy * dy);

						if (d < dist) {
							dist = d;
							sel_ship = sh;
						}
					}
				}

				SelectShip(sel_ship);
			}
			else {
				SelectShip(0);
				SelectNavpt(0);
			}
		}
	}

	if (s)
		SetupScroll(s);
}

// +--------------------------------------------------------------------+

Orbital*
MapView::GetSelection()
{
	Orbital* s = 0;

	switch (seln_mode) {
	case SELECT_SYSTEM:
		if (current_star < stars.size())
			s = stars[current_star];
		break;

	default:
	case SELECT_PLANET:
		if (current_planet < planets.size())
			s = planets[current_planet];
		break;

	case SELECT_REGION:
		if (current_region < regions.size())
			s = regions[current_region];
		break;

	case SELECT_STATION:
	case SELECT_STARSHIP:
	case SELECT_FIGHTER:
		break;
	}

	return s;
}

Ship*
MapView::GetSelectedShip()
{
	return current_ship;
}

MissionElement*
MapView::GetSelectedElem()
{
	return current_elem;
}

// +--------------------------------------------------------------------+

int
MapView::GetSelectionIndex()
{
	int s = 0;

	switch (seln_mode) {
	case SELECT_SYSTEM:  s = current_star;   break;
	default:
	case SELECT_PLANET:  s = current_planet; break;
	case SELECT_REGION:  s = current_region; break;

	case SELECT_STATION:
	case SELECT_STARSHIP:
	case SELECT_FIGHTER:
	{
		s = -1;
		Sim* sim = Sim::GetSim();
		Orbital* rgn = regions[current_region];
		SimRegion* simrgn = 0;

		if (sim && rgn)
			simrgn = sim->FindRegion(rgn->Name());

		if (simrgn) {
			if (current_ship && simrgn->GetNumShips()) {
				s = simrgn->GetShips().index(current_ship);
			}
		}
	}
	break;
	}

	return s;
}

void MapView::DrawTabbedText(SystemFont* InFont, const char* text)
{
	if (InFont && text && *text) {
		Rect label_rect;

		label_rect.w = rect.w;
		label_rect.h = rect.h;
		label_rect.Inset(8, 8, 8, 8);

		DWORD text_flags = DT_WORDBREAK | DT_LEFT;

		active_window->SetFont(InFont);
		active_window->DrawText(text, 0, label_rect, text_flags);
	}
}

// +--------------------------------------------------------------------+

void
MapView::Refresh()
{
	rect = GetRect();

	if (!system) {
		DrawGrid();
		DrawTabbedText(TitleFont, "No System");
		return;
	}

	if (font)
		font->SetColor(FColor::White);

	if (scrolling) {
		offset_x -= scroll_x;
		offset_y -= scroll_y;
		scrolling--;
	}

	rect.w -= 2;
	rect.h -= 2;

	switch (view_mode) {
	case VIEW_GALAXY: DrawGalaxy(); break;
	case VIEW_SYSTEM: DrawSystem(); break;
	case VIEW_REGION: DrawRegion(); break;
	default:          DrawGrid();   break;
	}

	rect.w += 2;
	rect.h += 2;

	if (menu_view) {
		if (current_navpt) {
			active_menu = nav_menu;
		}
		else if (current_ship) {
			if (ship && current_ship->GetIFF() == ship->GetIFF())
				active_menu = ship_menu;
			else
				active_menu = map_menu;
		}
		else if (current_elem) {
			if (mission && (editor || current_elem->GetIFF() == mission->GetTeam()))
				active_menu = ship_menu;
			else
				active_menu = map_menu;
		}
		else {
			active_menu = map_menu;
		}

		menu_view->SetBackColor(FColor(128, 128, 128));
		menu_view->SetTextColor(FColor::White);
		menu_view->SetMenu(active_menu);
		menu_view->DoMouseFrame();

		if (menu_view->GetAction()) {
			ProcessMenuItem(menu_view->GetAction());
		}

		menu_view->Refresh();
	}

	DrawTitle();
}

// +--------------------------------------------------------------------+

void
MapView::DrawTitle()
{
	TitleFont->SetColor(active_window->GetForeColor());
	DrawTabbedText(TitleFont, title);
}

// +--------------------------------------------------------------------+

void
MapView::DrawGalaxy()
{
	title = Game::GetText("MapView.title.Galaxy");
	DrawGrid();

	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx > cy) ? cx : cy;
	r = 10; // * zoom;

	Galaxy* g = Galaxy::GetInstance();
	if (g)
		r = g->Radius(); // * zoom;

	double scale = c / r;
	double ox = 0;
	double oy = 0;

	// compute offset:
	ListIter<StarSystem> iter = system_list;
	while (++iter) {
		StarSystem* sys = iter.value();

		if (system == sys) {
			if (fabs(sys->Location().X) > 10 || fabs(sys->Location().Y) > 10) {
				int sx = (int)sys->Location().X;
				int sy = (int)sys->Location().Y;

				sx -= sx % 10;
				sy -= sy % 10;

				ox = sx * -scale;
				oy = sy * -scale;
			}
		}
	}

	// draw the list of systems, and their connections:
	iter.reset();
	while (++iter) {
		StarSystem* sys = iter.value();

		int sx = (int)(cx + ox + sys->Location().X * scale);
		int sy = (int)(cy + oy + sys->Location().Y * scale);

		if (sx < 4 || sx > rect.w - 4 || sy < 4 || sy > rect.h - 4)
			continue;

		DrawEllipse(sx - 7, sy - 7, sx + 7, sy + 7, Ship::IFFColor(sys->Affiliation()));

		if (sys == system) {
			DrawLine(0, sy, rect.w, sy, FColor(128, 128, 128), Video::BLEND_ADDITIVE);
			DrawLine(sx, 0, sx, rect.h, FColor(128, 128, 128), Video::BLEND_ADDITIVE);
		}

		ListIter<StarSystem> iter2 = system_list;
		while (++iter2) {
			StarSystem* sys2 = iter2.value();

			if (sys != sys2 && sys->HasLinkTo(sys2)) {
				int ax = sx;
				int ay = sy;

				int bx = (int)(cx + ox + sys2->Location().X * scale);
				int by = (int)(cy + oy + sys2->Location().Y * scale);

				if (ax == bx) {
					if (ay < by) { ay += 8; by -= 8; }
					else { ay -= 8; by += 8; }
				}
				else if (ay == by) {
					if (ax < bx) { ax += 8; bx -= 8; }
					else { ax -= 8; bx += 8; }
				}
				else {
					FVector d = FVector(bx, by, 0) - FVector(ax, ay, 0);
					d.Normalize();

					ax += (int)(8 * d.X);
					ay += (int)(8 * d.Y);

					bx -= (int)(8 * d.X);
					by -= (int)(8 * d.Y);
				}

				DrawLine(ax, ay, bx, by, FColor(120, 120, 120), Video::BLEND_ADDITIVE);
			}
		}
	}

	// finally draw all the stars in the galaxy:
	if (g) {
		ListIter<Star> sIter = g->Stars();
		while (++sIter) {
			Star* st = sIter.value();

			int sx = (int)(cx + ox + st->Location().X * scale);
			int sy = (int)(cy + oy + st->Location().Y * scale);
			int sr = st->GetSize();

			if (sx < 4 || sx > rect.w - 4 || sy < 4 || sy > rect.h - 4)
				continue;

			FillEllipse(sx - sr, sy - sr, sx + sr, sy + sr, st->GetColor());

			if (!strncmp(st->GetName(), "GSC", 3))
				font->SetColor(FColor(100, 100, 100));
			else
				font->SetColor(FColor::White);

			Rect name_rect(sx - 60, sy + 8, 120, 20);
			active_window->SetFont(font);
			active_window->DrawText(st->GetName(), 0, name_rect, DT_SINGLELINE | DT_CENTER);
		}
	}

	font->SetColor(FColor::White);
}

// +--------------------------------------------------------------------+

void
MapView::DrawSystem()
{
	Text caption = Game::GetText("MapView.title.Starsystem");
	caption += " ";
	caption += system->GetName();

	if (current_ship) {
		caption += "\n";
		caption += Game::GetText("MapView.title.Ship");
		caption += " ";
		caption += current_ship->Name();
	}
	else if (current_elem) {
		caption += "\n";
		caption += Game::GetText("MapView.title.Ship");
		caption += " ";
		caption += current_elem->Name();
	}

	title = caption;

	ListIter<OrbitalBody> star = system->Bodies();
	while (++star) {
		int p_orb = 1;

		ListIter<OrbitalBody> planet = star->Satellites();
		while (++planet) {
			DrawOrbital(*planet, p_orb++);

			int m_orb = 1;

			ListIter<OrbitalBody> moon = planet->Satellites();
			while (++moon) {
				DrawOrbital(*moon, m_orb++);

				ListIter<OrbitalRegion> region = moon->Regions();
				while (++region) {
					DrawOrbital(*region, 1);
				}
			}

			ListIter<OrbitalRegion> region = planet->Regions();
			while (++region) {
				DrawOrbital(*region, 1);
			}
		}

		ListIter<OrbitalRegion> region = star->Regions();
		while (++region) {
			DrawOrbital(*region, 1);
		}

		DrawOrbital(*star, 0);
	}

	char r_txt[32];
	FormatNumber(r_txt, system->Radius() * zoom);

	char resolution[64];
	sprintf_s(resolution, "%s: %s", Game::GetText("MapView.info.Resolution").data(), r_txt);

	active_window->SetFont(font);
	Rect text_rect(4, 4, rect.w - 8, 24);

	active_window->DrawText(
		resolution,
		-1,
		text_rect,
		DT_SINGLELINE | DT_RIGHT
	);
}

// +--------------------------------------------------------------------+

void
MapView::DrawRegion()
{
	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];

	Text caption = Game::GetText("MapView.title.Sector");
	caption += " ";
	caption += rgn->Name();

	if (current_ship) {
		caption += "\n";
		caption += Game::GetText("MapView.title.Ship");
		caption += " ";
		caption += current_ship->Name();
	}
	else if (current_elem) {
		caption += "\n";
		caption += Game::GetText("MapView.title.Ship");
		caption += " ";
		caption += current_elem->Name();
	}

	title = caption;

	double cx = rect.w / 2;
	double cy = rect.h / 2;

	int size = (int)rgn->Radius();
	int step = (int)rgn->GridSpace();

	c = (cx < cy) ? cx : cy;
	r = rgn->Radius() * zoom;
	double scale = c / r;

	double ox = offset_x * scale;
	double oy = offset_y * scale;

	int left = (int)(-size * scale + ox + cx);
	int right = (int)(size * scale + ox + cx);
	int top = (int)(-size * scale + oy + cy);
	int bottom = (int)(size * scale + oy + cy);

	FColor major(48, 48, 48);
	FColor minor(24, 24, 24);

	int x, y;
	int tick = 0;

	for (x = 0; x <= size; x += step) {
		int lx = (int)(x * scale + ox + cx);
		if (!tick)
			DrawLine(lx, top, lx, bottom, major, Video::BLEND_ADDITIVE);
		else
			DrawLine(lx, top, lx, bottom, minor, Video::BLEND_ADDITIVE);

		lx = (int)(-x * scale + ox + cx);
		if (!tick)
			DrawLine(lx, top, lx, bottom, major, Video::BLEND_ADDITIVE);
		else
			DrawLine(lx, top, lx, bottom, minor, Video::BLEND_ADDITIVE);

		if (++tick > 3) tick = 0;
	}

	tick = 0;

	for (y = 0; y <= size; y += step) {
		int ly = (int)(y * scale + oy + cy);
		if (!tick)
			DrawLine(left, ly, right, ly, major, Video::BLEND_ADDITIVE);
		else
			DrawLine(left, ly, right, ly, minor, Video::BLEND_ADDITIVE);

		ly = (int)(-y * scale + oy + cy);
		if (!tick)
			DrawLine(left, ly, right, ly, major, Video::BLEND_ADDITIVE);
		else
			DrawLine(left, ly, right, ly, minor, Video::BLEND_ADDITIVE);

		if (++tick > 3) tick = 0;
	}

	int rep = 3;
	if (r > 70e3)  rep = 2;
	if (r > 250e3) rep = 1;

	if (campaign && rgn) {
		// draw the combatants in this region:
		ListIter<Combatant> iter = campaign->GetCombatants();
		while (++iter) {
			Combatant* combatant = iter.value();
			DrawCombatGroup(combatant->GetForce(), rep);
		}
	}

	else if (mission && rgn) {
		// draw the elements in this region:
		ListIter<MissionElement> elem = mission->GetElements();
		while (++elem)
			if (!elem->IsSquadron())
				DrawElem(*elem, (elem.value() == current_elem), rep);
	}

	else if (ship) {
		// draw the ships in this region:
		Sim* sim = Sim::GetSim();
		SimRegion* simrgn = 0;

		if (sim && rgn)
			simrgn = sim->FindRegion(rgn->Name());

		if (simrgn) {
			ListIter<SimContact> cIter = simrgn->TrackList(ship->GetIFF());
			while (++cIter) {
				SimContact* contact = cIter.value();
				Ship* s = contact->GetShip();

				if (s && ((int)s->Class() & ship_filter) && !IsClutter(*s) && s != ship)
					DrawShip(*s, (s == current_ship), rep);
			}

			ListIter<Ship> s_iter = simrgn->GetShips();
			while (++s_iter) {
				Ship* s = s_iter.value();

				if (s && (s->IsStatic()) && !IsClutter(*s) &&
					(s->GetIFF() == ship->GetIFF() || s->GetIFF() == 0))
					DrawShip(*s, (s == current_ship), rep);
			}

			// draw nav routes for allied ships not in the region:
			ListIter<SimRegion> r_iter = sim->GetRegions();
			while (++r_iter) {
				SimRegion* r2 = r_iter.value();

				if (r2 != simrgn) {
					ListIter<Ship> s_iter2 = r2->GetShips();
					while (++s_iter2) {
						Ship* s2 = s_iter2.value();

						if (s2 && !s2->IsStatic() && !IsClutter(*s2) &&
							(s2->GetIFF() == ship->GetIFF() || s2->GetIFF() == 0)) {
							DrawNavRoute(simrgn->GetOrbitalRegion(),
								s2->GetFlightPlan(),
								s2->MarkerColor(),
								s2, 0);
						}
					}
				}
			}
		}

		// draw our own ship:
		DrawShip(*ship, (ship == current_ship), rep);
	}

	char r_txt[32];
	FormatNumber(r_txt, r * 2);

	char resolution[64];
	sprintf_s(resolution, "%s: %s", Game::GetText("MapView.info.Resolution").data(), r_txt);

	active_window->SetFont(font);
	Rect text_rect(4, 4, rect.w - 8, 24);

	active_window->DrawText(
		resolution,
		-1,
		text_rect,
		DT_SINGLELINE | DT_RIGHT
	);
}

// +--------------------------------------------------------------------+

void
MapView::DrawGrid()
{
	int grid_step = rect.w / 8;
	int cx = rect.w / 2;
	int cy = rect.h / 2;

	FColor cc(32, 32, 32);

	DrawLine(0, cy, rect.w, cy, cc, Video::BLEND_ADDITIVE);
	DrawLine(cx, 0, cx, rect.h, cc, Video::BLEND_ADDITIVE);

	for (int i = 1; i < 4; i++) {
		DrawLine(0, cy + (i * grid_step), rect.w, cy + (i * grid_step), cc, Video::BLEND_ADDITIVE);
		DrawLine(0, cy - (i * grid_step), rect.w, cy - (i * grid_step), cc, Video::BLEND_ADDITIVE);
		DrawLine(cx + (i * grid_step), 0, cx + (i * grid_step), rect.h, cc, Video::BLEND_ADDITIVE);
		DrawLine(cx - (i * grid_step), 0, cx - (i * grid_step), rect.h, cc, Video::BLEND_ADDITIVE);
	}
}

// +--------------------------------------------------------------------+

void MapView::DrawOrbital(Orbital& body, int index)
{
	int type = body.Type();
	if (type == Orbital::NOTHING)
		return;

	int  x1, y1, x2, y2;
	Rect label_rect;
	int  label_w = 64;
	int  label_h = 18;

	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx < cy) ? cx : cy;
	r = system->Radius() * zoom;

	if ((r > 300e9) && (type > Orbital::PLANET))
		return;

	double xscale = cx / r;
	double yscale = cy / r * 0.75;

	double ox = offset_x * xscale;
	double oy = offset_y * yscale;

	double bo_x = body.Orbit() * xscale;
	double bo_y = body.Orbit() * yscale;
	double br = body.Radius() * yscale;

	double bx = body.Location().X * xscale;
	double by = body.Location().Y * yscale;

	double px = 0;
	double py = 0;

	if (body.Primary()) {
		double min_pr = GetMinRadius(body.Primary()->Type());

		if (index) {
			if (min_pr < 4) min_pr = 4;
			min_pr *= (index + 1);
		}

		double min_x = min_pr * xscale / yscale;
		double min_y = min_pr;

		if (bo_x < min_x) bo_x = min_x;
		if (bo_y < min_y) bo_y = min_y;

		px = body.Primary()->Location().X * xscale;
		py = body.Primary()->Location().Y * yscale;
	}

	if (type == Orbital::TERRAIN)
		bo_x = bo_y;

	int ipx = (int)(cx + px + ox);
	int ipy = (int)(cy + py + oy);
	int ibo_x = (int)bo_x;
	int ibo_y = (int)bo_y;

	x1 = ipx - ibo_x;
	y1 = ipy - ibo_y;
	x2 = ipx + ibo_x;
	y2 = ipy + ibo_y;

	if (type != Orbital::TERRAIN) {
		double a = x2 - x1;
		double b = rect.w * 32;
		if (a < b)
			DrawEllipse(x1, y1, x2, y2, FColor(64, 64, 64), Video::BLEND_ADDITIVE);
	}

	bx = px + bo_x * cos(body.Phase());
	by = py + bo_y * sin(body.Phase());

	double min_br = GetMinRadius(type);
	if (br < min_br) br = min_br;

	FColor color = FColor::White;
	switch (type) {
	case Orbital::STAR:    color = FColor(248, 248, 128); break;
	case Orbital::PLANET:  color = FColor(64, 64, 192); break;
	case Orbital::MOON:    color = FColor(32, 192, 96); break;
	case Orbital::REGION:  color = FColor(255, 255, 255); break;
	case Orbital::TERRAIN: color = FColor(16, 128, 48); break;
	default: break;
	}

	int icx = (int)(cx + bx + ox);
	int icy = (int)(cy + by + oy);
	int ibr = (int)br;

	x1 = icx - ibr;
	y1 = icy - ibr;
	x2 = icx + ibr;
	y2 = icy + ibr;

	if(type < Orbital::REGION)
	{
		Bitmap* map_icon = body.GetMapIcon();
		if (!map_icon)
			return;

		if (map_icon->Width() > 64)
		{
			if (type == Orbital::STAR)
				DrawBitmap(x1, y1, x2, y2, map_icon, Video::BLEND_ADDITIVE);
			else
				DrawBitmap(x1, y1, x2, y2, map_icon, Video::BLEND_ALPHA);
		}
		else
		{
			FillEllipse(x1, y1, x2, y2, color);
		}
	}
	else
	{
		DrawRect(x1, y1, x2, y2, color);

		if (campaign) {
			ListIter<Combatant> iter = campaign->GetCombatants();
			while (++iter) {
				Combatant* combatant = iter.value();
				if (ibr >= 4 || combatant->GetIFF() == 1)
					DrawCombatantSystem(combatant, &body, icx, icy, ibr);
			}
		}
	}

	if (type == Orbital::STAR || bo_y > label_h) {
		label_rect.x = x1 - label_w + (int)br;
		label_rect.y = y1 - label_h;
		label_rect.w = label_w * 2;
		label_rect.h = label_h;

		active_window->SetFont(font);
		active_window->DrawText(body.Name(), -1, label_rect, DT_SINGLELINE | DT_CENTER);
	}
}

// +--------------------------------------------------------------------+

double
MapView::GetMinRadius(int type)
{
	switch (type) {
	case Orbital::STAR:     return 8;
	case Orbital::PLANET:   return 4;
	case Orbital::MOON:     return 2;
	case Orbital::REGION:   return 2;
	}

	return 0;
}

// +--------------------------------------------------------------------+

static void ColorizeBitmap(Bitmap& img, const FColor& Tint)
{
	const int w = img.Width();
	const int h = img.Height();

	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			const FColor c = img.GetColor(x, y);

			if (c != FColor::Black && c != FColor::White) {
				const uint8 shade = (uint8)(c.B / 2);
				img.SetColor(x, y, ShadeTint(Tint, shade));
			}
		}
	}

	img.AutoMask();
}

static FVector shipshape1[] = {
	FVector(6.f,  0.f, 0.f),
	FVector(-6.f,  4.f, 0.f),
	FVector(-6.f, -4.f, 0.f)
};

static FVector shipshape2[] = {
	FVector(8.f,  0.f, 0.f),
	FVector(4.f,  4.f, 0.f),
	FVector(-8.f,  4.f, 0.f),
	FVector(-8.f, -4.f, 0.f),
	FVector(4.f, -4.f, 0.f)
};

static FVector shipshape3[] = {
	FVector(8.f,  8.f, 0.f),
	FVector(-8.f,  8.f, 0.f),
	FVector(-8.f, -8.f, 0.f),
	FVector(8.f, -8.f, 0.f)
};

// +--------------------------------------------------------------------+

void MapView::DrawShip(Ship& s, bool current, int rep)
{
	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];
	if (!rgn)
		return;

	int x1, y1, x2, y2;

	FVector shiploc(0, 0, 0);

	// Legacy handedness fix:
	FVector sloc = ToOtherHand(FVector(s.Location()));

	double cx = rect.w * 0.5;
	double cy = rect.h * 0.5;

	c = (cx < cy) ? cx : cy;
	r = rgn->Radius() * zoom;

	double scale = c / r;
	double ox = offset_x * scale;
	double oy = offset_y * scale;

	double rlx = 0;
	double rly = 0;

	int sprite_width = 10;

	SetFont(font);

	// ----------------------------------------------------
	// Draw ship icon
	// ----------------------------------------------------
	if (rep && view_mode == VIEW_REGION &&
		s.GetRegion() &&
		rgn == s.GetRegion()->GetOrbitalRegion())
	{
		double sx = (sloc.X + rlx) * scale;
		double sy = (sloc.Y + rly) * scale;

		shiploc.X = (int)(cx + sx + ox);
		shiploc.Y = (int)(cy + sy + oy);

		bool ship_visible =
			shiploc.X >= 0 && shiploc.X < rect.w &&
			shiploc.Y >= 0 && shiploc.Y < rect.h;

		if (ship_visible) {
			// ------------------------------------------------
			// Low-detail marker
			// ------------------------------------------------
			if (rep < 3) {
				FillRect(
					(int)shiploc.X - 2,
					(int)shiploc.Y - 2,
					(int)shiploc.X + 2,
					(int)shiploc.Y + 2,
					s.MarkerColor()
				);

				sprite_width = 2;

				if (&s == ship || !IsCrowded(s)) {
					Print(
						(int)shiploc.X - sprite_width,
						(int)shiploc.Y + sprite_width + 2,
						s.Name()
					);
				}
			}
			// ------------------------------------------------
			// High-detail sprite
			// ------------------------------------------------
			else {
				FVector heading = ToOtherHand(FVector(s.Heading()));
				heading.Z = 0;
				heading.Normalize();

				double theta;
				if (heading.Y > 0)
					theta = acos(heading.X);
				else
					theta = -acos(heading.X);

				const double THETA_SLICE = 4.0 / PI;
				const double THETA_OFFSET = PI + THETA_SLICE * 0.5;

				int sprite_index = (int)((theta + THETA_OFFSET) * THETA_SLICE);
				int nsprites = s.Design()->map_sprites.size();

				if (nsprites > 0) {
					sprite_index = sprite_index % nsprites;
					if (sprite_index < 0)
						sprite_index += nsprites;

					Bitmap* map_sprite = s.Design()->map_sprites[sprite_index];

					Bitmap bmp;
					bmp.CopyBitmap(*map_sprite);
					ColorizeBitmap(bmp, s.MarkerColor());

					sprite_width = bmp.Width() / 2;
					int h = bmp.Height() / 2;

					DrawBitmap(
						(int)shiploc.X - sprite_width,
						(int)shiploc.Y - h,
						(int)shiploc.X + sprite_width,
						(int)shiploc.Y + h,
						&bmp,
						Video::BLEND_ALPHA
					);
				}
				// ------------------------------------------------
				// Fallback shapes
				// ------------------------------------------------
				else {
					if (s.IsStatic()) {
						FillRect(
							(int)shiploc.X - 6, (int)shiploc.Y - 6,
							(int)shiploc.X + 6, (int)shiploc.Y + 6,
							s.MarkerColor()
						);
						DrawRect(
							(int)shiploc.X - 6, (int)shiploc.Y - 6,
							(int)shiploc.X + 6, (int)shiploc.Y + 6,
							FColor::White
						);
						sprite_width = 6;
					}
					else if (s.IsStarship()) {
						FillRect(
							(int)shiploc.X - 4, (int)shiploc.Y - 4,
							(int)shiploc.X + 4, (int)shiploc.Y + 4,
							s.MarkerColor()
						);
						DrawRect(
							(int)shiploc.X - 4, (int)shiploc.Y - 4,
							(int)shiploc.X + 4, (int)shiploc.Y + 4,
							FColor::White
						);
						sprite_width = 4;
					}
					else {
						FillRect(
							(int)shiploc.X - 3, (int)shiploc.Y - 3,
							(int)shiploc.X + 3, (int)shiploc.Y + 3,
							s.MarkerColor()
						);
						DrawRect(
							(int)shiploc.X - 3, (int)shiploc.Y - 3,
							(int)shiploc.X + 3, (int)shiploc.Y + 3,
							FColor::White
						);
						sprite_width = 3;
					}
				}

				Print(
					(int)shiploc.X - sprite_width,
					(int)shiploc.Y + sprite_width + 2,
					s.Name()
				);
			}
		}
	}

	// ----------------------------------------------------
	// Current selection highlight
	// ----------------------------------------------------
	if (current &&
		s.GetRegion() &&
		Text(s.GetRegion()->GetName()) == regions[current_region]->Name())
	{
		x1 = (int)(shiploc.X - sprite_width - 1);
		x2 = (int)(shiploc.X + sprite_width + 1);
		y1 = (int)(shiploc.Y - sprite_width - 1);
		y2 = (int)(shiploc.Y + sprite_width + 1);

		DrawRect(x1, y1, x2, y2, FColor::White);
	}

	// ----------------------------------------------------
	// Draw nav routes (friendly only)
	// ----------------------------------------------------
	if (s.GetIFF() == 0 || (ship && s.GetIFF() == ship->GetIFF())) {
		DrawNavRoute(rgn, s.GetFlightPlan(), s.MarkerColor(), &s, 0);
	}
}

// +--------------------------------------------------------------------+

void
MapView::DrawElem(MissionElement& s, bool current, int rep)
{
	if (!mission) return;

	bool visible =
		editor ||
		s.GetIFF() == 0 ||
		s.GetIFF() == mission->GetTeam() ||
		s.IntelLevel() > Intel::KNOWN;

	if (!visible) return;

	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];
	if (!rgn) return;

	int   x1, y1, x2, y2;
	FVector shiploc{};

	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx < cy) ? cx : cy;
	r = rgn->Radius() * zoom;

	double scale = c / r;

	double ox = offset_x * scale;
	double oy = offset_y * scale;

	double rlx = 0;
	double rly = 0;

	int sprite_width = 10;

	SetFont(font);

	// draw ship icon:
	if (!_stricmp(s.Region(), rgn->Name())) {
		double sx = (s.Location().X + rlx) * scale;
		double sy = (s.Location().Y + rly) * scale;

		shiploc.X = (int)(cx + sx + ox);
		shiploc.Y = (int)(cy + sy + oy);

		bool ship_visible =
			shiploc.X >= 0 && shiploc.X < rect.w &&
			shiploc.Y >= 0 && shiploc.Y < rect.h;

		if (ship_visible) {
			if (rep < 3) {
				FillRect(shiploc.X - 2, shiploc.Y - 2, shiploc.X + 2, shiploc.Y + 2, s.MarkerColor());
				sprite_width = 2;

				if (!IsCrowded(s))
					Print(shiploc.X - sprite_width, shiploc.Y + sprite_width + 2, s.Name());
			}
			else {
				double theta = s.Heading();

				const double THETA_SLICE = 4 / PI;
				const double THETA_OFFSET = PI / 2;

				int sprite_index = (int)((theta + THETA_OFFSET) * THETA_SLICE);
				int nsprites = 0;

				if (s.GetDesign())
					nsprites = s.GetDesign()->map_sprites.size();

				if (nsprites > 0) {
					if (sprite_index < 0 || sprite_index >= nsprites)
						sprite_index = sprite_index % nsprites;

					Bitmap* map_sprite = s.GetDesign()->map_sprites[sprite_index];

					Bitmap bmp;
					bmp.CopyBitmap(*map_sprite);
					ColorizeBitmap(bmp, s.MarkerColor());
					sprite_width = bmp.Width() / 2;
					int h = bmp.Height() / 2;

					DrawBitmap(shiploc.X - sprite_width,
						shiploc.Y - h,
						shiploc.X + sprite_width,
						shiploc.Y + h,
						&bmp,
						Video::BLEND_ALPHA);
				}
				else {
					theta -= PI / 2;

					if (s.IsStatic()) {
						FillRect(shiploc.X - 6, shiploc.Y - 6, shiploc.X + 6, shiploc.Y + 6, s.MarkerColor());
						DrawRect(shiploc.X - 6, shiploc.Y - 6, shiploc.X + 6, shiploc.Y + 6, FColor::White);
					}
					else if (s.IsStarship()) {
						FillRect(shiploc.X - 4, shiploc.Y - 4, shiploc.X + 4, shiploc.Y + 4, s.MarkerColor());
						DrawRect(shiploc.X - 4, shiploc.Y - 4, shiploc.X + 4, shiploc.Y + 4, FColor::White);
					}
					else {
						FillRect(shiploc.X - 3, shiploc.Y - 3, shiploc.X + 3, shiploc.Y + 3, s.MarkerColor());
						DrawRect(shiploc.X - 3, shiploc.Y - 3, shiploc.X + 3, shiploc.Y + 3, FColor::White);
					}
				}

				char label[64];

				if (s.Count() > 1)
					sprintf_s(label, "%s x %d", (const char*)s.Name(), s.Count());
				else
					strcpy_s(label, (const char*)s.Name());

				Print(shiploc.X - sprite_width, shiploc.Y + sprite_width + 2, label);
			}
		}
	}

	// draw current element marker:
	if (current && s.Region() == regions[current_region]->Name()) {
		x1 = (int)(shiploc.X - sprite_width - 1);
		x2 = (int)(shiploc.X + sprite_width + 1);
		y1 = (int)(shiploc.Y - sprite_width - 1);
		y2 = (int)(shiploc.Y + sprite_width + 1);

		DrawRect(x1, y1, x2, y2, FColor::White);
	}

	// only see routes for your own team:
	if (editor || s.GetIFF() == 0 || (mission && s.GetIFF() == mission->GetTeam())) {
		DrawNavRoute(rgn, s.NavList(), s.MarkerColor(), 0, &s);
	}
}

// +--------------------------------------------------------------------+

void MapView::DrawNavRoute(
	OrbitalRegion* rgn,
	List<Instruction>& s_route,
	FColor               s_marker,
	Ship* route_ship,   
	MissionElement* elem
)
{
	if (!rgn)
		return;

	int x1 = 0, y1 = 0, x2 = 0, y2 = 0;

	const double cx = rect.w * 0.5;
	const double cy = rect.h * 0.5;

	c = (cx < cy) ? cx : cy;
	r = system ? (system->Radius() * zoom) : 1.0;

	if (view_mode == VIEW_REGION) {
		r = rgn->Radius() * zoom;
	}

	const double scale = c / r;
	const double ox = offset_x * scale;
	const double oy = offset_y * scale;

	// --- Helpers ------------------------------------------------------

	auto ScaleColor = [](const FColor& In, float S) -> FColor
		{
			FLinearColor L(In);
			L *= S;
			return L.ToFColor(true);
		};

	auto AbsI = [](int v) { return v < 0 ? -v : v; };

	// --- Route state --------------------------------------------------

	FVector old_loc(0, 0, 0);
	double  old_x = 0;
	double  old_y = 0;
	bool    old_in = false;

	FVector first_loc(0, 0, 0);
	int     first_x = 0;
	int     first_y = 0;
	bool    first_in = false;

	bool draw_route = true;
	bool draw_bold = false;

	// Legacy rule: element index > 1 means "not flight lead" => don't draw route:
	if (route_ship && route_ship->GetElementIndex() > 1)
		draw_route = false;

	// Bold highlight if current selection:
	if (route_ship && route_ship == current_ship) {
		s_marker = ScaleColor(s_marker, 1.5f);
		draw_bold = true;
	}
	else if (elem && elem == current_elem) {
		s_marker = ScaleColor(s_marker, 1.5f);
		draw_bold = true;
	}

	// --- Draw segments + navpoint markers ----------------------------

	for (int i = 0; i < s_route.size(); i++) {
		Instruction* navpt = s_route[i];
		if (!navpt)
			continue;

		// Only draw navpoints that belong to this region:
		if (_stricmp(navpt->RegionName(), rgn->Name()) != 0) {
			old_loc = FVector(navpt->Location());
			old_x = 0;
			old_y = 0;
			old_in = false;
			continue;
		}

		const FVector nav_loc = FVector(navpt->Location());

		const double nav_x = nav_loc.X * scale;
		const double nav_y = nav_loc.Y * scale;

		const int isx = (int)(cx + nav_x + ox);
		const int isy = (int)(cy + nav_y + oy);

		// draw segment from previous point:
		if (old_in && draw_route) {
			const int iox = (int)(cx + old_x + ox);
			const int ioy = (int)(cy + old_y + oy);

			DrawLine(iox, ioy, isx, isy, s_marker, 0);

			const int dx = (iox - isx);
			const int dy = (ioy - isy);

			if (draw_bold) {
				// Legacy behavior was a rough "more horizontal vs vertical" decision.
				if (AbsI(dx) > AbsI(dy))
					DrawLine(iox, ioy + 1, isx, isy + 1, s_marker, 0);
				else
					DrawLine(iox + 1, ioy, isx + 1, isy, s_marker, 0);
			}

			// distance label:
			if ((dx * dx + dy * dy) > 2000) {
				const double dist = FVector(nav_loc - old_loc).Length();

				const int imx = (int)(cx + (old_x + nav_x) * 0.5 + ox);
				const int imy = (int)(cy + (old_y + nav_y) * 0.5 + oy);

				char dist_txt[32];
				FormatNumber(dist_txt, dist);

				font->SetColor(FColor(128, 128, 128));
				SetFont(font);
				Print(imx - 20, imy - 6, dist_txt);
				font->SetColor(FColor::White);
			}
		}

		// navpoint marker:
		x1 = isx - 3; y1 = isy - 3;
		x2 = isx + 3; y2 = isy + 3;

		FColor markColor = FColor::White;

		if (navpt->GetStatus() > INSTRUCTION_STATUS::ACTIVE) {
			markColor = FColor(128, 128, 128);
		}
		else if (!first_in) {
			first_in = true;
			first_loc = nav_loc;
			first_x = isx;
			first_y = isy;
		}

		if (draw_route) {
			DrawLine(x1, y1, x2, y2, markColor, 0);
			DrawLine(x1, y2, x2, y1, markColor, 0);

			if (navpt == current_navpt)
				DrawRect(x1 - 2, y1 - 2, x2 + 2, y2 + 2, markColor, 0);

			char buf[256];
			sprintf_s(buf, "%d", i + 1);
			SetFont(font);
			Print(x2 + 3, y1, buf);

			// expanded info for selected navpoint:
			if (navpt == current_navpt) {
				if (navpt->TargetName() && strlen(navpt->TargetName())) {
					sprintf_s(buf, "%s %s",
						Game::GetText(Text("MapView.item.") + Instruction::ActionName(navpt->GetAction())).data(),
						navpt->TargetName());
					Print(x2 + 3, y1 + 10, buf);
				}
				else {
					sprintf_s(buf, "%s",
						Game::GetText(Text("MapView.item.") + Instruction::ActionName(navpt->GetAction())).data());
					Print(x2 + 3, y1 + 10, buf);
				}

				sprintf_s(buf, "%s",
					Game::GetText(Text("MapView.item.") + Instruction::FormationName(navpt->GetFormation())).data());
				Print(x2 + 3, y1 + 20, buf);

				sprintf_s(buf, "%d", navpt->Speed());
				Print(x2 + 3, y1 + 30, buf);

				if (navpt->HoldTime()) {
					char hold_time[32];
					FormatTime(hold_time, navpt->HoldTime());

					sprintf_s(buf, "%s %s", "Hold", hold_time);
					Print(x2 + 3, y1 + 40, buf);
				}
			}
		}

		old_loc = nav_loc;
		old_x = nav_x;
		old_y = nav_y;
		old_in = true;
	}

	// --- Draw line from ship/elem to first active navpoint ------------

	if (first_in) {
		old_in = false;

		if (route_ship && route_ship->GetRegion()) {
			old_in = (route_ship->GetRegion()->GetOrbitalRegion() == rgn);

			if (old_in) {
				old_loc = ToOtherHand(FVector(route_ship->Location()));
				old_x = old_loc.X * scale;
				old_y = old_loc.Y * scale;
			}
		}
		else if (elem) {
			old_in = (_stricmp(elem->Region(), rgn->Name()) == 0);

			if (old_in) {
				old_loc = FVector(elem->Location());
				old_x = old_loc.X * scale;
				old_y = old_loc.Y * scale;
			}
		}

		if (old_in) {
			const int iox = (int)(cx + old_x + ox);
			const int ioy = (int)(cy + old_y + oy);

			DrawLine(iox, ioy, first_x, first_y, s_marker, 0);

			const int dx = (iox - first_x);
			const int dy = (ioy - first_y);

			if (draw_bold) {
				if (AbsI(dx) > AbsI(dy))
					DrawLine(iox, ioy + 1, first_x, first_y + 1, s_marker, 0);
				else
					DrawLine(iox + 1, ioy, first_x + 1, first_y, s_marker, 0);
			}

			if ((dx * dx + dy * dy) > 2000) {
				const double dist = FVector(first_loc - old_loc).Length();

				const double nav_x = first_loc.X * scale;
				const double nav_y = first_loc.Y * scale;

				const int imx = (int)(cx + (old_x + nav_x) * 0.5 + ox);
				const int imy = (int)(cy + (old_y + nav_y) * 0.5 + oy);

				char dist_txt[32];
				FormatNumber(dist_txt, dist);

				font->SetColor(FColor(128, 128, 128));
				SetFont(font);
				Print(imx - 20, imy - 6, dist_txt);
				font->SetColor(FColor::White);
			}
		}
	}
}

// +--------------------------------------------------------------------+

void MapView::DrawCombatantSystem(Combatant* CombatantPtr, Orbital* RegionOrbital, int InX, int InY, int InRadiusPx)
{
	if (!CombatantPtr) return;

	const int Team = CombatantPtr->GetIFF();

	int X1 = 0;
	int X2 = 0;
	int Y1 = InY - InRadiusPx;
	int Align = 0;

	switch (Team) {
	case 0:
		X1 = InX - 64;
		X2 = InX + 64;
		Y1 = InY + InRadiusPx + 4;
		Align = DT_CENTER;
		break;

	case 1:
		X1 = InX - 200;
		X2 = InX - InRadiusPx - 4;
		Align = DT_RIGHT;
		break;

	default:
		X1 = InX + InRadiusPx + 4;
		X2 = InX + 200;
		Align = DT_LEFT;
		break;
	}

	DrawCombatGroupSystem(CombatantPtr->GetForce(), RegionOrbital, X1, X2, Y1, Align);
}

// +--------------------------------------------------------------------+

void
MapView::DrawCombatGroupSystem(CombatGroup* group, Orbital* rgn, int x1, int x2, int& y, int a)
{
	if (!group || group->IsReserve() || group->CalcValue() < 1)
		return;

	char txt[80];

	if (group->GetRegion() == rgn->Name()) {
		switch (group->GetType()) {
		case ECOMBATGROUP_TYPE::CARRIER_GROUP:
		case ECOMBATGROUP_TYPE::BATTLE_GROUP:
		case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
		{
			sprintf_s(txt, "%s '%s'", group->GetShortDescription(), group->GetName().data());
			active_window->SetFont(font);

			Rect text_rect(x1, y, x2 - x1, 12);
			active_window->DrawText(txt, 0, text_rect, a);

			y += 10;
			break;
		}

		case ECOMBATGROUP_TYPE::BATTALION:
		case ECOMBATGROUP_TYPE::STATION:
		case ECOMBATGROUP_TYPE::STARBASE:
		case ECOMBATGROUP_TYPE::MINEFIELD:
		case ECOMBATGROUP_TYPE::BATTERY:
		case ECOMBATGROUP_TYPE::MISSILE:
		{
			active_window->SetFont(font);

			Rect text_rect(x1, y, x2 - x1, 12);

			active_window->DrawText(
				group->GetShortDescription(),
				0,
				text_rect,
				a
			);

			y += 10;
			break;
		}

		default:
			break;
		}
	}

	ListIter<CombatGroup> iter = group->GetComponents();
	while (++iter) {
		CombatGroup* g = iter.value();
		DrawCombatGroupSystem(g, rgn, x1, x2, y, a);
	}
}

// +--------------------------------------------------------------------+

void
MapView::DrawCombatGroup(CombatGroup* group, int rep)
{
	// does group even exist yet?
	if (!group || group->IsReserve() || group->CalcValue() < 1)
		return;

	// is group a squadron? don't draw squadrons on map:
	if (group->GetType() >= ECOMBATGROUP_TYPE::WING && group->GetType() < ECOMBATGROUP_TYPE::FLEET)
		return;

	// has group been discovered yet?
	CombatGroup* player_group = campaign->GetPlayerGroup();
	if (group->GetIFF() && player_group && player_group->GetIFF() != group->GetIFF())
		if (group->GetIntelLevel() <= Intel::KNOWN)
			return;

	// has group been destroyed already?
	if (group->CalcValue() < 1)
		return;

	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];
	if (!rgn) return;

	FVector shiploc{};

	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx < cy) ? cx : cy;
	r = rgn->Radius() * zoom;

	double scale = c / r;

	double ox = offset_x * scale;
	double oy = offset_y * scale;

	double rlx = 0;
	double rly = 0;

	int sprite_width = 10;

	if (group->GetUnits().size() > 0) {
		CombatUnit* unit = 0;

		for (int i = 0; i < group->GetUnits().size(); i++) {
			unit = group->GetUnits().at(i);

			if (unit->Count() - unit->DeadCount() > 0)
				break;
		}

		// draw unit icon:
		if (unit && unit->GetRegion() == rgn->Name() && unit->Type() > (int) CLASSIFICATION::LCA && unit->Count() > 0) {
			double sx = (unit->Location().X + rlx) * scale;
			double sy = (unit->Location().Y + rly) * scale;

			shiploc.X = (int)(cx + sx + ox);
			shiploc.Y = (int)(cy + sy + oy);

			bool ship_visible =
				shiploc.X >= 0 && shiploc.X < rect.w &&
				shiploc.Y >= 0 && shiploc.Y < rect.h;

			if (ship_visible) {
				if (rep < 3) {
					FillRect(shiploc.X - 2, shiploc.Y - 2, shiploc.X + 2, shiploc.Y + 2, unit->MarkerColor());
					sprite_width = 2;

					char buf[256];
					sprintf_s(buf, "%s", unit->Name().data());
					SetFont(font);
					Print(shiploc.X - sprite_width, shiploc.Y + sprite_width + 2, buf);
				}
				else {
					int sprite_index = 2;
					int nsprites = 0;

					if (unit->GetDesign())
						nsprites = unit->GetDesign()->map_sprites.size();

					if (nsprites) {
						if (sprite_index < 0 || sprite_index >= nsprites)
							sprite_index = sprite_index % nsprites;

						Bitmap* map_sprite = unit->GetDesign()->map_sprites[sprite_index];

						Bitmap bmp;
						bmp.CopyBitmap(*map_sprite);
						ColorizeBitmap(bmp, unit->MarkerColor());
						sprite_width = bmp.Width() / 2;
						int h = bmp.Height() / 2;

						DrawBitmap(shiploc.X - sprite_width,
							shiploc.Y - h,
							shiploc.X + sprite_width,
							shiploc.Y + h,
							&bmp,
							Video::BLEND_ALPHA);
					}
					else {
						if (unit->IsStatic()) {
							FillRect(shiploc.X - 6, shiploc.Y - 6, shiploc.X + 6, shiploc.Y + 6, unit->MarkerColor());
							DrawRect(shiploc.X - 6, shiploc.Y - 6, shiploc.X + 6, shiploc.Y + 6, FColor::White);
						}
						else if (unit->IsStarship()) {
							FillRect(shiploc.X - 4, shiploc.Y - 4, shiploc.X + 4, shiploc.Y + 4, unit->MarkerColor());
							DrawRect(shiploc.X - 4, shiploc.Y - 4, shiploc.X + 4, shiploc.Y + 4, FColor::White);
						}
						else {
							FillRect(shiploc.X - 3, shiploc.Y - 3, shiploc.X + 3, shiploc.Y + 3, unit->MarkerColor());
							DrawRect(shiploc.X - 3, shiploc.Y - 3, shiploc.X + 3, shiploc.Y + 3, FColor::White);
						}
					}

					char label[128];
					strcpy_s(label, unit->GetDescription());
					SetFont(font);
					Print(shiploc.X - sprite_width, shiploc.Y + sprite_width + 2, label);
				}
			}
		}
	}

	// recurse:
	ListIter<CombatGroup> iter = group->GetComponents();
	while (++iter) {
		CombatGroup* g = iter.value();
		DrawCombatGroup(g, rep);
	}
}

// +--------------------------------------------------------------------+

bool
MapView::IsClutter(Ship& test)
{
	Ship* leader = test.GetLeader();

	if (leader == &test)   // this is the leader:
		return false;

	// too close to leader?
	if (leader) {
		FVector testloc{}, leaderloc{};

		GetShipLoc(test, testloc);
		GetShipLoc(*leader, leaderloc);

		double dx = testloc.X - leaderloc.X;
		double dy = testloc.Y - leaderloc.Y;
		double d = dx * dx + dy * dy;

		if (d <= 64)
			return true;
	}

	return false;
}

// +--------------------------------------------------------------------+

bool
MapView::IsCrowded(Ship& test)
{
	FVector      testloc{}, refloc{};
	Sim* sim = Sim::GetSim();
	Orbital* rgn = regions[current_region];
	SimRegion* simrgn = sim ? sim->FindRegion(rgn->Name()) : 0;

	if (simrgn) {
		GetShipLoc(test, testloc);

		ListIter<Ship> s = simrgn->GetShips();
		while (++s) {
			Ship* ref = s.value();

			if (ref && ref != &test) {
				GetShipLoc(*ref, refloc);

				double dx = testloc.X - refloc.X;
				double dy = testloc.Y - refloc.Y;
				double d = dx * dx + dy * dy;

				if (d <= 64)
					return true;
			}
		}
	}

	return false;
}

void
MapView::GetShipLoc(Ship& s, FVector& shiploc)
{
	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx < cy) ? cx : cy;
	r = system->Radius() * zoom;

	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];

	if (view_mode == VIEW_REGION) {
		if (!rgn) return;
		r = rgn->Radius() * zoom;
	}

	double scale = c / r;

	double ox = offset_x * scale;
	double oy = offset_y * scale;

	double rlx = 0;
	double rly = 0;

	if (view_mode == VIEW_SYSTEM) {
		rgn = system->ActiveRegion();

		if (rgn) {
			rlx = rgn->Location().X;
			rly = rgn->Location().Y;
		}
	}

	if (view_mode == VIEW_SYSTEM ||
		(view_mode == VIEW_REGION && rgn == s.GetRegion()->GetOrbitalRegion())) {
		double sx = (s.Location().X + rlx) * scale;
		double sy = (s.Location().Y + rly) * scale;

		shiploc.X = (int)(cx + sx + ox);
		shiploc.Y = (int)(cy + sy + oy);
	}
	else {
		shiploc.X = -1;
		shiploc.Y = -1;
	}
}

// +--------------------------------------------------------------------+

bool
MapView::IsCrowded(MissionElement& test)
{
	FVector    testloc{}, refloc{};
	Sim* sim = Sim::GetSim();
	Orbital* rgn = regions[current_region];

	(void)sim; // legacy keeps sim around; unused in this branch.

	if (mission) {
		GetElemLoc(test, testloc);

		ListIter<MissionElement> it = mission->GetElements();
		while (++it) {
			MissionElement* ref = it.value();

			if (ref && ref != &test && !_stricmp(ref->Region(), rgn->Name())) {
				GetElemLoc(*ref, refloc);

				double dx = testloc.X - refloc.X;
				double dy = testloc.Y - refloc.Y;
				double d = dx * dx + dy * dy;

				if (d <= 64)
					return true;
			}
		}
	}

	return false;
}

void
MapView::GetElemLoc(MissionElement& s, FVector& shiploc)
{
	double cx = rect.w / 2;
	double cy = rect.h / 2;

	c = (cx < cy) ? cx : cy;
	r = system->Radius() * zoom;

	OrbitalRegion* rgn = (OrbitalRegion*)regions[current_region];

	if (view_mode == VIEW_REGION) {
		if (!rgn) return;
		r = rgn->Radius() * zoom;
	}

	double scale = c / r;

	double ox = offset_x * scale;
	double oy = offset_y * scale;

	double rlx = 0;
	double rly = 0;

	if (view_mode == VIEW_SYSTEM) {
		rgn = system->ActiveRegion();

		if (rgn) {
			rlx = rgn->Location().X;
			rly = rgn->Location().Y;
		}
	}

	if (view_mode == VIEW_SYSTEM ||
		(view_mode == VIEW_REGION && !_stricmp(s.Region(), rgn->Name()))) {
		double sx = (s.Location().X + rlx) * scale;
		double sy = (s.Location().Y + rly) * scale;

		shiploc.X = (int)(cx + sx + ox);
		shiploc.Y = (int)(cy + sy + oy);
	}
	else {
		shiploc.X = -1;
		shiploc.Y = -1;
	}
}

// +--------------------------------------------------------------------+

OrbitalRegion*
MapView::GetRegion() const
{
	OrbitalRegion* result = 0;

	if (current_region < regions.size())
		result = (OrbitalRegion*)regions[current_region];

	return result;
}

// +--------------------------------------------------------------------+

void
MapView::ZoomIn()
{
	zoom *= 0.9;

	if (view_mode == VIEW_SYSTEM) {
		if (system && zoom * system->Radius() < 2e6) {
			zoom = 2e6 / system->Radius();
		}
	}
	else if (view_mode == VIEW_REGION) {
		OrbitalRegion* rgn = GetRegion();
		if (rgn && zoom * rgn->Radius() < 1e3) {
			zoom = 1e3 / rgn->Radius();
		}
	}
}

void
MapView::ZoomOut()
{
	zoom *= 1.1;

	if (view_mode == VIEW_SYSTEM) {
		if (system && zoom * system->Radius() > 500e9) {
			zoom = 500e9 / system->Radius();
		}
	}
	else if (view_mode == VIEW_REGION) {
		OrbitalRegion* rgn = GetRegion();
		if (rgn && zoom * rgn->Radius() > 1e6) {
			zoom = 1e6 / rgn->Radius();
		}
	}
}

// +--------------------------------------------------------------------+

void
MapView::OnShow()
{
	EventDispatch* dispatch = EventDispatch::GetInstance();
	if (dispatch)
		dispatch->Register(this);
}

void
MapView::OnHide()
{
	EventDispatch* dispatch = EventDispatch::GetInstance();
	if (dispatch)
		dispatch->Unregister(this);

	if (captured) {
		ReleaseCapture();
		captured = false;
		Mouse::Show(true);
	}

	dragging = false;
}

// +--------------------------------------------------------------------+

bool
MapView::IsEnabled() const
{
	if (active_window) {
		return active_window->IsEnabled();
	}

	return false;
}

bool
MapView::IsVisible() const
{
	if (active_window) {
		return active_window->IsVisible();
	}

	return false;
}

bool
MapView::IsFormActive() const
{
	if (active_window) {
		return active_window->IsFormActive();
	}
	return false;
}

Rect
MapView::TargetRect() const
{
	if (active_window) {
		return active_window->TargetRect();
	}
	return Rect();
}

// +--------------------------------------------------------------------+

bool
MapView::OnMouseMove(int32 x, int32 y)
{
	if (captured) {
		EventTarget* test = 0;
		EventDispatch* dispatch = EventDispatch::GetInstance();
		if (dispatch)
			test = dispatch->GetCapture();

		if (test != this) {
			captured = false;
			Mouse::Show(true);
		}
		else {
			if (dragging) {
				int delta_x = x - mouse_x;
				int delta_y = y - mouse_y;

				offset_x += delta_x * r / c;
				offset_y += delta_y * r / c;

				Mouse::SetCursorPos(mouse_x, mouse_y);
			}
			else if (view_mode == VIEW_REGION) {
				double scale = r / c;
				click_x = (x - rect.x - rect.w / 2) * scale - offset_x;
				click_y = (y - rect.y - rect.h / 2) * scale - offset_y;

				if ((adding_navpt || moving_navpt) && current_navpt) {
					FVector loc = current_navpt->Location();
					loc.X = click_x;
					loc.Y = click_y;
					current_navpt->SetLocation(loc);
					current_navpt->SetStatus(current_status);
				}
				else if (editor && moving_elem && current_elem) {
					FVector loc = current_elem->Location();
					loc.X = click_x;
					loc.Y = click_y;
					current_elem->SetLocation(loc);
				}
			}
		}
	}

	const int Result = active_window->OnMouseMove(x, y);
	return Result != 0;
}

// +--------------------------------------------------------------------+

int
MapView::OnRButtonDown(int32 x, int32 y)
{
	if (!captured)
		captured = SetCapture();

	if (captured) {
		dragging = true;
		mouse_x = x;
		mouse_y = y;
		Mouse::Show(false);
	}

	return active_window->OnRButtonDown(x, y);
}

// +--------------------------------------------------------------------+

int
MapView::OnRButtonUp(int32 x, int32 y)
{
	if (captured) {
		ReleaseCapture();
		captured = false;
		Mouse::Show(true);
	}

	dragging = false;

	return active_window->OnRButtonUp(x, y);
}

// +--------------------------------------------------------------------+

int
MapView::OnClick()
{
	return active_window->OnClick();
}

int
MapView::OnLButtonDown(int32 x, int32 y)
{
	if (menu_view && menu_view->IsShown()) {
		// ignore this event...
	}
	else {
		if (!captured)
			captured = SetCapture();

		if (view_mode == VIEW_REGION) {
			double scale = r / c;
			click_x = (x - rect.x - rect.w / 2) * scale - offset_x;
			click_y = (y - rect.y - rect.h / 2) * scale - offset_y;

			if (current_navpt) {
				FVector  nloc = current_navpt->Location();
				double dx = nloc.X - click_x;
				double dy = nloc.Y - click_y;
				double d = sqrt(dx * dx + dy * dy);

				if (d < 5e3) {
					moving_navpt = true;

					if (ship && current_ship && ship != current_ship) {
						if (ship->GetElement() && current_ship->GetElement()) {
							if (!ship->GetElement()->CanCommand(current_ship->GetElement())) {
								moving_navpt = false;
							}
						}
					}
				}
			}
		}
	}

	return active_window->OnLButtonDown(x, y);
}

int
MapView::OnLButtonUp(int32 x, int32 y)
{
	bool process_event = false;

	if (captured) {
		process_event = true;
		ReleaseCapture();
		captured = false;
		Mouse::Show(true);
	}

	if (process_event && !adding_navpt) {
		if (!moving_navpt && !moving_elem) {
			UIButton::PlaySound();
		}

		if (view_mode == VIEW_REGION) {
			double scale = r / c;
			click_x = (x - rect.x - rect.w / 2) * scale - offset_x;
			click_y = (y - rect.y - rect.h / 2) * scale - offset_y;

			if (!scrolling)
				SelectAt(x, y);

			active_window->ClientEvent(EID_MAP_CLICK, x, y);
		}
		else if (!scrolling) {
			SelectAt(x, y);
			active_window->ClientEvent(EID_MAP_CLICK, x, y);
		}
	}

	if ((adding_navpt || moving_navpt) && current_navpt) {
		current_navpt->SetStatus(current_status);
	}

	adding_navpt = false;
	moving_navpt = false;
	moving_elem = false;

	return active_window->OnLButtonUp(x, y);
}
