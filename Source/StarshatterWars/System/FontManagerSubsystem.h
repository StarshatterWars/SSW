/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    StarshatterWars (Unreal Engine)
    FILE:         FontManagerSubsystem.h
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Unreal-native Font Manager
    - GameInstance subsystem (persistent across maps)
    - Registry of named Slate fonts
    - Replaces legacy SystemFont usage
    - GC-safe, asset-based, UMG/Slate compatible
*/

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Fonts/SlateFontInfo.h"
#include "FontManagerSubsystem.generated.h"

// +--------------------------------------------------------------------+
// Font Registry Entry
// +--------------------------------------------------------------------+

USTRUCT()
struct FFontItem
{
    GENERATED_BODY()

    UPROPERTY()
    FName Name = NAME_None;

    UPROPERTY()
    FSlateFontInfo FontInfo;
};

// +--------------------------------------------------------------------+
// Font Manager Subsystem
// +--------------------------------------------------------------------+

UCLASS()
class STARSHATTERWARS_API UFontManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Clears the font registry (does not destroy assets)
    UFUNCTION()
    void Close();

    // Register or update a named font
    UFUNCTION()
    void RegisterFont(FName Name, const FSlateFontInfo& FontInfo);

    // Lookup a font by name
    UFUNCTION()
    bool FindFont(FName Name, FSlateFontInfo& OutFontInfo) const;

private:
    // Registered fonts
    UPROPERTY()
    TArray<FFontItem> Fonts;
};
