/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionPackageLVElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject-backed ListView row widget for mission package entries.

    Mirrors the legacy package list used by MsnPkgDlg, but adapted
    for Unreal UListView using UMissionPackageListObject.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionPackageListObject.h"
#include "MissionPackageLVElement.generated.h"

class UTextBlock;
class UBorder;
class USizeBox;

UCLASS()
class STARSHATTERWARS_API UMissionPackageLVElement
    : public UUserWidget
    , public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* MarkerText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* ElementNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* RoleText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* PackageText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* SelectionBorder = nullptr;

    // Optional fixed-size layout boxes from the Widget Blueprint:
    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RowSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* MarkerSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* ElementNameSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* RoleSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* PackageSizeBox = nullptr;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;

    void ApplySelectionVisual();
    void ApplyColumnLayout();
    void ApplyTextRules();

protected:
    UPROPERTY()
    UMissionPackageListObject* PackageItem = nullptr;

    bool bRowSelected = false;
};