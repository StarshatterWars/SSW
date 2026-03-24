/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    UI / Operations
	FILE:         MissionListObject.h
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	UMissionListObject

	Lightweight UObject used as a ListView item for displaying
	campaign missions in Unreal UMG.

	This object acts as a bridge between:
		- Runtime campaign data (MissionInfo)
		- UI presentation (ListView rows)

	IMPORTANT
	=========
	This class now binds directly to runtime MissionInfo instead of
	FS_CampaignMissionList. This allows the UI to reflect the active
	campaign state managed by Campaign::GetCampaign().

	NOTE:
	MissionInfoPtr is NOT a UPROPERTY because MissionInfo is not a UObject.
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MissionListObject.generated.h"

class MissionInfo;

// +--------------------------------------------------------------------+

UCLASS(Blueprintable, BlueprintType)
class STARSHATTERWARS_API UMissionListObject : public UObject
{
	GENERATED_BODY()

public:

	// =========================
	// Display Fields (UMG Bindable)
	// =========================

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionSitrep;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionDesc;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionRegion;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionSystem;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionObjective;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionStatus;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionTime;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	FString MissionType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "List Item")
	int32 MissionId = 0;

	// =========================
	// Runtime Binding (NON-UObject)
	// =========================
	// Pointer to the runtime MissionInfo backing this list item.
	// This is used for:
	//   - Description display
	//   - Mission selection
	//   - Accept mission flow
	//
	// DO NOT mark as UPROPERTY (MissionInfo is not a UObject)
	//
	MissionInfo* MissionInfoPtr = nullptr;
};