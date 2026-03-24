/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (C) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         RadioTraffic.h
	AUTHOR:       Carlos Bott
	ORIGINAL AUTHOR AND STUDIO: John DiCamillo / Destroyer Studios LLC


	OVERVIEW
	========
	RadioTraffic maintains a history of all messages sent between ships
	in the simulation.  This class also handles displaying relevant
	traffic to the player.
*/

#pragma once

#include "Types.h"
#include "SimObject.h"
#include "List.h"
#include "Text.h"

// Minimal Unreal include (Vec3/Point -> FVector conversion if needed by inline callers):
#include "Math/Vector.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class SimElement;
class RadioMessage;
class Ship;
class SimObject;

// +--------------------------------------------------------------------+

class RadioTraffic
{
public:
	RadioTraffic();
	~RadioTraffic();

	// accessors:
	static void          Initialize();
	static void          Close();

	static RadioTraffic* GetInstance() { return radio_traffic; }

	static void          SendQuickMessage(Ship* ship, RadioMessageAction msg);
	static void          Transmit(RadioMessage* msg);
	static void          DiscardMessages();
	static Text          TranslateVox(const char* phrase);

	void                 SendRadioMessage(RadioMessage* msg);
	void                 DisplayMessage(RadioMessage* msg);

protected:
	List<RadioMessage>   traffic;

	static RadioTraffic* radio_traffic;
};
