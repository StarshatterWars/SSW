/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioMessage.h
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC


	OVERVIEW
	========
	RadioMessage (radio comms) class declaration
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "Instruction.h"
#include "List.h"
#include "Text.h"

// Minimal Unreal include required for FVector:
#include "Math/Vector.h"

// +--------------------------------------------------------------------+

class SimElement;
class Ship;
class SimObject;

// +--------------------------------------------------------------------+

class RadioMessage
{
public:
	RadioMessage(Ship* dst, const Ship* sender, RadioMessageAction action);
	RadioMessage(SimElement* dst, const Ship* sender, RadioMessageAction action);
	RadioMessage(const RadioMessage& rm);
	virtual ~RadioMessage();

	// accessors:
	static const char* ActionName(RadioMessageAction a);

	const Ship*			GetSender()          const { return sender; }
	Ship*				DestinationShip() const { return dst_ship; }
	SimElement*			DestinationElem() const { return dst_elem; }
	RadioMessageAction  GetRadioAction()          const { return action; }
	List<SimObject>&	TargetList() { return target_list; }
	const FVector&		GetLocation()        const { return location; }
	const Text&			GetInfo()            const { return info; }
	int					GetChannel()         const { return channel; }

	// mutators:
	void              SetDestinationShip(Ship* s) { dst_ship = s; }
	void              SetDestinationElem(SimElement* e) { dst_elem = e; }
	void              AddTarget(SimObject* s);
	void              SetLocation(const FVector& l) { location = l; }
	void              SetInfo(Text msg) { info = msg; }
	void              SetChannel(int c) { channel = c; }

protected:
	const Ship* sender;
	Ship* dst_ship;
	SimElement* dst_elem;
	RadioMessageAction   action;
	List<SimObject>   target_list;
	FVector           location = FVector::ZeroVector;
	Text              info;
	int               channel;
};
