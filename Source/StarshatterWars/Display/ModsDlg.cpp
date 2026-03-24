/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           ModsDlg.cpp
    AUTHOR:         Carlos Bott

    IMPLEMENTATION
    ==============
    UModsDlg - placeholder Mods page (no-op).
=============================================================================*/

#include "ModsDlg.h"

// Core
#include "Logging/LogMacros.h"

// UMG
#include "Components/TextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogModsDlg, Log, All);

UModsDlg::UModsDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UModsDlg::NativeConstruct()
{
    Super::NativeConstruct();

    if (PlaceholderText)
    {
        PlaceholderText->SetText(FText::FromString(TEXT("MODS NOT IMPLEMENTED YET")));
    }

    UE_LOG(LogModsDlg, Verbose, TEXT("[ModsDlg] Constructed (stub)"));
}

void UModsDlg::LoadFromSettings_Implementation()
{
    UE_LOG(LogModsDlg, Verbose, TEXT("[ModsDlg] LoadFromSettings (stub)"));
}

void UModsDlg::ApplySettings_Implementation()
{
    UE_LOG(LogModsDlg, Verbose, TEXT("[ModsDlg] ApplySettings (stub)"));
}

void UModsDlg::SaveSettings_Implementation()
{
    UE_LOG(LogModsDlg, Verbose, TEXT("[ModsDlg] SaveSettings (stub)"));
}

void UModsDlg::CancelChanges_Implementation()
{
    UE_LOG(LogModsDlg, Verbose, TEXT("[ModsDlg] CancelChanges (stub)"));
}
