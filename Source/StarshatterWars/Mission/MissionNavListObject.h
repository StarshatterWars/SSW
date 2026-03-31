/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionNavListObject.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UObject row-model for mission navigation lists.

    This class replaces the legacy ListBox row pattern used in
    MsnPkgDlg navigation display. It wraps Instruction data into
    a UObject suitable for UListView while preserving all
    Starshatter navigation semantics.

    This is a lightweight data container:
    - No ownership of Instruction
    - Cached strings for safe UI lifetime
*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionNavListObject.generated.h"

// Forward declarations:
class Instruction;

/**
 * Mission nav row item for UListView.
 * Mirrors legacy nav list entries backed by Instruction.
 */
UCLASS()
class STARSHATTERWARS_API UMissionNavListObject : public UObject
{
    GENERATED_BODY()

public:
    UMissionNavListObject();

    // Initialize from legacy nav instruction:
    void InitFromInstruction(Instruction* InInstruction, int32 InIndex, double InDistance);

    // Accessors:
    const FString& GetStepText() const { return StepText; }
    const FString& GetActionText() const { return ActionText; }
    const FString& GetRegionText() const { return RegionText; }
    const FString& GetDistanceText() const { return DistanceText; }
    const FString& GetSpeedText() const { return SpeedText; }

    int32 GetIndex() const { return Index; }

private:
    // Raw pointer (non-owning):
    Instruction* InstructionPtr = nullptr;

    // Cached UI-safe values:
    FString StepText;
    FString ActionText;
    FString RegionText;
    FString DistanceText;
    FString SpeedText;

    int32 Index = -1;
};