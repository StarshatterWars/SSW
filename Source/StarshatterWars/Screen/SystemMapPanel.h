/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         SystemMapPanel.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    USystemMapPanel

    System-level navigation panel hosted by UMissionNavDlg.
    Displays the currently selected star system and will later
    render the full orbital/system map.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SystemMapPanel.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;

UCLASS()
class STARSHATTERWARS_API USystemMapPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void SetViewedSystemName(const FString& InSystemName);
    const FString& GetViewedSystemName() const { return ViewedSystemName; }

protected:
    void BuildRuntimeLayout();
    void RefreshView();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UBorder* RootBorder = nullptr;

    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    UPROPERTY()
    UTextBlock* BodyText = nullptr;

    UPROPERTY()
    FString ViewedSystemName;
};
