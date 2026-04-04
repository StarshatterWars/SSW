/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         GalaxyMapPanel.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy map panel.

    - Loads galaxy systems from the environment subsystem
    - Caches star textures once
    - Uses origin-centered projection: world (0,0) -> panel center
    - Draws jump links in NativePaint
    - Draws star markers in NativePaint using Slate
    - Draws IFF rings in NativePaint
    - Draws mission-system selection guides
    - Draws highlighted route links
    - Supports zoom and right-mouse panning
    - Supports left-click selection using hit-testing
    - Clips all content to panel bounds
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "GalaxyMapPanel.generated.h"

class UCanvasPanel;
class UTexture2D;
class UMissionNavDlg;
class USystemMarker;

UCLASS()
class STARSHATTERWARS_API UGalaxyMapPanel : public UUserWidget
{
    GENERATED_BODY()

public:
    UGalaxyMapPanel(const FObjectInitializer& ObjectInitializer);

    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

public:
    void SetOwnerNavDlg(UMissionNavDlg* InOwner) { OwnerNavDlg = InOwner; }

    void BuildGalaxyMap(const TArray<FS_Galaxy>& InSystems);
    void ClearGalaxyMap();

    void ZoomIn();
    void ZoomOut();
    void ResetView();

    void SetSelectedSystem(const FString& InSystemName);
    const FString& GetSelectedSystem() const { return SelectedSystemName; }

    void SetCurrentMissionSystem(const FString& InSystemName);
    void SetRoutePath(const TArray<FString>& InRouteSystems);

    FVector2D ProjectToPanel(const FVector& WorldLocation) const;
    FVector2D NormalizeToPanel(const FVector2D& RawXY, const FVector2D& PanelSize) const;
    FVector2D ApplyViewTransformToPoint(const FVector2D& InPoint, const FVector2D& PanelSize) const;

protected:
    void RebuildNormalizationBounds();
    void RefreshSelectionVisuals();

    bool HitTestSystemAtLocalPoint(const FVector2D& LocalPoint, FString& OutSystemName) const;
    FSlateRect GetUsablePanelRect(const FVector2D& PanelSize) const;
    FSlateRect GetClipPanelRect(const FVector2D& PanelSize) const;

    void CacheStarTextures();
    UTexture2D* LoadGalaxyTexture(const TCHAR* AssetPath) const;
    UTexture2D* GetCachedStarTextureForClass(ESPECTRAL_CLASS InClass) const;

    FLinearColor GetIFFRingColor(const FS_Galaxy& SystemRow) const;
    bool IsRouteLink(const FString& A, const FString& B) const;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapRoot = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapCameraRoot = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UCanvasPanel* MapCanvas = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy")
    TSubclassOf<USystemMarker> MarkerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float LeftMargin = 48.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float RightMargin = 48.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float TopMargin = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float BottomMargin = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float MarkerRenderScale = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float VerticalDisplayScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    FVector2D InitialScreenOffset = FVector2D(200.0f, -200.0f);

    UPROPERTY()
    UMissionNavDlg* OwnerNavDlg = nullptr;

    UPROPERTY()
    TArray<FS_Galaxy> GalaxySystems;

    UPROPERTY()
    TMap<FString, FS_Galaxy> SystemLookup;

    UPROPERTY()
    TMap<FString, FVector2D> CachedSystemPositions;

    UPROPERTY()
    TMap<FString, USystemMarker*> MarkerMap;

    UPROPERTY()
    TMap<ESPECTRAL_CLASS, TObjectPtr<UTexture2D>> StarTextureCache;

    UPROPERTY()
    FString SelectedSystemName;

    UPROPERTY()
    FString CurrentMissionSystemName;

    UPROPERTY()
    TArray<FString> RoutePathSystems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float MapZoomLevel = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float MinZoom = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    float MaxZoom = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    FVector2D CurrentPan = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Galaxy Map")
    FVector2D ScreenOffset = FVector2D::ZeroVector;

    UPROPERTY()
    bool bIsPanning = false;

    UPROPERTY()
    FVector2D PanStartMouse = FVector2D::ZeroVector;

    UPROPERTY()
    FVector2D PanStartOffset = FVector2D::ZeroVector;

    float MaxAbsX = 1.0f;
    float MaxAbsY = 1.0f;
    bool bHasBounds = false;
};