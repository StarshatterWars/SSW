/*
    Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2024-2026. All Rights Reserved.

    ORIGINAL AUTHOR AND STUDIO:
    John DiCamillo / Destroyer Studios LLC

    SUBSYSTEM:    UI
    FILE:         SystemMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    System-level map panel for Mission Navigation.

    - Resolves the selected system from UStarshatterEnvironmentSubsystem
    - Uses FS_Galaxy::Stellar as the map hierarchy source
    - Draws the primary star using GalaxyMap textures
    - Tints the surrounding ring using FS_Galaxy::Iff
    - Draws planetary orbit rings
    - Draws planets above those rings using FS_PlanetMap::Icon
    - Scales planets from FS_PlanetMap::Radius
    - Draws moons using FS_MoonMap::Icon
    - Mirrors GalaxyMapPanel input behavior:
        * right mouse drag = pan
        * left click = hit-test / selection
        * mouse wheel = zoom
    - Clicking the star resets the view
    - Clicking a planet centers the view
*/

#include "SystemMapPanel.h"

#include "MissionUIStyle.h"
#include "StarshatterEnvironmentSubsystem.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

#include "Input/Reply.h"
#include "Math/Vector2D.h"
#include "Input/Events.h"

static void DrawOrbitEllipseLines(
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FGeometry& Geometry,
    const FVector2D& Center,
    float OrbitRadius,
    float VerticalScale,
    float TiltRadians,
    const FLinearColor& Color,
    float Thickness = 1.0f,
    int32 Segments = 96)
{
    TArray<FVector2D> OrbitPoints;
    OrbitPoints.Reserve(Segments + 1);

    for (int32 SegmentIndex = 0; SegmentIndex <= Segments; ++SegmentIndex)
    {
        const float Angle = (2.0f * PI * SegmentIndex) / Segments;

        const float BaseX = FMath::Cos(Angle) * OrbitRadius;
        const float BaseY = FMath::Sin(Angle) * OrbitRadius * VerticalScale;

        const float RotX = BaseX * FMath::Cos(TiltRadians) - BaseY * FMath::Sin(TiltRadians);
        const float RotY = BaseX * FMath::Sin(TiltRadians) + BaseY * FMath::Cos(TiltRadians);

        OrbitPoints.Add(FVector2D(
            Center.X + RotX,
            Center.Y + RotY));
    }

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        Geometry.ToPaintGeometry(),
        OrbitPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);
}

static void DrawPlanetRingEllipseLines(
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FGeometry& Geometry,
    const FVector2D& Center,
    float RingRadiusX,
    float RingRadiusY,
    const FLinearColor& Color,
    float Thickness = 1.0f,
    float GapPixels = 1.25f,
    int32 Segments = 72)
{
    auto BuildEllipsePoints =
        [&](float RadiusX, float RadiusY, TArray<FVector2D>& OutPoints)
        {
            OutPoints.Reset();
            OutPoints.Reserve(Segments + 1);

            for (int32 SegmentIndex = 0; SegmentIndex <= Segments; ++SegmentIndex)
            {
                const float Angle = (2.0f * PI * SegmentIndex) / Segments;
                const float OffsetX = FMath::Cos(Angle) * RadiusX;
                const float OffsetY = FMath::Sin(Angle) * RadiusY;

                OutPoints.Add(FVector2D(
                    Center.X + OffsetX,
                    Center.Y + OffsetY));
            }
        };

    TArray<FVector2D> OuterRingPoints;
    TArray<FVector2D> InnerRingPoints;

    BuildEllipsePoints(RingRadiusX, RingRadiusY, OuterRingPoints);
    BuildEllipsePoints(
        FMath::Max(1.0f, RingRadiusX - GapPixels),
        FMath::Max(1.0f, RingRadiusY - GapPixels),
        InnerRingPoints);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        Geometry.ToPaintGeometry(),
        OuterRingPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        Geometry.ToPaintGeometry(),
        InnerRingPoints,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);
}

void USystemMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    BuildLayout();

    if (RootCanvas)
    {
        RootCanvas->SetVisibility(ESlateVisibility::Visible);
        RootCanvas->SetClipping(EWidgetClipping::ClipToBounds);
    }

    ZoomScale = 1.0f;
    PanOffset = FVector2D::ZeroVector;
    bDraggingMap = false;
    DragStartScreenPosition = FVector2D::ZeroVector;
    DragStartPanOffset = FVector2D::ZeroVector;

    StarTextureCache.Empty();

    auto LoadMapTexture = [](const TCHAR* AssetPath) -> UTexture2D*
        {
            if (!AssetPath || !*AssetPath)
            {
                return nullptr;
            }

            UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, AssetPath);
            if (!LoadedTexture)
            {
                UE_LOG(LogTemp, Warning, TEXT("[SystemMapPanel] Failed to load texture: %s"), AssetPath);
            }

            return LoadedTexture;
        };

    IFFRingTexture = LoadMapTexture(TEXT("/Game/UI/GalaxyMap/IFFRing.IFFRing"));

    StarTextureCache.Add(ESPECTRAL_CLASS::A, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarA_map.StarA_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::B, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarB_map.StarB_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::F, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarF_map.StarF_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::G, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::K, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarK_map.StarK_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::M, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarM_map.StarM_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::O, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarO_map.StarO_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::WHITE_DWARF, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/White.White")));
    StarTextureCache.Add(ESPECTRAL_CLASS::RED_GIANT, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::BLACK_HOLE, LoadMapTexture(TEXT("/Game/UI/GalaxyMap/White.White")));

    RefreshView();
}

void USystemMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    ViewedSystemName = InSystemName.TrimStartAndEnd();
    ResetSystemView();
    RefreshView();
    Invalidate(EInvalidateWidget::Paint);
}

void USystemMapPanel::BuildLayout()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!RootCanvas)
    {
        RootCanvas =
            WidgetTree->ConstructWidget<UCanvasPanel>(
                UCanvasPanel::StaticClass(),
                TEXT("SystemMapRootCanvas"));

        RootCanvas->SetVisibility(ESlateVisibility::Visible);

        WidgetTree->RootWidget = RootCanvas;
    }

    if (!HeaderText)
    {
        HeaderText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SystemMapHeaderText"));

        HeaderText->SetFont(MissionUIStyle::GetHeaderFont(16));
        HeaderText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        HeaderText->SetJustification(ETextJustify::Left);

        if (UCanvasPanelSlot* HeaderCanvasSlot = RootCanvas->AddChildToCanvas(HeaderText))
        {
            HeaderCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
            HeaderCanvasSlot->SetPosition(FVector2D(8.f, 8.f));
            HeaderCanvasSlot->SetSize(FVector2D(420.f, 24.f));
        }
    }

    if (!InfoText)
    {
        InfoText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SystemMapInfoText"));

        InfoText->SetFont(MissionUIStyle::GetInfoValueFont());
        InfoText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        InfoText->SetJustification(ETextJustify::Right);

        if (UCanvasPanelSlot* InfoCanvasSlot = RootCanvas->AddChildToCanvas(InfoText))
        {
            InfoCanvasSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
            InfoCanvasSlot->SetAlignment(FVector2D(1.f, 0.f));
            InfoCanvasSlot->SetPosition(FVector2D(-8.f, 8.f));
            InfoCanvasSlot->SetSize(FVector2D(320.f, 24.f));
        }
    }
}

bool USystemMapPanel::ResolveViewedGalaxy(FS_Galaxy& OutGalaxy) const
{
    OutGalaxy = FS_Galaxy();

    if (ViewedSystemName.IsEmpty())
    {
        return false;
    }

    UGameInstance* GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        return false;
    }

    UStarshatterEnvironmentSubsystem* EnvironmentSubsystem =
        GameInstance->GetSubsystem<UStarshatterEnvironmentSubsystem>();

    if (!EnvironmentSubsystem)
    {
        return false;
    }

    for (const FS_Galaxy& GalaxyRow : EnvironmentSubsystem->GalaxyDataArray)
    {
        if (GalaxyRow.Name.Equals(ViewedSystemName, ESearchCase::IgnoreCase))
        {
            OutGalaxy = GalaxyRow;
            return true;
        }
    }

    return false;
}

const FS_StarMap* USystemMapPanel::GetPrimaryStarMap(const FS_Galaxy& InGalaxy) const
{
    if (InGalaxy.Stellar.Num() > 0)
    {
        return &InGalaxy.Stellar[0];
    }

    return nullptr;
}

UTexture2D* USystemMapPanel::GetStarTextureForClass(ESPECTRAL_CLASS InClass) const
{
    if (const TObjectPtr<UTexture2D>* FoundTexture = StarTextureCache.Find(InClass))
    {
        return FoundTexture->Get();
    }

    if (const TObjectPtr<UTexture2D>* FallbackTexture = StarTextureCache.Find(ESPECTRAL_CLASS::G))
    {
        return FallbackTexture->Get();
    }

    return nullptr;
}

UTexture2D* USystemMapPanel::LoadPlanetMapTextureByName(const FString& TextureName) const
{
    if (TextureName.IsEmpty())
    {
        return nullptr;
    }

    const FString AssetPath = FString::Printf(
        TEXT("/Game/UI/PlanetMap/%s.%s"),
        *TextureName,
        *TextureName);

    UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, *AssetPath);
    if (!LoadedTexture)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[SystemMapPanel] Failed to load planet-map texture: %s"),
            *AssetPath);
    }

    return LoadedTexture;
}

UTexture2D* USystemMapPanel::GetPlanetTexture(const FS_PlanetMap& InPlanet) const
{
    return LoadPlanetMapTextureByName(InPlanet.Icon);
}

UTexture2D* USystemMapPanel::GetMoonTexture(const FS_MoonMap& InMoon) const
{
    return LoadPlanetMapTextureByName(InMoon.Icon);
}

float USystemMapPanel::ComputeStarDrawSize(const FS_StarMap& InStar) const
{
    const float RawRadius = static_cast<float>(InStar.Radius);

    if (RawRadius <= 0.0f)
    {
        return 44.0f;
    }

    const float VisualSize = FMath::LogX(10.0f, RawRadius + 1.0f) * 8.0f;
    return FMath::Clamp(VisualSize, 40.0f, 96.0f);
}

float USystemMapPanel::ComputeRingDrawSize(const FS_StarMap& InStar) const
{
    return ComputeStarDrawSize(InStar) + 18.0f;
}

float USystemMapPanel::ComputeMaxDrawOrbitRadius(const FVector2D& PanelSize, float StarSize) const
{
    const float HorizontalLimit = (PanelSize.X * 0.5f) - 60.0f;
    const float VerticalLimit = (PanelSize.Y * 0.5f) - 60.0f;
    const float SafeLimit = FMath::Min(HorizontalLimit, VerticalLimit);

    return FMath::Clamp(SafeLimit - (StarSize * 0.10f), 320.0f, 520.0f);
}

float USystemMapPanel::ComputePlanetOrbitRadius(
    const FS_PlanetMap& InPlanet,
    float MaxOrbitInSystem,
    float MaxDrawRadius) const
{
    const float OrbitValue = static_cast<float>(InPlanet.Orbit);

    if (OrbitValue <= 0.0f || MaxOrbitInSystem <= 0.0f)
    {
        return 0.0f;
    }

    const float NormalizedOrbit =
        FMath::Clamp(OrbitValue / MaxOrbitInSystem, 0.0f, 1.0f);

    const float MinOrbitRadius = 165.0f;
    const float SpreadOrbit = FMath::Pow(NormalizedOrbit, 1.55f);

    return FMath::Lerp(MinOrbitRadius, MaxDrawRadius, SpreadOrbit);
}

float USystemMapPanel::ComputePlanetDrawSize(const FS_PlanetMap& InPlanet) const
{
    const float RawRadius = static_cast<float>(InPlanet.Radius);

    if (RawRadius <= 0.0f)
    {
        return 10.0f;
    }

    const FString IconName = InPlanet.Icon.ToLower();

    const bool bGasGiant =
        IconName.Contains(TEXT("gasgiant")) ||
        RawRadius >= 12.0e6f;

    if (bGasGiant)
    {
        return FMath::Clamp(18.0f + (RawRadius / 1.0e6f), 30.0f, 80.0f);
    }

    return FMath::Clamp(6.0f + (RawRadius / 1.2e6f), 8.0f, 24.0f);
}

float USystemMapPanel::ComputePlanetAngleRadians(const FS_PlanetMap& InPlanet, int32 PlanetIndex) const
{
    const float OrbitAngleDegrees = static_cast<float>(InPlanet.OrbitAngle);

    if (!FMath::IsNearlyZero(OrbitAngleDegrees))
    {
        return FMath::DegreesToRadians(OrbitAngleDegrees);
    }

    return FMath::DegreesToRadians(PlanetIndex * 57.0f);
}

float USystemMapPanel::ComputeMoonOrbitRadius(
    const FS_MoonMap& InMoon,
    float MaxMoonOrbitForPlanet,
    float ParentPlanetDrawSize) const
{
    const float OrbitValue = static_cast<float>(InMoon.Orbit);

    if (OrbitValue <= 0.0f || MaxMoonOrbitForPlanet <= 0.0f)
    {
        return ParentPlanetDrawSize * 0.78f;
    }

    const float NormalizedOrbit =
        FMath::Clamp(OrbitValue / MaxMoonOrbitForPlanet, 0.0f, 1.0f);

    const float MinMoonOrbitRadius = ParentPlanetDrawSize * 0.78f;
    const float MaxMoonOrbitRadius = ParentPlanetDrawSize * 1.20f;

    return FMath::Lerp(MinMoonOrbitRadius, MaxMoonOrbitRadius, NormalizedOrbit);
}

float USystemMapPanel::ComputeMoonDrawSize(const FS_MoonMap& InMoon) const
{
    const float RawRadius = static_cast<float>(InMoon.Radius);

    if (RawRadius <= 0.0f)
    {
        return 3.0f;
    }

    const float VisualSize = FMath::LogX(10.0f, RawRadius + 1.0f) * 2.0f;
    return FMath::Clamp(VisualSize, 2.5f, 7.0f);
}

float USystemMapPanel::ComputeMoonAngleRadians(const FS_MoonMap& InMoon, int32 MoonIndex) const
{
    const float OrbitAngleDegrees = static_cast<float>(InMoon.OrbitAngle);

    if (!FMath::IsNearlyZero(OrbitAngleDegrees))
    {
        return FMath::DegreesToRadians(OrbitAngleDegrees);
    }

    return FMath::DegreesToRadians(MoonIndex * 83.0f);
}

float USystemMapPanel::ComputeOrbitTiltRadians(const FS_PlanetMap& InPlanet) const
{
    return 0.0f;
}

float USystemMapPanel::ComputeOrbitVerticalScale(const FS_PlanetMap& InPlanet) const
{
    return 1.0f;
}

FVector2D USystemMapPanel::ComputeOrbitPosition(
    const FVector2D& SystemCenter,
    float OrbitRadius,
    float OrbitAngleRadians,
    float OrbitTiltRadians,
    float VerticalScale) const
{
    const float BaseX = FMath::Cos(OrbitAngleRadians) * OrbitRadius;
    const float BaseY = FMath::Sin(OrbitAngleRadians) * OrbitRadius * VerticalScale;

    const float RotX = BaseX * FMath::Cos(OrbitTiltRadians) - BaseY * FMath::Sin(OrbitTiltRadians);
    const float RotY = BaseX * FMath::Sin(OrbitTiltRadians) + BaseY * FMath::Cos(OrbitTiltRadians);

    return FVector2D(
        SystemCenter.X + RotX,
        SystemCenter.Y + RotY);
}

float USystemMapPanel::GetZoomedValue(float InValue) const
{
    return InValue * ZoomScale;
}

FVector2D USystemMapPanel::ClampPanOffset(const FVector2D& InOffset, const FVector2D& PanelSize) const
{
    const float MaxPanX = FMath::Max(0.0f, PanelSize.X * (ZoomScale - 1.0f) * 0.35f);
    const float MaxPanY = FMath::Max(0.0f, PanelSize.Y * (ZoomScale - 1.0f) * 0.35f);

    return FVector2D(
        FMath::Clamp(InOffset.X, -MaxPanX, MaxPanX),
        FMath::Clamp(InOffset.Y, -MaxPanY, MaxPanY));
}

FLinearColor USystemMapPanel::ComputeStarTint(const FS_StarMap& InStar) const
{
    if (InStar.Color != FColor(0, 0, 0, 0))
    {
        return FLinearColor(InStar.Color);
    }

    switch (InStar.Class)
    {
    case ESPECTRAL_CLASS::O:           return FLinearColor(0.72f, 0.82f, 1.00f, 1.0f);
    case ESPECTRAL_CLASS::B:           return FLinearColor(0.78f, 0.86f, 1.00f, 1.0f);
    case ESPECTRAL_CLASS::A:           return FLinearColor(0.88f, 0.93f, 1.00f, 1.0f);
    case ESPECTRAL_CLASS::F:           return FLinearColor(1.00f, 0.98f, 0.85f, 1.0f);
    case ESPECTRAL_CLASS::G:           return FLinearColor(1.00f, 0.91f, 0.30f, 1.0f);
    case ESPECTRAL_CLASS::K:           return FLinearColor(1.00f, 0.72f, 0.24f, 1.0f);
    case ESPECTRAL_CLASS::M:           return FLinearColor(1.00f, 0.42f, 0.22f, 1.0f);
    case ESPECTRAL_CLASS::WHITE_DWARF: return FLinearColor(0.92f, 0.96f, 1.00f, 1.0f);
    case ESPECTRAL_CLASS::RED_GIANT:   return FLinearColor(1.00f, 0.40f, 0.20f, 1.0f);
    case ESPECTRAL_CLASS::BLACK_HOLE:  return FLinearColor(0.55f, 0.55f, 0.65f, 1.0f);
    default:                           return FLinearColor(1.00f, 0.90f, 0.35f, 1.0f);
    }
}

FLinearColor USystemMapPanel::ComputeSystemIFFRingTint(const FS_Galaxy& InGalaxy) const
{
    switch (InGalaxy.Iff)
    {
    case 1:  return FLinearColor(0.20f, 1.00f, 0.20f, 0.90f);
    case 2:  return FLinearColor(1.00f, 0.25f, 0.25f, 0.90f);
    case 3:  return FLinearColor(1.00f, 0.85f, 0.25f, 0.90f);
    default: return FLinearColor(0.60f, 0.75f, 1.00f, 0.80f);
    }
}

void USystemMapPanel::RefreshView()
{
    bValidSystem = ResolveViewedGalaxy(CachedGalaxyRow);

    if (HeaderText)
    {
        HeaderText->SetText(FText::FromString(
            ViewedSystemName.IsEmpty()
            ? TEXT("SYSTEM")
            : FString::Printf(TEXT("SYSTEM: %s"), *ViewedSystemName.ToUpper())));
    }

    if (!bValidSystem)
    {
        CachedPrimaryStarMap = FS_StarMap();

        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("NO SYSTEM")));
        }

        return;
    }

    const FS_StarMap* PrimaryStarMap = GetPrimaryStarMap(CachedGalaxyRow);
    if (!PrimaryStarMap)
    {
        CachedPrimaryStarMap = FS_StarMap();

        if (InfoText)
        {
            InfoText->SetText(FText::FromString(TEXT("NO STAR DATA")));
        }

        return;
    }

    CachedPrimaryStarMap = *PrimaryStarMap;

    if (InfoText)
    {
        InfoText->SetText(FText::FromString(FString::Printf(
            TEXT("STAR: %s"),
            *CachedPrimaryStarMap.Name)));
    }
}

bool USystemMapPanel::HandleClickSelection(
    const FVector2D& LocalPos,
    const FGeometry& InGeometry)
{
    if (!bValidSystem || CachedPrimaryStarMap.Name.IsEmpty())
    {
        return false;
    }

    const FVector2D PanelSize = GetCachedGeometry().GetLocalSize();

    const FVector2D BaseCenter(
        PanelSize.X * 0.5f,
        (PanelSize.Y * 0.55f) - 10.0f);

    const FVector2D SystemCenter = BaseCenter + PanOffset;

    const float StarSize =
        GetZoomedValue(ComputeStarDrawSize(CachedPrimaryStarMap));

    const float StarHitRadius = (StarSize * 0.5f) + 10.0f;

    if (FVector2D::Distance(LocalPos, SystemCenter) <= StarHitRadius)
    {
        ResetSystemView();
        return true;
    }

    float MaxOrbitInSystem = 0.0f;
    for (const FS_PlanetMap& PlanetRow : CachedPrimaryStarMap.Planet)
    {
        MaxOrbitInSystem = FMath::Max(MaxOrbitInSystem, static_cast<float>(PlanetRow.Orbit));
    }

    const float MaxDrawOrbitRadius =
        GetZoomedValue(ComputeMaxDrawOrbitRadius(PanelSize, StarSize));

    for (int32 PlanetIndex = 0; PlanetIndex < CachedPrimaryStarMap.Planet.Num(); ++PlanetIndex)
    {
        const FS_PlanetMap& PlanetRow = CachedPrimaryStarMap.Planet[PlanetIndex];

        const float OrbitRadius = ComputePlanetOrbitRadius(
            PlanetRow,
            MaxOrbitInSystem,
            MaxDrawOrbitRadius);

        const float OrbitTiltRadians = ComputeOrbitTiltRadians(PlanetRow);
        const float OrbitVerticalScale = ComputeOrbitVerticalScale(PlanetRow);
        const float PlanetAngleRadians = ComputePlanetAngleRadians(PlanetRow, PlanetIndex);
        const float PlanetDrawSize = GetZoomedValue(ComputePlanetDrawSize(PlanetRow));

        const FVector2D PlanetCenter = ComputeOrbitPosition(
            SystemCenter,
            OrbitRadius,
            PlanetAngleRadians,
            OrbitTiltRadians,
            OrbitVerticalScale);

        const float PlanetHitRadius = (PlanetDrawSize * 0.5f) + 6.0f;

        if (FVector2D::Distance(LocalPos, PlanetCenter) <= PlanetHitRadius)
        {
            const FVector2D RelativeOffset = PlanetCenter - SystemCenter;
            FocusOnPlanet(RelativeOffset, PanelSize);
            return true;
        }
    }

    return false;
}

void USystemMapPanel::ZoomIn()
{
    ZoomScale = FMath::Clamp(ZoomScale + 0.1f, MinZoomScale, MaxZoomScale);
    Invalidate(EInvalidateWidget::Paint);
}

void USystemMapPanel::ZoomOut()
{
    ZoomScale = FMath::Clamp(ZoomScale - 0.1f, MinZoomScale, MaxZoomScale);
    PanOffset = ClampPanOffset(PanOffset, GetCachedGeometry().GetLocalSize());
    Invalidate(EInvalidateWidget::Paint);
}

void USystemMapPanel::ResetSystemView()
{
    ZoomScale = 1.0f;
    PanOffset = FVector2D::ZeroVector;
    bDraggingMap = false;

    Invalidate(EInvalidateWidget::Paint);
}

void USystemMapPanel::FocusOnPlanet(const FVector2D& RelativeOffset, const FVector2D& PanelSize)
{
    PanOffset = ClampPanOffset(-RelativeOffset, PanelSize);
    Invalidate(EInvalidateWidget::Paint);
}
FReply USystemMapPanel::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bDraggingMap = true;
        DragStartScreenPosition = InMouseEvent.GetScreenSpacePosition();
        DragStartPanOffset = PanOffset;

        return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::SetDirectly);
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPos =
            InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        HandleClickSelection(LocalPos, InGeometry);
        return FReply::Handled().SetUserFocus(TakeWidget(), EFocusCause::SetDirectly);
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USystemMapPanel::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bDraggingMap = false;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply USystemMapPanel::NativeOnMouseMove(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (bDraggingMap)
    {
        const FVector2D MouseDelta =
            InMouseEvent.GetScreenSpacePosition() - DragStartScreenPosition;

        PanOffset = ClampPanOffset(
            DragStartPanOffset + MouseDelta,
            InGeometry.GetLocalSize());

        Invalidate(EInvalidateWidget::Paint);
        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply USystemMapPanel::NativeOnMouseWheel(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    const float Delta = InMouseEvent.GetWheelDelta();

    UE_LOG(LogTemp, Warning, TEXT("[SystemMapPanel] Mouse wheel delta: %f"), Delta);

    if (Delta > 0.0f)
    {
        ZoomIn();
        return FReply::Handled();
    }

    if (Delta < 0.0f)
    {
        ZoomOut();
        return FReply::Handled();
    }

    return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

int32 USystemMapPanel::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    LayerId = Super::NativePaint(
        Args,
        AllottedGeometry,
        MyCullingRect,
        OutDrawElements,
        LayerId,
        InWidgetStyle,
        bParentEnabled);

    if (!bValidSystem || CachedPrimaryStarMap.Name.IsEmpty())
    {
        return LayerId;
    }

    const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
    const FVector2D SystemCenter(
        (PanelSize.X * 0.5f) + PanOffset.X,
        (PanelSize.Y * 0.50f) + PanOffset.Y);

    const float StarSize = GetZoomedValue(ComputeStarDrawSize(CachedPrimaryStarMap));
    const float RingSize = GetZoomedValue(ComputeRingDrawSize(CachedPrimaryStarMap));

    UTexture2D* StarTexture = GetStarTextureForClass(CachedPrimaryStarMap.Class);

    const FLinearColor StarTint = ComputeStarTint(CachedPrimaryStarMap);
    const FLinearColor IFFRingTint = ComputeSystemIFFRingTint(CachedGalaxyRow);

    int32 PaintLayer = LayerId;

    float MaxOrbitInSystem = 0.0f;
    for (const FS_PlanetMap& PlanetRow : CachedPrimaryStarMap.Planet)
    {
        MaxOrbitInSystem = FMath::Max(MaxOrbitInSystem, static_cast<float>(PlanetRow.Orbit));
    }

    const float MaxDrawOrbitRadius =
        GetZoomedValue(ComputeMaxDrawOrbitRadius(PanelSize, StarSize));

    for (const FS_PlanetMap& PlanetRow : CachedPrimaryStarMap.Planet)
    {
        const float OrbitRadius = ComputePlanetOrbitRadius(
            PlanetRow,
            MaxOrbitInSystem,
            MaxDrawOrbitRadius);

        const float OrbitVerticalScale = ComputeOrbitVerticalScale(PlanetRow);
        const float OrbitTiltRadians = ComputeOrbitTiltRadians(PlanetRow);

        DrawOrbitEllipseLines(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry,
            SystemCenter,
            OrbitRadius,
            OrbitVerticalScale,
            OrbitTiltRadians,
            FLinearColor(0.35f, 0.45f, 0.65f, 0.40f),
            1.0f,
            96);
    }

    if (IFFRingTexture)
    {
        FSlateBrush IFFRingBrush;
        IFFRingBrush.DrawAs = ESlateBrushDrawType::Image;
        IFFRingBrush.SetResourceObject(IFFRingTexture);
        IFFRingBrush.ImageSize = FVector2D(RingSize, RingSize);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(SystemCenter.X - RingSize * 0.5f, SystemCenter.Y - RingSize * 0.5f),
                FVector2D(RingSize, RingSize)),
            &IFFRingBrush,
            ESlateDrawEffect::None,
            IFFRingTint);
    }

    if (StarTexture)
    {
        FSlateBrush StarBrush;
        StarBrush.DrawAs = ESlateBrushDrawType::Image;
        StarBrush.SetResourceObject(StarTexture);
        StarBrush.ImageSize = FVector2D(StarSize, StarSize);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(SystemCenter.X - StarSize * 0.5f, SystemCenter.Y - StarSize * 0.5f),
                FVector2D(StarSize, StarSize)),
            &StarBrush,
            ESlateDrawEffect::None,
            StarTint);
    }

    for (int32 PlanetIndex = 0; PlanetIndex < CachedPrimaryStarMap.Planet.Num(); ++PlanetIndex)
    {
        const FS_PlanetMap& PlanetRow = CachedPrimaryStarMap.Planet[PlanetIndex];

        const float OrbitRadius = ComputePlanetOrbitRadius(
            PlanetRow,
            MaxOrbitInSystem,
            MaxDrawOrbitRadius);

        const float OrbitTiltRadians = ComputeOrbitTiltRadians(PlanetRow);
        const float OrbitVerticalScale = ComputeOrbitVerticalScale(PlanetRow);
        const float PlanetAngleRadians = ComputePlanetAngleRadians(PlanetRow, PlanetIndex);
        const float PlanetDrawSize = GetZoomedValue(ComputePlanetDrawSize(PlanetRow));

        const FVector2D PlanetCenter = ComputeOrbitPosition(
            SystemCenter,
            OrbitRadius,
            PlanetAngleRadians,
            OrbitTiltRadians,
            OrbitVerticalScale);

        if (!PlanetRow.Ring.IsEmpty())
        {
            const float PlanetRingRadiusX = PlanetDrawSize * 0.58f;
            const float PlanetRingRadiusY = PlanetDrawSize * 0.58f;

            DrawPlanetRingEllipseLines(
                OutDrawElements,
                ++PaintLayer,
                AllottedGeometry,
                PlanetCenter,
                PlanetRingRadiusX,
                PlanetRingRadiusY,
                FLinearColor(0.56f, 0.56f, 0.56f, 0.90f),
                0.9f,
                1.0f,
                72);

            PaintLayer += 1;
        }

        UTexture2D* PlanetTexture = GetPlanetTexture(PlanetRow);
        if (PlanetTexture)
        {
            FSlateBrush PlanetBrush;
            PlanetBrush.DrawAs = ESlateBrushDrawType::Image;
            PlanetBrush.SetResourceObject(PlanetTexture);
            PlanetBrush.ImageSize = FVector2D(PlanetDrawSize, PlanetDrawSize);

            FSlateDrawElement::MakeBox(
                OutDrawElements,
                ++PaintLayer,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(PlanetCenter.X - PlanetDrawSize * 0.5f, PlanetCenter.Y - PlanetDrawSize * 0.5f),
                    FVector2D(PlanetDrawSize, PlanetDrawSize)),
                &PlanetBrush,
                ESlateDrawEffect::None,
                FLinearColor::White);
        }

        float MaxMoonOrbitForPlanet = 0.0f;
        for (const FS_MoonMap& MoonRow : PlanetRow.Moon)
        {
            MaxMoonOrbitForPlanet = FMath::Max(
                MaxMoonOrbitForPlanet,
                static_cast<float>(MoonRow.Orbit));
        }

        for (int32 MoonIndex = 0; MoonIndex < PlanetRow.Moon.Num(); ++MoonIndex)
        {
            const FS_MoonMap& MoonRow = PlanetRow.Moon[MoonIndex];

            const float MoonOrbitRadius = ComputeMoonOrbitRadius(
                MoonRow,
                MaxMoonOrbitForPlanet,
                PlanetDrawSize);

            const float MoonAngleRadians = ComputeMoonAngleRadians(
                MoonRow,
                MoonIndex);

            const float MoonDrawSize = GetZoomedValue(ComputeMoonDrawSize(MoonRow));

            TArray<FVector2D> MoonOrbitPoints;
            MoonOrbitPoints.Reserve(49);

            for (int32 SegmentIndex = 0; SegmentIndex <= 48; ++SegmentIndex)
            {
                const float Angle = (2.0f * PI * SegmentIndex) / 48.0f;
                const float OffsetX = FMath::Cos(Angle) * MoonOrbitRadius;
                const float OffsetY = FMath::Sin(Angle) * MoonOrbitRadius;

                MoonOrbitPoints.Add(FVector2D(
                    PlanetCenter.X + OffsetX,
                    PlanetCenter.Y + OffsetY));
            }

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                ++PaintLayer,
                AllottedGeometry.ToPaintGeometry(),
                MoonOrbitPoints,
                ESlateDrawEffect::None,
                FLinearColor(0.55f, 0.55f, 0.60f, 0.12f),
                true,
                0.5f);

            const FVector2D MoonCenter(
                PlanetCenter.X + FMath::Cos(MoonAngleRadians) * MoonOrbitRadius,
                PlanetCenter.Y + FMath::Sin(MoonAngleRadians) * MoonOrbitRadius);

            UTexture2D* MoonTexture = GetMoonTexture(MoonRow);
            if (MoonTexture)
            {
                FSlateBrush MoonBrush;
                MoonBrush.DrawAs = ESlateBrushDrawType::Image;
                MoonBrush.SetResourceObject(MoonTexture);
                MoonBrush.ImageSize = FVector2D(MoonDrawSize, MoonDrawSize);

                FSlateDrawElement::MakeBox(
                    OutDrawElements,
                    ++PaintLayer,
                    AllottedGeometry.ToPaintGeometry(
                        FVector2D(MoonCenter.X - MoonDrawSize * 0.5f, MoonCenter.Y - MoonDrawSize * 0.5f),
                        FVector2D(MoonDrawSize, MoonDrawSize)),
                    &MoonBrush,
                    ESlateDrawEffect::None,
                    FLinearColor::White);
            }
        }
    }

    FSlateDrawElement::MakeText(
        OutDrawElements,
        ++PaintLayer,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(SystemCenter.X - 100.0f, SystemCenter.Y + StarSize * 0.5f + 22.0f),
            FVector2D(200.0f, 20.0f)),
        CachedPrimaryStarMap.Name,
        FCoreStyle::GetDefaultFontStyle("Regular", 11),
        ESlateDrawEffect::None,
        FLinearColor::White);

    return PaintLayer;
}