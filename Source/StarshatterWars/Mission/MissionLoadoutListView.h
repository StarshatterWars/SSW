/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionLoadoutListView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Small UListView helper that exposes a public setter for the
    protected EntryWidgetClass member in this engine version.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "MissionLoadoutListView.generated.h"

UCLASS()
class STARSHATTERWARS_API UMissionLoadoutListView : public UListView
{
    GENERATED_BODY()

public:
    void SetEntryWidgetClassPublic(TSubclassOf<UUserWidget> InEntryWidgetClass)
    {
        EntryWidgetClass = InEntryWidgetClass;
    }
};