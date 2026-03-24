/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CampaignPlan.h
	AUTHOR:       Carlos Bott

	UNREAL PORT:
	- Maintains all variables and methods (names, signatures, members).
	- Uses UE-friendly types (FString) while preserving Starshatter class layout.
	- Remains a pure C++ interface (no UObject required).
*/

#pragma once

// Original includes mapped to Unreal-compatible shims:
#include "Types.h"
#include "Text.h"
#include "Term.h"
#include "List.h"

// +--------------------------------------------------------------------+

class Campaign;
class Combatant;
class CombatGroup;
class CombatUnit;

// +--------------------------------------------------------------------+

class CampaignPlan
{
public:
	static const char* TYPENAME() { return "CampaignPlan"; }

	// Maintains signature and default exec_time initialization:
	CampaignPlan(Campaign* c)
		: campaign(c)
		, exec_time(-1e6)
	{
	}

	virtual ~CampaignPlan() {}

	int operator == (const CampaignPlan& p) const { return this == &p; }

	// operations:
	virtual void ExecFrame() {}
	virtual void SetLockout(int seconds) {}

protected:
	Campaign* campaign = nullptr;
	double   exec_time = 0.0;
};
