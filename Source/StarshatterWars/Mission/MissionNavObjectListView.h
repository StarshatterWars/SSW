/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectListView.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Custom ListView for the top-right Mission Navigation object list.

    This class assigns its entry widget class in the constructor so the
    ListView is valid immediately when created.
*/

#pragma once

#include "CoreMinimal.h"
#include "Components/ListView.h"
#include "MissionNavObjectListView.generated.h"

UCLASS()
class STARSHATTERWARS_API UMissionNavObjectListView : public UListView
{
    GENERATED_BODY()

public:
    UMissionNavObjectListView(const FObjectInitializer& ObjectInitializer);

    void SetEntryWidgetClassPublic(TSubclassOf<UUserWidget> InEntryWidgetClass);
};