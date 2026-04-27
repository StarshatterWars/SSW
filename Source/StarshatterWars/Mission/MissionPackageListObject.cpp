/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPackageListObject.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UMissionPackageListObject.

    Converts MissionElement data into cached FString fields suitable
    for Unreal UI widgets (UListView).

    Uses FShipDesign (DataTable-backed) for display information.
*/

#include "MissionPackageListObject.h"

#include "MissionElement.h"
#include "GameStructs_System.h"

UMissionPackageListObject::UMissionPackageListObject()
{
}

void UMissionPackageListObject::InitFromMissionElement(
    MissionElement* InElement,
    int32 InIndex,
    bool bIsPlayerElement)
{
    ElementPtr = InElement;
    Index = InIndex;

    Marker.Empty();
    ElementName.Empty();
    RoleText.Empty();
    PackageText.Empty();
    ElementID = 0;

    if (!ElementPtr)
    {
        return;
    }

    const FShipDesign* Design = ElementPtr->GetShipDesign();

    // No more ASCII marker text here. The row widget will render the arrow texture.
    Marker = bIsPlayerElement ? TEXT("PLAYER") : TEXT("");

    ElementName = ANSI_TO_TCHAR(ElementPtr->GetName().data());
    RoleText = ANSI_TO_TCHAR(ElementPtr->RoleName().data());
    ElementID = ElementPtr->GetElementID();

    if (!Design)
    {
        PackageText = TEXT("UNKNOWN");
        return;
    }

    // Fighters: match MissionNav formatting, e.g. "2x F-32"
    if (ElementPtr->IsSquadron())
    {
        const int32 TotalCount = ElementPtr->Count();

        FString FighterType;

        if (!Design->DisplayName.IsEmpty())
        {
            FighterType = Design->DisplayName;
        }
        else if (!Design->ShipName.IsEmpty())
        {
            FighterType = Design->ShipName;
        }
        else if (!Design->Abrv.IsEmpty())
        {
            FighterType = Design->Abrv;
        }
        else
        {
            FighterType = ElementName.IsEmpty() ? TEXT("FTR") : ElementName;
        }

        if (TotalCount > 0)
        {
            PackageText = FString::Printf(TEXT("%dx %s"), TotalCount, *FighterType);
        }
        else
        {
            PackageText = FighterType;
        }

        return;
    }

    if (ElementPtr->Count() > 1)
    {
        PackageText = FString::Printf(
            TEXT("%d %s"),
            ElementPtr->Count(),
            Design->Abrv.IsEmpty() ? TEXT("UNK") : *Design->Abrv);
    }
    else
    {
        const FString DisplayName =
            !Design->DisplayName.IsEmpty()
            ? Design->DisplayName
            : Design->ShipName;

        PackageText = FString::Printf(
            TEXT("%s %s"),
            Design->Abrv.IsEmpty() ? TEXT("UNK") : *Design->Abrv,
            DisplayName.IsEmpty() ? TEXT("UNKNOWN") : *DisplayName);
    }
}