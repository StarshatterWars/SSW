/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionWeaponLoadoutLVElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject-backed ListView row widget for standard mission loadouts.

    Mirrors the top STANDARD LOADOUTS list in the mission weapon panel.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionWeaponLoadoutListObject.h"
#include "MissionWeaponLoadoutLVElement.generated.h"

class UTextBlock;
class UBorder;
class USizeBox;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponLoadoutLVElement
    : public UUserWidget
    , public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;

    void ApplySelectionVisual();
    void ApplyTextRules();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* LoadoutNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* WeightText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* SelectionBorder = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* NameSizeBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    USizeBox* WeightSizeBox = nullptr;

protected:
    UPROPERTY()
    UMissionWeaponLoadoutListObject* LoadoutItem = nullptr;

    bool bRowSelected = false;
};
