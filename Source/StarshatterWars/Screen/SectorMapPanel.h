#pragma once

/*
    Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026.

    SUBSYSTEM:    UI
    FILE:         SectorMapPanel.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Sector (Region) Map Panel - Read Only Pass 2

    - Runtime StarSystem + OrbitalRegion driven
    - Legacy DrawRegion() style grid and rep scaling
    - Read-only mission element rendering
    - No nav editing
*/

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SectorMapPanel.generated.h"

class UCanvasPanel;

class StarSystem;
class OrbitalRegion;
class Mission;
class MissionElement;

UCLASS()
class STARSHATTERWARS_API USectorMapPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseMove(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseWheel(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

public:
    void SetViewedSystemName(const FString& InSystemName);
    void SetViewedSectorName(const FString& InSectorName);

    void SetMission(Mission* InMission);
    void SetSelectedElement(MissionElement* InElement);

    const FString& GetViewedSystemName() const { return ViewedSystemName; }
    const FString& GetViewedSectorName() const { return ViewedSectorName; }

    void ZoomIn();
    void ZoomOut();

protected:
    void BuildRuntimeLayout();
    void RefreshView();

    bool ResolveViewedSystem(StarSystem*& OutSystem) const;
    bool ResolveViewedRegion(StarSystem* InSystem, OrbitalRegion*& OutRegion) const;

    void DrawRegionGrid(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 LayerId,
        const FVector2D& Center,
        float Scale,
        int32 RegionRadius,
        int32 GridStep) const;

    void DrawMissionElements(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep) const;

    void DrawMissionElement(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep,
        MissionElement* Element) const;

    int32 ComputeRepLevel(double ZoomedRadius) const;

    FVector2D ClampPanOffset(const FVector2D& InOffset, const FVector2D& PanelSize) const;



protected:
    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    FString ViewedSystemName;

    UPROPERTY()
    FString ViewedSectorName;

    StarSystem* CachedRuntimeSystem = nullptr;
    OrbitalRegion* CachedRegion = nullptr;

    Mission* CachedMission = nullptr;
    MissionElement* SelectedElement = nullptr;

    bool bValidView = false;

    bool bDraggingMap = false;
    FVector2D DragStartScreenPosition = FVector2D::ZeroVector;
    FVector2D DragStartPanOffset = FVector2D::ZeroVector;
    FVector2D PanOffset = FVector2D::ZeroVector;

    float ZoomScale = 1.0f;
    float MinZoomScale = 0.25f;
    float MaxZoomScale = 32.0f;
};