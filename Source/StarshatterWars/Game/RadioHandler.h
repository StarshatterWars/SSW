/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioHandler.h
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC


	OVERVIEW
	========
	RadioHandler (radio comms) class declaration
*/

#pragma once

#include "Types.h"
#include "SimObject.h"

// Minimal Unreal include (Vec3/Point -> FVector conversion):
#include "Math/Vector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class RadioMessage;
class Ship;

// +--------------------------------------------------------------------+

class RadioHandler
{
public:
	RadioHandler();
	virtual ~RadioHandler();

	virtual bool      ProcessMessage(RadioMessage* msg, Ship* s);
	virtual void      AcknowledgeMessage(RadioMessage* msg, Ship* s);

protected:
	virtual bool      IsOrder(RadioMessageAction action);
	virtual bool      ProcessMessageOrders(RadioMessage* msg, Ship* s);
	virtual bool      ProcessMessageAction(RadioMessage* msg, Ship* s);

	virtual bool      Inbound(RadioMessage* msg, Ship* s);
	virtual bool      Picture(RadioMessage* msg, Ship* s);
	virtual bool      Support(RadioMessage* msg, Ship* s);
	virtual bool      SkipNavpoint(RadioMessage* msg, Ship* s);
	virtual bool      LaunchProbe(RadioMessage* msg, Ship* s);
};
