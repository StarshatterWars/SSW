/*=============================================================================
    Project:        Starshatter Wars
    Studio:         Fractal Dev Studios
    Copyright:      (c) 2025-2026.
    All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           ModsDlg.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UModsDlg

    Placeholder Mods subpage for the OptionsScreen hub.

    - Lives inside UOptionsScreen via WidgetSwitcher.
    - Currently does nothing (stub) while mods system is implemented.
    - Implements IOptionsPage so OptionsScreen can call Load/Apply/Save/Cancel
      without special cases.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "OptionsPage.h"
#include "ModsDlg.generated.h"

class UTextBlock;

UCLASS()
class STARSHATTERWARS_API UModsDlg : public UUserWidget, public IOptionsPage
{
    GENERATED_BODY()

public:
    UModsDlg(const FObjectInitializer& ObjectInitializer);

protected:
    virtual void NativeConstruct() override;

public:
    // ------------------------------------------------------------
    // IOptionsPage (stub implementations)
    // ------------------------------------------------------------
    virtual void LoadFromSettings_Implementation() override;
    virtual void ApplySettings_Implementation() override;
    virtual void SaveSettings_Implementation() override;
    virtual void CancelChanges_Implementation() override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PlaceholderText;
};
