#pragma once

/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI
    FILE:         SystemMapPanel.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    System-level map panel (runtime StarSystem driven)

    - Uses StarSystem / OrbitalBody (NO DataTables)
    - Renders star, planets, moons from runtime hierarchy
    - Supports zoom, pan, selection
*/

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "Input/Reply.h"

#include "SystemMapPanel.generated.h"

// Forward declarations
class UCanvasPanel;
class UTextBlock;
class UTexture2D;

class StarSystem;
class OrbitalBody;

// ------------------------------------------------------------

// Generic activation callback. Operations uses this to treat a double-click
// on the primary/central star as "return to Galaxy".
DECLARE_DELEGATE_OneParam(
    FSystemPrimaryStarActivatedDelegate,
    const FString&);

// ------------------------------------------------------------

UCLASS()
class STARSHATTERWARS_API USystemMapPanel : public UUserWidget
{
    GENERATED_BODY()

public:
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

    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    virtual FReply NativeOnMouseButtonDoubleClick(
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
    // Fired only when the user double-clicks the primary/central star.
    // Hosts that do not bind this delegate keep their existing behavior.
    FSystemPrimaryStarActivatedDelegate OnPrimaryStarActivated;

    void SetViewedSystemName(const FString& InSystemName);
    const FString& GetViewedSystemName() const { return ViewedSystemName; }
    // Generic owner hook so this panel can be hosted by Mission Navigation,
    // Operations, or another screen without depending on a specific dialog class.
    void SetNavigationOwner(UObject* InOwner) { NavigationOwner = InOwner; }

    // Backward-compatible alias used by the existing MissionNavDlg code.
    // This can be removed after MissionNavDlg is migrated to SetNavigationOwner().
    void SetOwnerNavDlg(UObject* InOwner) { SetNavigationOwner(InOwner); }

public:
    // Shared with the 3D sun camera so rendering and hit testing stay aligned.
    FVector2D GetSystemCenterLocal() const
    {
        const FVector2D Size = GetCachedGeometry().GetLocalSize();
        return FVector2D(Size.X * 0.5f, Size.Y * 0.5f + 30.0f) + PanOffset;
    }

    void ShowSystemOverview();
    bool CenterOnBodyByName(const FString& InBodyName);

    void ZoomIn();
    void ZoomOut();

    void SetSelectedBodyName(const FString& InName);

    /** Enable/disable only the 2D primary-star image. Orbit layout,
        star hit testing, labels, and navigation remain active. */
    void SetDrawPrimaryStar2D(bool bInDraw)
    {
        bDrawPrimaryStar2D = bInDraw;
        Invalidate(EInvalidateWidget::Paint);
    }

protected:

    // ------------------------------------------------------------
    // Layout / View
    // ------------------------------------------------------------
    void BuildLayout();
    void RefreshView();

    bool ResolveViewedSystem(StarSystem*& OutSystem) const;

    // ------------------------------------------------------------
    // Texture helpers
    // ------------------------------------------------------------
    UTexture2D* GetStarTextureForClass(ESPECTRAL_CLASS InClass) const;
    UTexture2D* GetPlanetTexture(const OrbitalBody* InPlanet) const;
    UTexture2D* GetMoonTexture(const OrbitalBody* InMoon) const;
    UTexture2D* LoadPlanetMapTextureByName(const FString& TextureName) const;

    // ------------------------------------------------------------
    // Size / Orbit calculations
    // ------------------------------------------------------------
    float ComputeStarDrawSize(const OrbitalBody* InStar) const;
    float ComputeRingDrawSize(const OrbitalBody* InStar) const;

    float ComputeMaxDrawOrbitRadius(const FVector2D& PanelSize, float StarSize) const;

    float ComputePlanetOrbitRadius(
        const OrbitalBody* InPlanet,
        float MaxOrbitInSystem,
        float MaxDrawRadius) const;

    float ComputePlanetDrawSize(const OrbitalBody* InPlanet) const;
    float ComputePlanetAngleRadians(const OrbitalBody* InPlanet, int32 PlanetIndex) const;

    float ComputeMoonOrbitRadius(
        const OrbitalBody* InMoon,
        float MaxMoonOrbitForPlanet,
        float ParentPlanetDrawSize) const;

    float ComputeMoonDrawSize(const OrbitalBody* InMoon) const;
    float ComputeMoonAngleRadians(const OrbitalBody* InMoon, int32 MoonIndex) const;

    float ComputeOrbitTiltRadians(const OrbitalBody* InPlanet) const;
    float ComputeOrbitVerticalScale(const OrbitalBody* InPlanet) const;

    FVector2D ComputeOrbitPosition(
        const FVector2D& SystemCenter,
        float OrbitRadius,
        float OrbitAngleRadians,
        float OrbitTiltRadians,
        float VerticalScale) const;

    // ------------------------------------------------------------
    // Interaction / Camera
    // ------------------------------------------------------------
    float GetZoomedValue(float InValue) const;
    FVector2D ClampPanOffset(const FVector2D& InOffset, const FVector2D& PanelSize) const;

    void ResetSystemView();
    void FocusOnPlanet(const FVector2D& RelativeOffset, const FVector2D& PanelSize);

    bool HandleClickSelection(const FVector2D& LocalPos, const FGeometry& InGeometry);

    // ------------------------------------------------------------
    // Lookup / selection helpers
    // ------------------------------------------------------------
    bool FindBodyOffsetByName(
        const FString& InBodyName,
        FVector2D& OutUnzoomedOffset) const;

    bool FindBodyScreenPositionByName(
        const FString& InBodyName,
        const FGeometry& AllottedGeometry,
        FVector2D& OutScreenPosition,
        float& OutDrawSize) const;

    // ------------------------------------------------------------
    // Color helpers
    // ------------------------------------------------------------
    FLinearColor ComputeStarTint(const OrbitalBody* InStar) const;
    FLinearColor ComputeSystemIFFRingTint(StarSystem* InSystem) const;

protected:

    // ------------------------------------------------------------
    // UI
    // ------------------------------------------------------------
    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    UPROPERTY()
    UTextBlock* InfoText = nullptr;

    // ------------------------------------------------------------
    // State
    // ------------------------------------------------------------
    UPROPERTY()
    FString ViewedSystemName;

    UPROPERTY()
    bool bValidSystem = false;

    // ------------------------------------------------------------
    // Runtime data (REPLACES DataTables)
    // ------------------------------------------------------------
    StarSystem* CachedRuntimeSystem = nullptr;
    OrbitalBody* CachedPrimaryStarBody = nullptr;

    TArray<OrbitalBody*> CachedPlanetBodies;

    // ------------------------------------------------------------
    // Rendering
    // ------------------------------------------------------------
    UPROPERTY()
    TMap<ESPECTRAL_CLASS, TObjectPtr<UTexture2D>> StarTextureCache;

    UPROPERTY()
    UTexture2D* IFFRingTexture = nullptr;

    // Operations uses a real 3D ACentralSun instead of the Slate star image.
    UPROPERTY()
    bool bDrawPrimaryStar2D = true;

    // ------------------------------------------------------------
    // Camera / Input
    // ------------------------------------------------------------
    UPROPERTY()
    bool bDraggingMap = false;

    UPROPERTY()
    FVector2D DragStartScreenPosition = FVector2D::ZeroVector;

    UPROPERTY()
    FVector2D DragStartPanOffset = FVector2D::ZeroVector;

    UPROPERTY()
    FVector2D PanOffset = FVector2D::ZeroVector;

    // Generic host reference.  The SystemMapPanel currently does not call back
    // into its owner; it is retained only as neutral host context for future use.
    UPROPERTY()
    TObjectPtr<UObject> NavigationOwner = nullptr;

    UPROPERTY()
    float ZoomScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Map")
    float MinZoomScale = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "System Map")
    float MaxZoomScale = 16.0f;

protected:
        float GetFocusZoomForBody(const OrbitalBody* Body) const;

protected:
    bool bCameraAnimating = false;
    FVector2D TargetPan = FVector2D::ZeroVector;
    float TargetZoom = 1.0f;

    float CameraInterpSpeed = 8.0f;
    float FocusMinPlanetZoom = 1.75f;
    float FocusMinMoonZoom = 2.75f;

    // ------------------------------------------------------------
    // Selection
    // ------------------------------------------------------------
    UPROPERTY()
    FString SelectedBodyName;
};