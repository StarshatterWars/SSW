/*  Project Starshatter Wars
	Fractal Dev Studios
	Copyright (c) 2025-2026. All Rights Reserved.

	ORIGINAL AUTHOR AND STUDIO
	==========================
	John DiCamillo / Destroyer Studios LLC

	SUBSYSTEM:    UI / Operations
	FILE:         MissionLVElement.cpp
	AUTHOR:       Carlos Bott

	OVERVIEW
	========
	UMissionLVElement

	Runtime ListView row widget for command-screen missions.
*/

#include "MissionLVElement.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UMissionLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	MissionList = Cast<UMissionListObject>(ListItemObject);
	if (!MissionList)
	{
		bRowSelected = false;
		ApplySelectionVisual();
		return;
	}

	if (MissionName)
	{
		MissionName->SetText(FText::FromString(MissionList->MissionName));
	}

	if (MissionStatus)
	{
		MissionStatus->SetText(FText::FromString(MissionList->MissionStatus));
	}

	if (MissionTime)
	{
		MissionTime->SetText(FText::FromString(MissionList->MissionTime));
	}

	if (MissionType)
	{
		MissionType->SetText(FText::FromString(MissionList->MissionType));
	}

	// Important: recycled entries must start from current cached state
	ApplySelectionVisual();
}

void UMissionLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	bRowSelected = bIsSelected;
	ApplySelectionVisual();
}

void UMissionLVElement::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	MissionList = nullptr;
	bRowSelected = false;
	ApplySelectionVisual();
}

void UMissionLVElement::ApplySelectionVisual()
{
	if (!SelectionBorder)
	{
		return;
	}

	if (bRowSelected)
	{
		SelectionBorder->SetBrushColor(FLinearColor(0.0f, 0.7f, 1.0f, 0.35f));
	}
	else
	{
		SelectionBorder->SetBrushColor(FLinearColor(0, 0, 0, 0));
	}
}