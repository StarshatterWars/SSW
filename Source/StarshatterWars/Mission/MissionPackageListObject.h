/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionPackageListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for mission package lists.

    This class replaces the legacy ListBox row pattern used in
    MsnPkgDlg. It wraps MissionElement data into a UObject suitable
    for UListView while preserving Starshatter package semantics.

    This is a lightweight data container:
    - No ownership of MissionElement
    - Cached strings for safe UI lifetime
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionPackageListObject.generated.h"

// Forward declarations:
class MissionElement;

/**
 * Package list row item for UListView.
 * Mirrors legacy package list entries backed by MissionElement.
 */
UCLASS()
class STARSHATTERWARS_API UMissionPackageListObject : public UObject
{
    GENERATED_BODY()

public:
    UMissionPackageListObject();

    // Initialize from mission element:
    void InitFromMissionElement(
        MissionElement* InElement,
        int32 InIndex,
        bool bIsPlayerElement);

    // Accessors:
    const FString& GetMarker() const { return Marker; }
    const FString& GetElementName() const { return ElementName; }
    const FString& GetRoleText() const { return RoleText; }
    const FString& GetPackageText() const { return PackageText; }

    int32 GetIndex() const { return Index; }
    int32 GetElementID() const { return ElementID; }

private:
    // Raw pointer (non-owning):
    MissionElement* ElementPtr = nullptr;

    // Cached UI-safe values:
    FString Marker;
    FString ElementName;
    FString RoleText;
    FString PackageText;

    int32 Index = INDEX_NONE;
    int32 ElementID = 0;
};