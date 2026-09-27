/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.

    SUBSYSTEM:      Stars.exe
    FILE:           OptionsManagedPage.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UOptionsManagedPage / IOptionsManagedPage

    Small interface implemented by any Options subpage that wants a back-pointer
    to UOptionsScreen.

    This lets UOptionsScreen iterate UUserWidget pages (from WidgetSwitcher)
    and assign the manager without knowing each concrete class type.

=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OptionsManagedPage.generated.h"

class UOptionsScreen;

UINTERFACE(BlueprintType)
class STARSHATTERWARS_API UOptionsManagedPage : public UInterface
{
    GENERATED_BODY()
};

class STARSHATTERWARS_API IOptionsManagedPage
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Options")
    void SetOptionsManager(UOptionsScreen* InManager);
};