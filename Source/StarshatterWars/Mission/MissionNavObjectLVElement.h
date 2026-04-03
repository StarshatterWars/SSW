/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectLVElement.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject-backed ListView row widget for the right-side MissionNav object list.

    This widget builds its full row layout in C++ so no Blueprint entry
    widget is required.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "MissionNavObjectListObject.h"
#include "MissionNavObjectLVElement.generated.h"

class UBorder;
class UHorizontalBox;
class USizeBox;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class STARSHATTERWARS_API UMissionNavObjectLVElement : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;
    virtual void NativeOnEntryReleased() override;

    void BuildRuntimeWidget();
    void ApplySelectionVisual();
    void ApplySlotRules();
    void ApplyColumnLayout();
    void ApplyTextRules();

protected:
    UPROPERTY()
    UBorder* SelectionBorder = nullptr;

    UPROPERTY()
    USizeBox* RowSizeBox = nullptr;

    UPROPERTY()
    UHorizontalBox* RowHorizontalBox = nullptr;

    UPROPERTY()
    USizeBox* PrimarySizeBox = nullptr;

    UPROPERTY()
    USizeBox* SecondarySizeBox = nullptr;

    UPROPERTY()
    UTextBlock* PrimaryText = nullptr;

    UPROPERTY()
    UTextBlock* SecondaryText = nullptr;

protected:
    UPROPERTY()
    UMissionNavObjectListObject* ObjectItem = nullptr;

    bool bRowSelected = false;
};