/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         SectorMapPanel.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    USectorMapPanel

    Sector-level navigation panel hosted by UMissionNavDlg.
    This is currently a shell widget that will later render
    mission-space objects, regions, stations, and tactical data.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SectorMapPanel.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

UCLASS()
class STARSHATTERWARS_API USectorMapPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void SetViewedSystemName(const FString& InSystemName);
    void SetViewedSectorName(const FString& InSectorName);

    const FString& GetViewedSystemName() const { return ViewedSystemName; }
    const FString& GetViewedSectorName() const { return ViewedSectorName; }

protected:
    void BuildRuntimeLayout();
    void RefreshView();

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UBorder* RootBorder = nullptr;

    UPROPERTY()
    UVerticalBox* ContentBox = nullptr;

    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    UPROPERTY()
    UTextBlock* BodyText = nullptr;

    UPROPERTY()
    FString ViewedSystemName;

    UPROPERTY()
    FString ViewedSectorName;
};