/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	SUBSYSTEM:    Stars.exe
	FILE:         Combatant.h
	AUTHOR:       Carlos Bott

	ORIGINAL AUTHOR: John DiCamillo
	ORIGINAL STUDIO: Destroyer Studios LLC

	OVERVIEW
	========
	One side in a military conflict
*/

#pragma once

#include "Types.h"
#include "Geometry.h"
#include "Text.h"
#include "List.h"
#include "GameStructs.h"

// +--------------------------------------------------------------------+

class CombatGroup;
class Mission;

// +--------------------------------------------------------------------+

class Combatant
{
public:
	static const char* TYPENAME() { return "Combatant"; }

	Combatant(const char* com_name, const char* file_name, int iff);
	Combatant(const char* com_name, CombatGroup* force);
	~Combatant();

	// operations:

	// accessors:
	const char*				GetName() const { return name; }
	int						GetIFF() const { return iff; }
	int						GetScore() const { return score; }
	const char*				GetDescription() const { return name; }
	CombatGroup*			GetForce() { return force; }
	CombatGroup*			FindGroup(ECOMBATGROUP_TYPE type, int id = -1);
	List<CombatGroup>&		GetTargetList() { return target_list; }
	List<CombatGroup>&		GetDefendList() { return defend_list; }
	List<Mission>&			GetMissionList() { return mission_list; }

	void                    AddMission(Mission* m);
	void                    SetScore(int points) { score = points; }
	void                    AddScore(int points) { score += points; }

	double                  GetTargetStratFactor(ECOMBATGROUP_TYPE type);
	void                    SetTargetStratFactor(ECOMBATGROUP_TYPE type, double f);

private:
	Text                    name;
	int                     iff;
	int                     score;

	CombatGroup* force;
	List<CombatGroup>       target_list;
	List<CombatGroup>       defend_list;
	List<Mission>           mission_list;

	double                  target_factor[8];
};
