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
class UMissionNavDlg;

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
    const FString& GetViewedSystemName() const { return ViewedSystemName; }
    void SetOwnerNavDlg(UMissionNavDlg* InOwner) { OwnerNavDlg = InOwner; }

public:
    void ShowSystemOverview();
    bool CenterOnBodyByName(const FString& InBodyName);

    void ZoomIn();
    void ZoomOut();

    void SetSelectedBodyName(const FString& InName);

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

    UPROPERTY()
    UMissionNavDlg* OwnerNavDlg = nullptr;

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