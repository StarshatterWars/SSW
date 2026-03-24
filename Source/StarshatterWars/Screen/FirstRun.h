/*=============================================================================
    Project:        Starshatter Wars (nGenEx Unreal Port)
    Studio:         Fractal Dev Games
    Copyright:      (C) 2024–2026. All Rights Reserved.

    SUBSYSTEM:      StarshatterWars (Unreal Engine)
    FILE:           FirstRun.h
    AUTHOR:         Carlos Bott

    OVERVIEW
    ========
    UFirstRun

    First-run player creation dialog.
    - Captures a player name from UI
    - Initializes a new FS_PlayerGameInfo profile
    - Persists the profile via UStarshatterPlayerSubsystem ONLY

    NOTES
    =====
    - No dependency on SSWGameInstance for save/load
    - UI close/routing is intentionally left to the host screen
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "FirstRun.generated.h"

class UButton;
class UTextBlock;
class UEditableTextBox;

UCLASS()
class STARSHATTERWARS_API UFirstRun : public UBaseScreen
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidgetOptional))
    UButton* btn_apply = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UButton* btn_cancel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* FirstRunTitle = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* FirstRunPrompt = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UEditableTextBox* PlayerNameBox = nullptr;

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnApplyClicked();

    UFUNCTION()
    void OnCancelClicked();
};
