/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioMessage.cpp
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC


	OVERVIEW
	========
	Radio communication message class implementation
*/

#include "RadioMessage.h"

#include "Ship.h"
#include "Text.h"

// Minimal Unreal includes:
#include "Math/Vector.h"

// NOTE: MemDebug.h removed (not supported in Unreal).

// +--------------------------------------------------------------------+

RadioMessage::RadioMessage(Ship* dst, const Ship* s, RadioMessageAction a)
	: sender(s)
	, dst_ship(dst)
	, dst_elem(nullptr)
	, action(a)
	, location(FVector::ZeroVector)
	, channel(0)
{
	if (s)
		channel = s->GetIFF();
}

RadioMessage::RadioMessage(SimElement* dst, const Ship* s, RadioMessageAction a)
	: sender(s)
	, dst_ship(nullptr)
	, dst_elem(dst)
	, action(a)
	, location(FVector::ZeroVector)
	, channel(0)
{
	if (s)
		channel = s->GetIFF();
}

RadioMessage::RadioMessage(const RadioMessage& rm)
	: sender(rm.sender)
	, dst_ship(rm.dst_ship)
	, dst_elem(rm.dst_elem)
	, action(rm.action)
	, target_list()
	, location(rm.location)
	, info(rm.info)
	, channel(rm.channel)
{
	if (rm.target_list.size() > 0) {
		for (int i = 0; i < rm.target_list.size(); i++) {
			SimObject* obj = rm.target_list.at(i);
			target_list.append(obj);
		}
	}
}

RadioMessage::~RadioMessage()
{
}

// +--------------------------------------------------------------------+

const char* RadioMessage::ActionName(RadioMessageAction a)
{
	if (a == RadioMessageAction::ACK) {
		const int coin = rand();
		if (coin < 10000) return "Acknowledged";
		if (coin < 17000) return "Roger that";
		if (coin < 20000) return "Understood";
		if (coin < 22000) return "Copy that";
		return "Affirmative";
	}

	if (a == RadioMessageAction::DISTRESS) {
		const int coin = rand();
		if (coin < 15000) return "Mayday! Mayday!";
		if (coin < 18000) return "She's breaking up!";
		if (coin < 21000) return "Checking out!";
		return "We're going down!";
	}

	if (a == RadioMessageAction::WARN_ACCIDENT) {
		const int coin = rand();
		if (coin < 15000) return "Check your fire!";
		if (coin < 18000) return "Watch it!";
		if (coin < 21000) return "Hey! We're on your side!";
		return "Confirm your targets!";
	}

	if (a == RadioMessageAction::WARN_TARGETED) {
		const int coin = rand();
		if (coin < 15000) return "Break off immediately!";
		if (coin < 20000) return "Buddy spike!";
		return "Abort! Abort!";
	}

	switch (a) {
	case RadioMessageAction::NONE:     
		return "";

	case RadioMessageAction::NACK:     
		return "Negative, Unable";

	case RadioMessageAction::ATTACK:
		return "Engage";
	case RadioMessageAction::ESCORT:
		return "Escort";
	case RadioMessageAction::BRACKET:  
		return "Bracket";
	case RadioMessageAction::IDENTIFY:
		return "Identify";

	case RadioMessageAction::COVER_ME: 
		return "Cover Me";
	case RadioMessageAction::MOVE_PATROL:    
		return "Vector";
	case RadioMessageAction::SKIP_NAVPOINT:
		return "Skip Navpoint";
	case RadioMessageAction::RESUME_MISSION: 
		return "Resume Mission";
	case RadioMessageAction::RTB: 
		return "Return to Base";
	case RadioMessageAction::DOCK_WITH:   
		return "Dock With";
	case RadioMessageAction::QUANTUM_TO:    
		return "Jump to";
	case RadioMessageAction::FARCAST_TO:  
		return "Farcast to";

	case RadioMessageAction::GO_DIAMOND:  
		return "Goto Diamond Formation";
	case RadioMessageAction::GO_SPREAD:   
		return "Goto Spread Formation";
	case RadioMessageAction::GO_BOX:   
		return "Goto Box Formation";
	case RadioMessageAction::GO_TRAIL:  
		return "Goto Trail Formation";

	case RadioMessageAction::WEP_FREE:
		return "Break and Attack";
	case RadioMessageAction::WEP_HOLD:
		return "Hold All Weapons";
	case RadioMessageAction::FORM_UP: 
		return "Return to Formation";
	case RadioMessageAction::SAY_POSITION: 
		return "Say Your Position";

	case RadioMessageAction::LAUNCH_PROBE: 
		return "Launch Probe";
	case RadioMessageAction::GO_EMCON1:  
		return "Goto EMCON 1";
	case RadioMessageAction::GO_EMCON2: 
		return "Goto EMCON 2";
	case RadioMessageAction::GO_EMCON3: 
		return "Goto EMCON 3";

	case RadioMessageAction::REQUEST_PICTURE: 
		return "Request Picture";
	case RadioMessageAction::REQUEST_SUPPORT:
		return "Request Support";
	case RadioMessageAction::PICTURE: 
		return "Picture is clear";

	case RadioMessageAction::CALL_INBOUND:
		return "Calling Inbound";
	case RadioMessageAction::CALL_APPROACH:
		return "Roger your approach";
	case RadioMessageAction::CALL_CLEARANCE:  
		return "You have clearance";
	case RadioMessageAction::CALL_FINALS: 
		return "On final approach";
	case RadioMessageAction::CALL_WAVE_OFF:
		return "Wave off - Runway is closed";

	case RadioMessageAction::DECLARE_ROGUE: 
		return "Prepare to be destroyed!";

	case RadioMessageAction::CALL_ENGAGING:  
		return "Engaging";
	case RadioMessageAction::FOX_1: 
		return "Fox One!";
	case RadioMessageAction::FOX_2: 
		return "Fox Two!";
	case RadioMessageAction::FOX_3:        
		return "Fox Three!";
	case RadioMessageAction::SPLASH_1: 
		return "Splash One!";
	case RadioMessageAction::SPLASH_2: 
		return "Splash Two!";
	case RadioMessageAction::SPLASH_3:   
		return "Splash Three!";
	case RadioMessageAction::SPLASH_4: 
		return "Splash Four!";
	case RadioMessageAction::SPLASH_5:     
		return "Target Destroyed!";
	case RadioMessageAction::SPLASH_6:
		return "Enemy Destroyed!";
	case RadioMessageAction::SPLASH_7:  
		return "Confirmed Kill!";
	case RadioMessageAction::BREAK_ORBIT: 
		return "Breaking Orbit";
	case RadioMessageAction::MAKE_ORBIT:    
		return "Heading for Orbit";
	case RadioMessageAction::QUANTUM_JUMP: 
		return "Going Quantum";

	default:      
		return "Unknown";
	}
}

// +--------------------------------------------------------------------+

void RadioMessage::AddTarget(SimObject* obj)
{
	if (obj && !target_list.contains(obj)) {
		target_list.append(obj);
	}
}
