/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavLVElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject-backed ListView row widget for mission navigation entries.

    Mirrors the legacy navigation list used by MsnPkgDlg, but adapted
    for Unreal UListView using UMissionNavListObject.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionNavListObject.h"
#include "MissionNavLVElement.generated.h"

class UTextBlock;
class UBorder;

UCLASS()
class STARSHATTERWARS_API UMissionNavLVElement : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* StepText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ActionText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* RegionText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* DistanceText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SpeedText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* SelectionBorder = nullptr;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;

    void ApplySelectionVisual();

protected:
    UPROPERTY()
    UMissionNavListObject* NavItem = nullptr;

    bool bRowSelected = false;
};
