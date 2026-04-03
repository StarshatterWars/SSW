/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    UI / Mission Briefing
    FILE:         MissionNavObjectListView.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UMissionNavObjectListView.
*/

#include "MissionNavObjectListView.h"
#include "MissionNavObjectLVElement.h"

UMissionNavObjectListView::UMissionNavObjectListView(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    EntryWidgetClass = UMissionNavObjectLVElement::StaticClass();
}

void UMissionNavObjectListView::SetEntryWidgetClassPublic(TSubclassOf<UUserWidget> InEntryWidgetClass)
{
    EntryWidgetClass = InEntryWidgetClass;
}