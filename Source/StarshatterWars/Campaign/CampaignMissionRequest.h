// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"

// Forward declarations (match your port style: plain C++ classes, notxUObjects)
class Campaign;
class CombatGroup;

/**
 * Mirror of Starshatter CampaignMissionRequest.
 * A lightweight "mission generation request" object that planners / campaign logic can pass around.
 *
 * UE Port Notes:
 * - Starshatter used Text + Point; we use FString + FVector.
 * - CombatGroup pointers are plain C++ pointers (no UPROPERTY).
 * - Keep method names as close to original as practical.
 */
class CampaignMissionRequest
{
public:
	static const char* TYPENAME() { return "CampaignMissionRequest"; }

	CampaignMissionRequest(
		Campaign* InCampaign,
		int32 InType,
		int32 InStart,
		CombatGroup* InPrimary,
		CombatGroup* InTarget = nullptr
	);

	// Accessors (mirror)
	Campaign* GetCampaign()       const { return CampaignObj; }
	int32        Type()              const { return MissionType; }
	int32        OpposingType()      const { return OppType; }
	int32        StartTime()         const { return Start; }

	CombatGroup* GetPrimaryGroup()   const { return PrimaryGroup; }
	CombatGroup* GetSecondaryGroup() const { return SecondaryGroup; }
	CombatGroup* GetObjective()      const { return Objective; }

	bool         IsLocSpecified()    const { return bUseLoc; }
	const FString& RegionName()      const { return Region; }
	const FVector& Location()        const { return Location3D; }
	const FString& Script()          const { return ScriptName; }

	// Mutators (mirror)
	void SetType(int32 T) { MissionType = T; }
	void SetOpposingType(int32 T) { OppType = T; }
	void SetStartTime(int32 S) { Start = S; }

	void SetPrimaryGroup(CombatGroup* G) { PrimaryGroup = G; }
	void SetSecondaryGroup(CombatGroup* G) { SecondaryGroup = G; }
	void SetObjective(CombatGroup* G) { Objective = G; }

	// Location/script (mirror semantics: setting either location or region implies "use_loc = true")
	void SetRegionName(const FString& InRegion) { Region = InRegion; bUseLoc = true; }
	void SetLocation(const FVector& InLoc) { Location3D = InLoc; bUseLoc = true; }
	void SetScript(const FString& InScript) { ScriptName = InScript; }

private:
	Campaign* CampaignObj = nullptr;

	int32 MissionType = 0;          // type of mission
	int32 OppType = -1;         // opposing mission type
	int32 Start = 0;          // start time (campaign seconds, same unit as original)

	CombatGroup* PrimaryGroup = nullptr;  // player's group
	CombatGroup* SecondaryGroup = nullptr;  // optional support group
	CombatGroup* Objective = nullptr;  // target or ward

	bool    bUseLoc = false;     // use the specified location
	FString Region;
	FVector Location3D = FVector::ZeroVector;
	FString ScriptName;
};

