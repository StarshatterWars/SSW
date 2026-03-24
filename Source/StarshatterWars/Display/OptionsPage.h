/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           OptionsPage.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    IOptionsPage

    Optional interface for OptionsScreen subpages.

    Allows OptionsScreen to call:
      - LoadFromSettings
      - ApplySettings
      - SaveSettings
      - CancelChanges

    NOTES
    =====
    - Use BlueprintNativeEvent so C++ pages can implement *_Implementation
    - Keep this interface "thin" and stable (no UI types in signatures)
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OptionsPage.generated.h"

UINTERFACE(BlueprintType)
class STARSHATTERWARS_API UOptionsPage : public UInterface
{
    GENERATED_BODY()
};

class STARSHATTERWARS_API IOptionsPage
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Options")
    void LoadFromSettings();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Options")
    void ApplySettings();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Options")
    void SaveSettings();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Options")
    void CancelChanges();
};
