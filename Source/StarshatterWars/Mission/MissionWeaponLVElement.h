/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionWeaponLVElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject-backed ListView row widget for mission weapon entries.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionWeaponListObject.h"
#include "MissionWeaponLVElement.generated.h"

class UTextBlock;
class UBorder;

UCLASS()
class STARSHATTERWARS_API UMissionWeaponLVElement
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

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* StationText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AllowedWeaponsText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SelectedWeaponText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* AmmoText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* SelectionBorder = nullptr;

protected:
    UPROPERTY()
    UMissionWeaponListObject* WeaponItem = nullptr;

    bool bRowSelected = false;
};