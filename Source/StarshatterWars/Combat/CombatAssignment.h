/*  Project Starshatter 4.5
	Destroyer Studios LLC
	Copyright © 1997-2004. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         CombatAssignment.h
	AUTHOR:       Carlos Bott

	UNREAL PORT:
	- Maintains all variables and methods (names, signatures, members).
	- Uses UE-compatible shims for List and project Types.
*/

#pragma once

#include "Types.h"
#include "List.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class CombatGroup;
class SimRegion;

// +--------------------------------------------------------------------+

class CombatAssignment
{
public:
	static const char* TYPENAME() { return "CombatAssignment"; }

	CombatAssignment(int t, CombatGroup* obj, CombatGroup* rsc = 0);
	~CombatAssignment();

	int operator < (const CombatAssignment& a) const;

	// operations:
	void                 SetObjective(CombatGroup* o) { objective = o; }
	void                 SetResource(CombatGroup* r) { resource = r; }

	// accessors:
	int                  Type() { return type; }
	CombatGroup* GetObjective() { return objective; }
	CombatGroup* GetResource() { return resource; }

	const char* GetDescription()  const;
	bool                 IsActive()  const { return resource != 0; }

private:
	int                  type = 0;
	CombatGroup* objective = nullptr;
	CombatGroup* resource = nullptr;
};
