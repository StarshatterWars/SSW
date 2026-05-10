/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioHandler.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC


	OVERVIEW
	========
	Radio message handler class implementation
*/

#include "RadioHandler.h"

#include "RadioMessage.h"
#include "RadioTraffic.h"
#include "Instruction.h"
#include "SimDirector.h"
#include "SimContact.h"
#include "SimElement.h"
#include "Mission.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Sim.h"
#include "StarSystem.h"
#include "Power.h"
#include "Drive.h"
#include "Shield.h"
#include "Hangar.h"
#include "FlightDeck.h"
#include "WeaponGroup.h"
#include "SteerAI.h"

#include "Text.h"
#include "Game.h"
#include "GameStructs.h"

// Minimal Unreal includes for UE_LOG + FVector conversions used below:
#include "Math/Vector.h"
#include "Logging/LogMacros.h"
#include "Containers/UnrealString.h"   // FString
#include "Misc/StringBuilder.h"        // optional, not required

// Create a local log category for this translation unit:
DEFINE_LOG_CATEGORY_STATIC(LogRadioHandler, Log, All);

// +--------------------------------------------------------------------+

RadioHandler::RadioHandler()
{
}

RadioHandler::~RadioHandler()
{
}

// +--------------------------------------------------------------------+

bool
RadioHandler::ProcessMessage(RadioMessage* msg, Ship* s)
{
	if (!s || !msg || !msg->GetSender())
		return false;

	if (s->Class() >= CLASSIFICATION::FARCASTER && s->Class() <= CLASSIFICATION::C3I)
		return false;

	if (msg->GetSender()->IsRogue()) {
		Ship* sender = (Ship*)msg->GetSender();  // cast-away const
		RadioMessage* nak = new RadioMessage(sender, s, RadioMessageAction::NACK);
		RadioTraffic::Transmit(nak);
		return false;
	}

	bool respond = (s != msg->GetSender());

	// SPECIAL CASE:
	// skip navpoint must be processed by elem leader,
	// even if the elem leader sent the message:
	if (msg->GetRadioAction() == RadioMessageAction::SKIP_NAVPOINT && !respond)
		ProcessMessageAction(msg, s);

	if (!ProcessMessageOrders(msg, s))
		respond = respond && ProcessMessageAction(msg, s);

	return respond;
}

// +----------------------------------------------------------------------+

bool
RadioHandler::IsOrder(RadioMessageAction action)
{
	bool result = false;

	switch (action) {
	default:
	case RadioMessageAction::NONE:
	case RadioMessageAction::ACK:
	case RadioMessageAction::NACK:  
		result = false; 
		break;

		// target mgt:
	case RadioMessageAction::ATTACK:
	case RadioMessageAction::ESCORT:
	case RadioMessageAction::BRACKET:
	case RadioMessageAction::IDENTIFY:
		result = true;  
		break;

		// combat mgt:
	case RadioMessageAction::COVER_ME:
	case RadioMessageAction::WEP_HOLD:
	case RadioMessageAction::FORM_UP: 
		result = true;  
		break;

	case RadioMessageAction::WEP_FREE:
	case RadioMessageAction::SAY_POSITION:
	case RadioMessageAction::LAUNCH_PROBE:  
		result = false; 
		break;

		// formation mgt:
	case RadioMessageAction::GO_DIAMOND:
	case RadioMessageAction::GO_SPREAD:
	case RadioMessageAction::GO_BOX:
	case RadioMessageAction::GO_TRAIL:  
		result = true;
		break;

		// mission mgt:
	case RadioMessageAction::MOVE_PATROL:    
		result = true;
		break;
	case RadioMessageAction::SKIP_NAVPOINT:   
		result = false;
		break;
	case RadioMessageAction::RESUME_MISSION:
		result = true; 
		break;

	case RadioMessageAction::RTB:
	case RadioMessageAction::DOCK_WITH:
	case RadioMessageAction::QUANTUM_TO:
	case RadioMessageAction::FARCAST_TO:
		result = true;
		break;

		// sensor mgt:
	case RadioMessageAction::GO_EMCON1:
	case RadioMessageAction::GO_EMCON2:
	case RadioMessageAction::GO_EMCON3:     
		result = true;  
		break;

		// support:
	case RadioMessageAction::REQUEST_PICTURE:
	case RadioMessageAction::REQUEST_SUPPORT:
	case RadioMessageAction::PICTURE:    
		result = false;
		break;

		// traffic control:
	case RadioMessageAction::CALL_INBOUND:
	case RadioMessageAction::CALL_APPROACH:
	case RadioMessageAction::CALL_CLEARANCE:
	case RadioMessageAction::CALL_FINALS:
	case RadioMessageAction::CALL_WAVE_OFF:
		result = false;
		break;
	}

	return result;
}

// +----------------------------------------------------------------------+

bool
RadioHandler::ProcessMessageOrders(RadioMessage* msg, Ship* ship)
{
	Instruction* instruction = ship->GetRadioOrders();
	RadioMessageAction action = RadioMessageAction::NONE;

	if (msg && msg->GetRadioAction() == RadioMessageAction::RESUME_MISSION) {
		instruction->SetRadioAction(RadioMessageAction::NONE);
		instruction->SetFormation(INSTRUCTION_FORMATION::NONE);
		instruction->SetWeaponsFree(true);
		if (instruction->GetTarget()) {
			instruction->ClearTarget();
			ship->DropTarget();
		}
		return true;
	}

	if (msg && IsOrder(msg->GetRadioAction())) {
		int posture_only = false;

		action = msg->GetRadioAction();

		if (action == RadioMessageAction::FORM_UP)
			action = RadioMessageAction::WEP_HOLD;

		// target orders => drop current target:
		if (action >= RadioMessageAction::ATTACK &&
			action <= RadioMessageAction::COVER_ME ||
			action == RadioMessageAction::WEP_HOLD ||
			action >= RadioMessageAction::DOCK_WITH &&
			action <= RadioMessageAction::FARCAST_TO) {

			if (ship != msg->GetSender())
				ship->DropTarget();

			SimDirector* dir = ship->GetDirector();
			if (dir && dir->GetType() >= ESteerAIType::SEEKER && dir->GetType() <= ESteerAIType::GROUND) {
				SteerAI* ai = (SteerAI*)dir;
				ai->SetTarget(0);
			}

			// farcast and quantum jump radio messages:
			if (action >= RadioMessageAction::QUANTUM_TO) {
				Sim* sim = Sim::GetSim();

				if (sim) {
					SimRegion* rgn = sim->FindRegion(msg->GetInfo());

					if (rgn) {
						instruction->SetRadioAction(action);
						instruction->SetLocation(FVector::ZeroVector); // Point(0,0,0) -> FVector
						instruction->SetRegion(rgn);
						instruction->SetFarcast(action == RadioMessageAction::FARCAST_TO);
						instruction->SetWeaponsFree(false);
						return true;
					}
				}
			}
		}

		// formation orders => set formation:
		if (action >= RadioMessageAction::GO_DIAMOND &&
			action <= RadioMessageAction::GO_TRAIL) {

			switch (action) {
			case RadioMessageAction::GO_DIAMOND:  
				instruction->SetFormation(INSTRUCTION_FORMATION::DIAMOND); 
				break;
			case RadioMessageAction::GO_SPREAD:    
				instruction->SetFormation(INSTRUCTION_FORMATION::SPREAD);
				break;
			case RadioMessageAction::GO_BOX:
				instruction->SetFormation(INSTRUCTION_FORMATION::BOX);
				break;
			case RadioMessageAction::GO_TRAIL:    
				instruction->SetFormation(INSTRUCTION_FORMATION::TRAIL);
				break;
			}

			posture_only = true;
		}

		// emcon orders => set emcon:
		if (action >= RadioMessageAction::GO_EMCON1 &&
			action <= RadioMessageAction::GO_EMCON3) {

			switch (msg->GetRadioAction()) {
			case RadioMessageAction::GO_EMCON1:  
				instruction->SetEMCON(1);  
				break;
			case RadioMessageAction::GO_EMCON2: 
				instruction->SetEMCON(2);  
				break;
			case RadioMessageAction::GO_EMCON3:   
				instruction->SetEMCON(3);
				break;
			}

			posture_only = true;
		}

		if (!posture_only) {
			instruction->SetRadioAction(action);
			instruction->ClearTarget();

			if (msg->TargetList().size() > 0) {
				SimObject* msg_tgt = msg->TargetList().at(0);
				instruction->SetTarget(msg_tgt);

				// Point(...) is a Starshatter helper type; convert to FVector directly:
				instruction->SetLocation(msg_tgt->GetLocation());
			}

			else if (action == RadioMessageAction::COVER_ME) {
				instruction->SetTarget((Ship*)msg->GetSender());
				instruction->SetLocation(msg->GetSender()->GetLocation());
			}

			else if (action == RadioMessageAction::MOVE_PATROL) {
				instruction->SetLocation(msg->GetLocation());
			}

			// handle element engagement:
			if (action == RadioMessageAction::ATTACK && msg->TargetList().size() > 0) {
				SimElement* elem = msg->DestinationElem();

				if (!elem && msg->DestinationShip())
					elem = msg->DestinationShip()->GetElement();

				if (elem) {
					SimObject* msg_tgt = msg->TargetList().at(0);
					if (msg_tgt && msg_tgt->GetType() == SimObject::SIM_SHIP) {
						SimElement* tgt = ((Ship*)msg_tgt)->GetElement();
						elem->SetAssignment(tgt);

						if (msg->TargetList().size() > 1)
							instruction->SetTarget(tgt->Name().data());
						else
							instruction->SetTarget(msg_tgt);
					}
					else {
						elem->ResumeAssignment();
					}
				}
			}

			else if (action == RadioMessageAction::RESUME_MISSION) {
				SimElement* elem = msg->DestinationElem();

				if (!elem && msg->DestinationShip())
					elem = msg->DestinationShip()->GetElement();

				if (elem) {
					elem->ResumeAssignment();
				}
			}
		}

		instruction->SetWeaponsFree(action <= RadioMessageAction::WEP_FREE);
		return true;
	}

	return false;
}

// +----------------------------------------------------------------------+

bool
RadioHandler::ProcessMessageAction(RadioMessage* msg, Ship* ship)
{
	if (!msg) return false;

	if (msg->GetRadioAction() == RadioMessageAction::CALL_INBOUND)
		return Inbound(msg, ship);

	if (msg->GetRadioAction() == RadioMessageAction::CALL_FINALS)
		return true;   // acknowledge

	if (msg->GetRadioAction() == RadioMessageAction::REQUEST_PICTURE)
		return Picture(msg, ship);

	if (msg->GetRadioAction() == RadioMessageAction::REQUEST_SUPPORT)
		return Support(msg, ship);

	if (msg->GetRadioAction() == RadioMessageAction::SKIP_NAVPOINT)
		return SkipNavpoint(msg, ship);

	if (msg->GetRadioAction() == RadioMessageAction::LAUNCH_PROBE)
		return LaunchProbe(msg, ship);

	return false;
}

bool
RadioHandler::SkipNavpoint(RadioMessage* msg, Ship* ship)
{
	// Find next Instruction:
	Instruction* navpt = ship->GetNextNavPoint();
	int          elem_index = ship->GetElementIndex();

	if (navpt && elem_index < 2) {
		ship->SetNavptStatus(navpt, INSTRUCTION_STATUS::SKIPPED);
	}

	return true;
}

bool
RadioHandler::LaunchProbe(RadioMessage* msg, Ship* ship)
{
	if (ship && ship->GetProbeLauncher()) {
		ship->LaunchProbe();
		return ship->GetProbe() != 0;
	}

	return false;
}

bool
RadioHandler::Inbound(RadioMessage* msg, Ship* ship)
{
	Ship* inbound = (Ship*)msg->GetSender();
	Hangar* hangar = ship->GetHangar();
	FlightDeck* deck = 0;
	int         squadron = -1;
	int         slot = -1;
	bool        same_rgn = false;

	if (inbound && inbound->GetRegion() == ship->GetRegion())
		same_rgn = true;

	// is the sender already inbound to us?
	if (inbound->GetInbound() &&
		inbound->GetInbound()->GetDeck() &&
		inbound->GetInbound()->GetDeck()->GetCarrier() == ship) {
		InboundSlot* islot = inbound->GetInbound();
		deck = islot->GetDeck();
		squadron = islot->Squadron();
		slot = islot->Index();
	}

	// otherwise, find space for sender:
	else {
		if (hangar && same_rgn) {
			if (hangar->FindSlot(inbound, squadron, slot)) {
				int shortest_queue = 1000;

				for (int i = 0; i < ship->NumFlightDecks(); i++) {
					FlightDeck* d = ship->GetFlightDeck(i);
					if (d->IsRecoveryDeck()) {
						int nwaiting = d->GetRecoveryQueue().size();

						if (nwaiting < shortest_queue) {
							deck = d;
							shortest_queue = nwaiting;
						}
					}
				}
			}
		}
	}

	// if no space (or not a carrier!) wave sender off:
	if (!deck || !same_rgn || squadron < 0 || slot < 0) {
		RadioMessage* wave_off = new RadioMessage(inbound, ship, RadioMessageAction::NACK);
		if (!hangar)
			wave_off->SetInfo("No Hangar");

		else if (!same_rgn) {
			const char* RegionNameAnsi = ship->GetRegion() ? ship->GetRegion()->GetName() : "Unknown";
			const FString Info = FString::Printf(TEXT("Too Far Away (%s)"), ANSI_TO_TCHAR(RegionNameAnsi));
			wave_off->SetInfo(TCHAR_TO_ANSI(*Info));
		}

		else
			wave_off->SetInfo("Landing Bays all full");

		RadioTraffic::Transmit(wave_off);
		return false;
	}

	// put sender in recovery queue, if not already there:
	InboundSlot* inbound_slot = inbound->GetInbound();
	int          sequence = 0;

	if (!inbound_slot) {
		inbound_slot = new InboundSlot(inbound, deck, squadron, slot);
		sequence = deck->Inbound(inbound_slot);
	}
	else {
		sequence = inbound_slot->Index();
	}

	// inform sender of status:
	RadioMessage* approach = new RadioMessage(inbound, ship, RadioMessageAction::CALL_APPROACH);

	if (inbound_slot->Cleared()) {
		const char* DeckNameAnsi = deck ? deck->Name() : "Unknown Deck";
		const FString Info = FString::Printf(TEXT("Cleared to land on %s"), ANSI_TO_TCHAR(DeckNameAnsi));
		approach->SetInfo(TCHAR_TO_ANSI(*Info));
	}
	else if (sequence) {
		const char* DeckNameAnsi = deck ? deck->Name() : "Unknown Deck";
		const FString Info = FString::Printf(TEXT("Sequenced to land %d on %s"), sequence, ANSI_TO_TCHAR(DeckNameAnsi));
		approach->SetInfo(TCHAR_TO_ANSI(*Info));
	}

	RadioTraffic::Transmit(approach);

	return false;
}

bool
RadioHandler::Picture(RadioMessage* msg, Ship* ship)
{
	if (!ship) return false;

	// try to find some enemy fighters in the area:
	Ship* tgt = 0;
	double      range = 1e9;

	ListIter<SimContact> iter = ship->GetContactList();
	while (++iter) {
		SimContact* c = iter.value();
		int      iff = c->GetIFF(ship);
		Ship* s = c->GetShip();

		if (s && s->IsDropship() && s->IsHostileTo(ship)) {
			const FVector Delta = msg->GetSender()->GetLocation() - s->GetLocation();
			const double s_range = (double)Delta.Size();
			if (!tgt || s_range < range) {
				tgt = s;
				range = s_range;
			}
		}
	}

	// found some:
	if (tgt) {
		SimElement* sender = msg->GetSender()->GetElement();
		SimElement* tgt_elem = tgt->GetElement();
		RadioMessage* response = new RadioMessage(sender, ship, RadioMessageAction::ATTACK);

		if (tgt_elem) {
			for (int i = 1; i <= tgt_elem->NumShips(); i++)
				response->AddTarget(tgt_elem->GetShip(i));
		}
		else {
			response->AddTarget(tgt);
		}

		RadioTraffic::Transmit(response);
	}

	// nobody worth killin':
	else {
		Ship* sender = (Ship*)msg->GetSender();  // cast-away const
		RadioMessage* response = new RadioMessage(sender, ship, RadioMessageAction::PICTURE);
		RadioTraffic::Transmit(response);
	}

	return false;
}

bool
RadioHandler::Support(RadioMessage* msg, Ship* ship)
{
	if (!ship) return false;

	// try to find some fighters with time on their hands...
	SimElement* help = 0;
	SimElement* cmdr = ship->GetElement();
	SimElement* baby = msg->GetSender()->GetElement();
	SimRegion* rgn = msg->GetSender()->GetRegion();

	for (int i = 0; i < rgn->GetShips().size(); i++) {
		Ship* s = rgn->GetShips().at(i);
		SimElement* e = s->GetElement();

		if (e && s->IsDropship() &&
			e->Type() == (int)EMISSIONTYPE::PATROL &&
			e != baby &&
			cmdr->CanCommand(e) &&
			s->GetRadioOrders()->GetRadioAction() == RadioMessageAction::NONE) {
			help = e;
			break;
		}
	}

	// found some:
	if (help) {
		RadioMessage* escort = new RadioMessage(help, ship, RadioMessageAction::ESCORT);
		escort->TargetList().append(msg->GetSender());
		RadioTraffic::Transmit(escort);

		Text ok = "Help Enroute";
		Ship* sender = (Ship*)msg->GetSender();  // cast-away const
		RadioMessage* response = new RadioMessage(sender, ship, RadioMessageAction::ACK);
		response->SetInfo(ok);
		RadioTraffic::Transmit(response);
	}

	// no help in sight:
	else {
		Text nope = "No help available";
		Ship* sender = (Ship*)msg->GetSender();  // cast-away const
		RadioMessage* response = new RadioMessage(sender, ship, RadioMessageAction::NACK);
		response->SetInfo(nope);
		RadioTraffic::Transmit(response);
	}

	return false;
}

// +----------------------------------------------------------------------+

void
RadioHandler::AcknowledgeMessage(RadioMessage* msg, Ship* s)
{
	if (!s || !msg || !msg->GetSender())
		return;

	const RadioMessageAction Action = msg->GetRadioAction();

	// If it's already an ACK/NACK, don't acknowledge it again:
	if (Action >= RadioMessageAction::ACK && Action <= RadioMessageAction::NACK)
		return;

	Ship* sender = (Ship*)msg->GetSender(); // cast-away const (legacy)
	RadioMessage* ack = new RadioMessage(sender, s, RadioMessageAction::ACK);
	RadioTraffic::Transmit(ack);
}