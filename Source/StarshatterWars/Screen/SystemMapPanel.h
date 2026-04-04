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
    System-level map panel for Mission Navigation.

    This panel renders a star system using map-driven data:

    - Uses FS_Galaxy and FS_StarMap as the data source
    - Renders the central star using texture-based rendering
    - Uses spectral-class-based star textures
    - Uses FS_Galaxy::Iff to tint the UI ring around the star
    - Draws larger, clamped, legacy-style tilted orbit ellipses
    - Draws planets above the orbit rings using FS_PlanetMap::Icon
    - Scales planets from FS_PlanetMap::Radius
    - Draws procedural rings for planets that define ring data
    - Provides moon texture lookup using FS_MoonMap::Icon
*/

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "SystemMapPanel.generated.h"

class UCanvasPanel;
class UTextBlock;
class UTexture2D;

UCLASS()
class STARSHATTERWARS_API USystemMapPanel : public UUserWidget
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

public:
    void SetViewedSystemName(const FString& InSystemName);
    const FString& GetViewedSystemName() const { return ViewedSystemName; }

protected:
    void BuildLayout();
    void RefreshView();

    bool ResolveViewedGalaxy(FS_Galaxy& OutGalaxy) const;
    const FS_StarMap* GetPrimaryStarMap(const FS_Galaxy& InGalaxy) const;

    UTexture2D* GetStarTextureForClass(ESPECTRAL_CLASS InClass) const;
    UTexture2D* GetPlanetTexture(const FS_PlanetMap& InPlanet) const;
    UTexture2D* GetMoonTexture(const FS_MoonMap& InMoon) const;
    UTexture2D* LoadPlanetMapTextureByName(const FString& TextureName) const;

    float ComputeStarDrawSize(const FS_StarMap& InStar) const;
    float ComputeRingDrawSize(const FS_StarMap& InStar) const;

    float ComputeMaxDrawOrbitRadius(const FVector2D& PanelSize, float StarSize) const;
    float ComputePlanetOrbitRadius(const FS_PlanetMap& InPlanet, float MaxOrbitInSystem, float MaxDrawRadius) const;
    float ComputePlanetDrawSize(const FS_PlanetMap& InPlanet) const;
    float ComputePlanetAngleRadians(const FS_PlanetMap& InPlanet, int32 PlanetIndex) const;

    float ComputeMoonOrbitRadius(const FS_MoonMap& InMoon, float MaxMoonOrbitForPlanet, float ParentPlanetDrawSize) const;
    float ComputeMoonDrawSize(const FS_MoonMap& InMoon) const;
    float ComputeMoonAngleRadians(const FS_MoonMap& InMoon, int32 MoonIndex) const;

    float ComputeOrbitTiltRadians(const FS_PlanetMap& InPlanet) const;
    float ComputeOrbitVerticalScale(const FS_PlanetMap& InPlanet) const;

    FVector2D ComputeOrbitPosition(
        const FVector2D& SystemCenter,
        float OrbitRadius,
        float OrbitAngleRadians,
        float OrbitTiltRadians,
        float VerticalScale) const;

    FLinearColor ComputeStarTint(const FS_StarMap& InStar) const;
    FLinearColor ComputeSystemIFFRingTint(const FS_Galaxy& InGalaxy) const;

protected:
    UPROPERTY()
    UCanvasPanel* RootCanvas = nullptr;

    UPROPERTY()
    UTextBlock* HeaderText = nullptr;

    UPROPERTY()
    UTextBlock* InfoText = nullptr;

    UPROPERTY()
    FString ViewedSystemName;

    UPROPERTY()
    bool bValidSystem = false;

    UPROPERTY()
    FS_Galaxy CachedGalaxyRow;

    UPROPERTY()
    FS_StarMap CachedPrimaryStarMap;

    UPROPERTY()
    TMap<ESPECTRAL_CLASS, TObjectPtr<UTexture2D>> StarTextureCache;

    UPROPERTY()
    UTexture2D* IFFRingTexture = nullptr;
};