#pragma once

/*
    Project Starshatter Wars
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

    Shared read-only region view for Mission Navigation and Operations.

    Current implementation:
    - Runtime StarSystem and OrbitalRegion driven
    - Legacy DrawRegion style grid and rep scaling
    - Mission Navigation renders visible mission elements and nav routes
    - Operations mode renders major structures only (stations/starbases)
    - Operations mode suppresses ships, fighters, squadrons, and nav routes
    - Click picking respects the active display mode
    - Supports right panel -> center on selected element
    - No editing
*/

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "GameStructs.h"
#include "SectorMapPanel.generated.h"

class UCanvasPanel;
class UTexture2D;
class UMissionNavDlg;

class StarSystem;
class OrbitalRegion;
class Mission;
class MissionElement;

DECLARE_DELEGATE_OneParam(
    FSectorElementSelectedDelegate,
    MissionElement*);

DECLARE_DELEGATE_OneParam(
    FSectorOperationsGroupSelectedDelegate,
    const FS_CombatGroup*);

struct FShipDesign;

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
    // Generic callback used by Operations or any future host.
    // Existing Mission Navigation ownership remains supported as a fallback.
    FSectorElementSelectedDelegate OnElementSelected;

    // Operations uses persistent/static combat-group data.
    FSectorOperationsGroupSelectedDelegate OnOperationsGroupSelected;

    void SetOwnerNavDlg(UMissionNavDlg* InOwnerNavDlg);

    void SetOperationsView(bool bInOperationsView);
    bool IsOperationsView() const { return bOperationsView; }

    void SetViewedSystemName(const FString& InSystemName);
    void SetViewedSectorName(const FString& InSectorName);
    void SetMission(Mission* InMission);
    void SetSelectedElement(MissionElement* InElement);
    void SetSelectedOperationsGroup(const FS_CombatGroup* InGroup);

    bool CenterOnElement(MissionElement* InElement);
    bool CenterOnOperationsGroup(const FS_CombatGroup* InGroup);

    const FS_CombatGroup* GetSelectedOperationsGroup() const { return SelectedOperationsGroup; }

    void ZoomIn();
    void ZoomOut();

    const FString& GetViewedSystemName() const { return ViewedSystemName; }
    const FString& GetViewedSectorName() const { return ViewedSectorName; }
    double ResolveElementHeadingRadians(MissionElement* Element) const;
    bool ShouldShowMissionElementInBriefing(const MissionElement* Element) const;

protected:
    void BuildRuntimeLayout();
    void RefreshView();

    bool ResolveViewedSystem(StarSystem*& OutSystem) const;
    bool ResolveViewedRegion(StarSystem* InSystem, OrbitalRegion*& OutRegion) const;

    float GetElementSpriteSize(const MissionElement* Element, int32 Rep) const;

    bool ShouldRenderElement(MissionElement* Element) const;
    bool IsMajorStructureElement(MissionElement* Element) const;

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


    // ------------------------------------------------------------
    // Operations/static-data path
    // ------------------------------------------------------------

    void DrawOperationsGroups(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep) const;

    void DrawOperationsGroup(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep,
        const FS_CombatGroup* Group) const;

    const FS_CombatGroup* HitTestOperationsGroupAtLocalPoint(
        const FVector2D& LocalPoint,
        const FVector2D& Center,
        float Scale) const;

    bool FindOperationsGroupScreenPosition(
        const FS_CombatGroup* Group,
        const FVector2D& Center,
        float Scale,
        FVector2D& OutScreenPos) const;

    bool OperationsGroupMatchesView(const FS_CombatGroup& Group) const;
    bool IsOperationsGroupCrowded(const FS_CombatGroup* TestGroup, float Scale) const;

    void DrawSelectedOperationsGroupTag(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 LayerId,
        const FVector2D& ScreenPos,
        const FS_CombatGroup* Group) const;

    FLinearColor GetOperationsIFFColor(const FS_CombatGroup* Group) const;

   

    void DrawMissionNavRoutes(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep) const;

    void DrawMissionNavRouteForElement(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 BaseLayerId,
        const FVector2D& Center,
        float Scale,
        int32 Rep,
        MissionElement* Element) const;

    void DrawSelectionCrosshair(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 LayerId,
        const FVector2D& Center,
        float Radius) const;

    bool IsElementCrowded(MissionElement* TestElement, float Scale) const;

    MissionElement* HitTestMissionElementAtLocalPoint(
        const FVector2D& LocalPoint,
        const FVector2D& Center,
        float Scale) const;

    bool FindMissionElementScreenPosition(
        MissionElement* Element,
        const FVector2D& Center,
        float Scale,
        FVector2D& OutScreenPos) const;

    void DrawSelectedElementTag(
        FSlateWindowElementList& OutDrawElements,
        const FGeometry& AllottedGeometry,
        int32 LayerId,
        const FVector2D& ScreenPos,
        MissionElement* Element) const;

    int32 ComputeRepLevel(double ZoomedRadius) const;
    FVector2D ClampPanOffset(const FVector2D& InOffset, const FVector2D& PanelSize) const;
    FLinearColor GetMapIFFColor(const MissionElement* Element) const;

    // ------------------------------------------------------------
    // Sprite system (NEW)
    // ------------------------------------------------------------

    UTexture2D* GetShipMapSprite(const FString& ShipName, const FString& SpriteName);
    int32 ComputeFacingIndex(float YawRadians) const;
    const FShipDesign* ResolveShipDesignForElement(MissionElement* Element) const;



protected:
    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    FString ViewedSystemName;

    UPROPERTY()
    FString ViewedSectorName;

    UMissionNavDlg* OwnerNavDlg = nullptr;

    StarSystem* CachedRuntimeSystem = nullptr;
    OrbitalRegion* CachedRegion = nullptr;
    Mission* CachedMission = nullptr;
    MissionElement* SelectedElement = nullptr;

    // Points into CombatGroupRegistry, which owns the static roster rows.
    const FS_CombatGroup* SelectedOperationsGroup = nullptr;

    double GetDisplayRadius() const;
    bool bOperationsView = false;
    bool bValidView = false;

    bool bDraggingMap = false;
    FVector2D DragStartScreenPosition = FVector2D::ZeroVector;
    FVector2D DragStartPanOffset = FVector2D::ZeroVector;
    FVector2D PanOffset = FVector2D::ZeroVector;

    float ZoomScale = 12.0f;
    float MinZoomScale = 0.25f;
    float MaxZoomScale = 96.0f;

    // ------------------------------------------------------------
    // Sprite cache (NEW)
    // ------------------------------------------------------------

    UPROPERTY()
    TMap<FString, UTexture2D*> MapSpriteCache;
};