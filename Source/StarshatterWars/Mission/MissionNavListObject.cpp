/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO
    =========================
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    Stars.exe
    FILE:         MissionNavListObject.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Implementation of UMissionNavListObject.

    Converts legacy Instruction data into cached FString
    fields suitable for Unreal UI widgets (UListView).

    This preserves original Starshatter behavior while eliminating
    lifetime hazards caused by direct legacy nav-list access.
*/

#include "MissionNavListObject.h"

#include "Instruction.h"

UMissionNavListObject::UMissionNavListObject()
{
}

void UMissionNavListObject::InitFromInstruction(
    Instruction* InInstruction,
    int32 InIndex,
    double InDistance)
{
    InstructionPtr = InInstruction;
    Index = InIndex;

    StepText.Empty();
    ActionText.Empty();
    RegionText.Empty();
    DistanceText.Empty();
    SpeedText.Empty();

    if (!InstructionPtr)
        return;

    StepText = FString::FromInt(InIndex + 1);
    ActionText = ANSI_TO_TCHAR(Instruction::ActionName(InstructionPtr->GetAction()));
    RegionText = ANSI_TO_TCHAR(InstructionPtr->RegionName());
    DistanceText = FString::Printf(TEXT("%.0f"), InDistance);
    SpeedText = FString::FromInt(InstructionPtr->Speed());
}