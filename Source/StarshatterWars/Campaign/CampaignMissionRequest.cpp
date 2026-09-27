// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "CampaignMissionRequest.h"
#include "Campaign.h"
#include "CombatGroup.h"

// Forward-declared types (Campaign, CombatGroup) are intentionally notxincluded here.
// Include them only where you actually need their full definitions.

CampaignMissionRequest::CampaignMissionRequest(
	Campaign* InCampaign,
	int32 InType,
	int32 InStart,
	CombatGroup* InPrimary,
	CombatGroup* InTarget
)
	: CampaignObj(InCampaign)
	, MissionType(InType)
	, OppType(-1)
	, Start(InStart)
	, PrimaryGroup(InPrimary)
	, SecondaryGroup(nullptr)
	, Objective(InTarget)
	, bUseLoc(false)
	, Region()
	, Location3D(FVector::ZeroVector)
	, ScriptName()
{
}

