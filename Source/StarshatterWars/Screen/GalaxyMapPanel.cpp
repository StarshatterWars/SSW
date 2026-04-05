/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         GalaxyMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy map panel.

    - Loads runtime star systems from the environment subsystem
    - Caches star textures once
    - Projects all systems into panel space
    - Draws jump links in NativePaint
    - Draws star markers in NativePaint using Slate
    - Draws IFF rings in NativePaint
    - Supports zoom and right-mouse panning
    - Supports left-click selection using hit-testing
    - Supports left-button double-click activation
    - Uses separate projection rect and clip rect
*/

#include "GalaxyMapPanel.h"

#include "MissionNavDlg.h"
#include "SystemMarker.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"

#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"

static void DrawSelectionGuides(
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FGeometry& Geometry,
    const FVector2D& Center,
    float Radius,
    const FSlateRect& ClipRect,
    const FLinearColor& Color,
    float Thickness = 1.5f)
{
    {
        TArray<FVector2D> Points;
        Points.Add(FVector2D(ClipRect.Left, Center.Y));
        Points.Add(FVector2D(Center.X - Radius, Center.Y));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            true,
            Thickness);
    }

    {
        TArray<FVector2D> Points;
        Points.Add(FVector2D(Center.X + Radius, Center.Y));
        Points.Add(FVector2D(ClipRect.Right, Center.Y));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            true,
            Thickness);
    }

    {
        TArray<FVector2D> Points;
        Points.Add(FVector2D(Center.X, ClipRect.Top));
        Points.Add(FVector2D(Center.X, Center.Y - Radius));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            true,
            Thickness);
    }

    {
        TArray<FVector2D> Points;
        Points.Add(FVector2D(Center.X, Center.Y + Radius));
        Points.Add(FVector2D(Center.X, ClipRect.Bottom));

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            Geometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            true,
            Thickness);
    }
}

static void DrawCircle(
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FGeometry& Geometry,
    const FVector2D& Center,
    float Radius,
    const FLinearColor& Color,
    float Thickness = 1.5f,
    int32 NumSegments = 24)
{
    TArray<FVector2D> Points;
    Points.Reserve(NumSegments + 1);

    for (int32 i = 0; i <= NumSegments; ++i)
    {
        const float Angle = (2.0f * PI * i) / NumSegments;

        FVector2D P(
            Center.X + FMath::Cos(Angle) * Radius,
            Center.Y + FMath::Sin(Angle) * Radius);

        Points.Add(P);
    }

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        Geometry.ToPaintGeometry(),
        Points,
        ESlateDrawEffect::None,
        Color,
        true,
        Thickness);
}

UGalaxyMapPanel::UGalaxyMapPanel(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UGalaxyMapPanel::NativeOnInitialized()
{
    Super::NativeOnInitialized();
}

void UGalaxyMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    if (MapRoot)
    {
        MapRoot->SetClipping(EWidgetClipping::ClipToBounds);
    }

    if (MapCameraRoot)
    {
        MapCameraRoot->SetClipping(EWidgetClipping::ClipToBounds);
        MapCameraRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    }

    if (MapCanvas)
    {
        MapCanvas->SetClipping(EWidgetClipping::ClipToBounds);
    }

    ResetView();
    CacheStarTextures();

    UE_LOG(LogTemp, Warning,
        TEXT("[GalaxyMapPanel] MapRoot=%p MapCameraRoot=%p MapCanvas=%p MarkerClass=%s"),
        MapRoot,
        MapCameraRoot,
        MapCanvas,
        *GetNameSafe(MarkerClass));

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterEnvironmentSubsystem* Env = GI->GetSubsystem<UStarshatterEnvironmentSubsystem>())
        {
            LoadFromRuntimeSystems(Env->GetRuntimeStarSystems());
        }
    }
}

void UGalaxyMapPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (bCameraAnimating)
    {
        CurrentPan = FMath::Vector2DInterpTo(
            CurrentPan,
            TargetPan,
            InDeltaTime,
            CameraInterpSpeed);

        MapZoomLevel = FMath::FInterpTo(
            MapZoomLevel,
            TargetZoom,
            InDeltaTime,
            CameraInterpSpeed);

        const bool bPanDone = CurrentPan.Equals(TargetPan, 0.5f);
        const bool bZoomDone = FMath::IsNearlyEqual(MapZoomLevel, TargetZoom, 0.01f);

        if (bPanDone && bZoomDone)
        {
            CurrentPan = TargetPan;
            MapZoomLevel = TargetZoom;
            bCameraAnimating = false;
        }

        Invalidate(EInvalidateWidget::Paint);
    }
}

int32 UGalaxyMapPanel::NativePaint(
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

    const FVector2D PanelSize = AllottedGeometry.GetLocalSize();

    if (CachedSystemPositions.Num() == 0 || SystemLookup.Num() == 0)
    {
        return LayerId;
    }

    const FSlateRect ClipRect = GetClipPanelRect(PanelSize);

    int32 PaintLayer = LayerId;

    if (!CurrentMissionSystemName.IsEmpty())
    {
        const FString HeaderText = CurrentMissionSystemName.ToUpper() + TEXT(" SYSTEM");
        const FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);

        FSlateDrawElement::MakeText(
            OutDrawElements,
            ++PaintLayer,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(ClipRect.Left + 8.0f, ClipRect.Top + 8.0f),
                FVector2D(260.0f, 24.0f)),
            HeaderText,
            HeaderFont,
            ESlateDrawEffect::None,
            FLinearColor(1.0f, 0.95f, 0.55f, 1.0f));
    }

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;
        StarSystem* System = Pair.Value;
        if (!System)
        {
            continue;
        }

        const FVector2D* RawStart = CachedSystemPositions.Find(SystemName);
        if (!RawStart)
        {
            continue;
        }

        const TArray<FString> LinkedNames = GetLinkedSystemNames(System);

        for (const FString& LinkedName : LinkedNames)
        {
            const FVector2D* RawEnd = CachedSystemPositions.Find(LinkedName);
            if (!RawEnd)
            {
                continue;
            }

            if (SystemName.Compare(LinkedName, ESearchCase::IgnoreCase) >= 0)
            {
                continue;
            }

            const FVector2D Start = ApplyViewTransformToPoint(*RawStart, PanelSize);
            const FVector2D End = ApplyViewTransformToPoint(*RawEnd, PanelSize);

            const bool bBothLeft = Start.X < ClipRect.Left && End.X < ClipRect.Left;
            const bool bBothRight = Start.X > ClipRect.Right && End.X > ClipRect.Right;
            const bool bBothTop = Start.Y < ClipRect.Top && End.Y < ClipRect.Top;
            const bool bBothBottom = Start.Y > ClipRect.Bottom && End.Y > ClipRect.Bottom;

            if (bBothLeft || bBothRight || bBothTop || bBothBottom)
            {
                continue;
            }

            TArray<FVector2D> Points;
            Points.Add(Start);
            Points.Add(End);

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                PaintLayer + 1,
                AllottedGeometry.ToPaintGeometry(),
                Points,
                ESlateDrawEffect::None,
                FLinearColor(0.35f, 0.45f, 0.65f, 0.45f),
                true,
                1.5f);
        }
    }

    PaintLayer += 1;

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;
        StarSystem* System = Pair.Value;
        if (!System)
        {
            continue;
        }

        const FVector2D* RawPos = CachedSystemPositions.Find(SystemName);
        if (!RawPos)
        {
            continue;
        }

        const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);

        UTexture2D* StarTex = GetCachedStarTextureForClass(GetSpectralClassForSystem(System));
        if (!StarTex)
        {
            continue;
        }

        float BaseSize = 28.0f;

        if (SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
        {
            BaseSize = 42.0f;
        }
        else if (SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase))
        {
            BaseSize = 36.0f;
        }

        const FVector2D DrawSize(BaseSize * MarkerRenderScale);
        const FVector2D DrawPos = ScreenPos - DrawSize * 0.5f;

        const float Pad = 64.0f;
        if (DrawPos.X > ClipRect.Right + Pad ||
            DrawPos.X + DrawSize.X < ClipRect.Left - Pad ||
            DrawPos.Y > ClipRect.Bottom + Pad ||
            DrawPos.Y + DrawSize.Y < ClipRect.Top - Pad)
        {
            continue;
        }

        {
            const float Radius = 0.5f * DrawSize.X + 5.0f;

            DrawCircle(
                OutDrawElements,
                PaintLayer + 1,
                AllottedGeometry,
                ScreenPos,
                Radius,
                GetIFFRingColor(System),
                1.5f,
                28);
        }

        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.SetResourceObject(StarTex);
        Brush.ImageSize = DrawSize;

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            PaintLayer + 2,
            AllottedGeometry.ToPaintGeometry(DrawPos, DrawSize),
            &Brush,
            ESlateDrawEffect::None,
            FLinearColor::White);

        if (SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
        {
            const float Radius = 0.5f * DrawSize.X + 12.0f;
            const FLinearColor MissionColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

            DrawCircle(
                OutDrawElements,
                PaintLayer + 3,
                AllottedGeometry,
                ScreenPos,
                Radius,
                MissionColor,
                2.5f,
                32);

            DrawSelectionGuides(
                OutDrawElements,
                PaintLayer + 4,
                AllottedGeometry,
                ScreenPos,
                Radius,
                ClipRect,
                MissionColor,
                1.5f);
        }

        if (SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase) &&
            !SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
        {
            const float Radius = 0.5f * DrawSize.X + 12.0f;

            DrawCircle(
                OutDrawElements,
                PaintLayer + 5,
                AllottedGeometry,
                ScreenPos,
                Radius,
                FLinearColor(1.0f, 0.95f, 0.40f, 1.0f),
                2.0f,
                32);
        }
    }

    PaintLayer += 5;

    if (!SelectedSystemName.IsEmpty())
    {
        for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
        {
            const FString& SystemName = Pair.Key;
            StarSystem* System = Pair.Value;
            if (!System)
            {
                continue;
            }

            const FVector2D* RawStart = CachedSystemPositions.Find(SystemName);
            if (!RawStart)
            {
                continue;
            }

            const TArray<FString> LinkedNames = GetLinkedSystemNames(System);

            for (const FString& LinkedName : LinkedNames)
            {
                const FVector2D* RawEnd = CachedSystemPositions.Find(LinkedName);
                if (!RawEnd)
                {
                    continue;
                }

                if (SystemName.Compare(LinkedName, ESearchCase::IgnoreCase) >= 0)
                {
                    continue;
                }

                const bool bTouchesSelection =
                    SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase) ||
                    LinkedName.Equals(SelectedSystemName, ESearchCase::IgnoreCase);

                if (!bTouchesSelection || IsRouteLink(SystemName, LinkedName))
                {
                    continue;
                }

                const FVector2D Start = ApplyViewTransformToPoint(*RawStart, PanelSize);
                const FVector2D End = ApplyViewTransformToPoint(*RawEnd, PanelSize);

                TArray<FVector2D> Points;
                Points.Add(Start);
                Points.Add(End);

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    PaintLayer + 1,
                    AllottedGeometry.ToPaintGeometry(),
                    Points,
                    ESlateDrawEffect::None,
                    FLinearColor(1.0f, 0.95f, 0.55f, 0.85f),
                    true,
                    2.0f);
            }
        }
    }

    if (RoutePathSystems.Num() >= 2)
    {
        for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
        {
            const FString& SystemName = Pair.Key;
            StarSystem* System = Pair.Value;
            if (!System)
            {
                continue;
            }

            const FVector2D* RawStart = CachedSystemPositions.Find(SystemName);
            if (!RawStart)
            {
                continue;
            }

            const TArray<FString> LinkedNames = GetLinkedSystemNames(System);

            for (const FString& LinkedName : LinkedNames)
            {
                const FVector2D* RawEnd = CachedSystemPositions.Find(LinkedName);
                if (!RawEnd)
                {
                    continue;
                }

                if (SystemName.Compare(LinkedName, ESearchCase::IgnoreCase) >= 0)
                {
                    continue;
                }

                if (!IsRouteLink(SystemName, LinkedName))
                {
                    continue;
                }

                const FVector2D Start = ApplyViewTransformToPoint(*RawStart, PanelSize);
                const FVector2D End = ApplyViewTransformToPoint(*RawEnd, PanelSize);

                TArray<FVector2D> Points;
                Points.Add(Start);
                Points.Add(End);

                FSlateDrawElement::MakeLines(
                    OutDrawElements,
                    PaintLayer + 2,
                    AllottedGeometry.ToPaintGeometry(),
                    Points,
                    ESlateDrawEffect::None,
                    FLinearColor(0.20f, 0.85f, 1.00f, 0.98f),
                    true,
                    3.5f);
            }
        }
    }

    PaintLayer += 2;

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;

        const FVector2D* RawPos = CachedSystemPositions.Find(SystemName);
        if (!RawPos)
        {
            continue;
        }

        const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);

        float BaseSize = 28.0f;
        if (SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
        {
            BaseSize = 42.0f;
        }
        else if (SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase))
        {
            BaseSize = 36.0f;
        }

        const FVector2D DrawSize(BaseSize * MarkerRenderScale);

        const bool bShowName =
            SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase) ||
            SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase) ||
            (MapZoomLevel >= 1.1f);

        if (bShowName)
        {
            const float LabelWidth = 140.0f;
            const float LabelHeight = 20.0f;

            const FVector2D TextPos(
                ScreenPos.X - (LabelWidth * 0.5f),
                ScreenPos.Y + (0.5f * DrawSize.Y) + 8.0f);

            const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 10);

            FLinearColor TextColor = FLinearColor(0.90f, 0.95f, 1.0f, 0.95f);

            if (SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
            {
                TextColor = FLinearColor(1.0f, 0.35f, 0.35f, 1.0f);
            }
            else if (SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase))
            {
                TextColor = FLinearColor(1.0f, 0.95f, 0.55f, 1.0f);
            }

            FSlateDrawElement::MakeText(
                OutDrawElements,
                PaintLayer + 1,
                AllottedGeometry.ToPaintGeometry(TextPos, FVector2D(LabelWidth, LabelHeight)),
                SystemName,
                FontInfo,
                ESlateDrawEffect::None,
                TextColor);
        }
    }

    return PaintLayer + 1;
}

void UGalaxyMapPanel::CacheStarTextures()
{
    StarTextureCache.Empty();

    StarTextureCache.Add(ESPECTRAL_CLASS::A,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarA_map.StarA_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::B,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarB_map.StarB_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::F,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarF_map.StarF_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::G,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::K,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarK_map.StarK_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::M,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarM_map.StarM_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::O,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarO_map.StarO_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::WHITE_DWARF,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/White.White")));
    StarTextureCache.Add(ESPECTRAL_CLASS::RED_GIANT,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/StarG_map.StarG_map")));
    StarTextureCache.Add(ESPECTRAL_CLASS::BLACK_HOLE,
        LoadGalaxyTexture(TEXT("/Game/UI/GalaxyMap/White.White")));
}

UTexture2D* UGalaxyMapPanel::LoadGalaxyTexture(const TCHAR* AssetPath) const
{
    if (!AssetPath || !*AssetPath)
    {
        return nullptr;
    }

    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, AssetPath);
    if (!Texture)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GalaxyMapPanel] Failed to load texture: %s"), AssetPath);
    }

    return Texture;
}

UTexture2D* UGalaxyMapPanel::GetCachedStarTextureForClass(ESPECTRAL_CLASS InClass) const
{
    if (const TObjectPtr<UTexture2D>* Found = StarTextureCache.Find(InClass))
    {
        return Found->Get();
    }

    if (const TObjectPtr<UTexture2D>* DefaultFound = StarTextureCache.Find(ESPECTRAL_CLASS::G))
    {
        return DefaultFound->Get();
    }

    return nullptr;
}

ESPECTRAL_CLASS UGalaxyMapPanel::GetSpectralClassForSystem(StarSystem* InSystem) const
{
    if (!InSystem)
    {
        return ESPECTRAL_CLASS::G;
    }

    switch (InSystem->GetSequence())
    {
    case Star::O:           return ESPECTRAL_CLASS::O;
    case Star::B:           return ESPECTRAL_CLASS::B;
    case Star::A:           return ESPECTRAL_CLASS::A;
    case Star::F:           return ESPECTRAL_CLASS::F;
    case Star::G:           return ESPECTRAL_CLASS::G;
    case Star::K:           return ESPECTRAL_CLASS::K;
    case Star::M:           return ESPECTRAL_CLASS::M;
    case Star::RED_GIANT:   return ESPECTRAL_CLASS::RED_GIANT;
    case Star::WHITE_DWARF: return ESPECTRAL_CLASS::WHITE_DWARF;
    case Star::BLACK_HOLE:  return ESPECTRAL_CLASS::BLACK_HOLE;
    default:                return ESPECTRAL_CLASS::G;
    }
}

FLinearColor UGalaxyMapPanel::GetIFFRingColor(StarSystem* InSystem) const
{
    if (!InSystem)
    {
        return FLinearColor(0.55f, 0.75f, 1.00f, 0.90f);
    }

    if (InSystem->GetAffiliation() == 1)
    {
        return FLinearColor(0.20f, 1.00f, 0.20f, 0.95f);
    }

    if (InSystem->GetAffiliation() == 2)
    {
        return FLinearColor(1.00f, 0.25f, 0.25f, 0.95f);
    }

    if (InSystem->GetAffiliation() == 3)
    {
        return FLinearColor(1.00f, 0.85f, 0.25f, 0.95f);
    }

    return FLinearColor(0.55f, 0.75f, 1.00f, 0.90f);
}

TArray<FString> UGalaxyMapPanel::GetLinkedSystemNames(StarSystem* InSystem) const
{
    TArray<FString> Result;

    if (!InSystem)
    {
        return Result;
    }

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        StarSystem* Other = Pair.Value;
        if (!Other || Other == InSystem)
        {
            continue;
        }

        if (InSystem->HasLinkTo(Other))
        {
            Result.Add(Pair.Key);
        }
    }

    Result.Sort();
    return Result;
}

void UGalaxyMapPanel::LoadFromRuntimeSystems(const TArray<StarSystem*>& InSystems)
{
    RuntimeSystemRefs.Empty();
    SystemLookup.Empty();
    CachedSystemPositions.Empty();

    for (StarSystem* System : InSystems)
    {
        if (!System)
        {
            continue;
        }

        RuntimeSystemRefs.Add(System);

        const FString SystemName = ANSI_TO_TCHAR(System->GetName());
        SystemLookup.Add(SystemName, System);
    }

    RebuildNormalizationBounds();

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        StarSystem* System = Pair.Value;
        if (!System)
        {
            continue;
        }

        const FVector2D Pos = ProjectToPanel(System->GetLocation());
        CachedSystemPositions.Add(Pair.Key, Pos);
    }

    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::ClearGalaxyMap()
{
    RuntimeSystemRefs.Empty();
    SystemLookup.Empty();
    CachedSystemPositions.Empty();
    MarkerMap.Empty();
    SelectedSystemName.Empty();
    CurrentMissionSystemName.Empty();
    RoutePathSystems.Empty();
    bHasBounds = false;

    if (MapCanvas)
    {
        MapCanvas->ClearChildren();
    }

    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::SetSelectedSystem(const FString& InSystemName)
{
    SelectedSystemName = InSystemName.TrimStartAndEnd();
    RefreshSelectionVisuals();
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::SetCurrentMissionSystem(const FString& InSystemName)
{
    CurrentMissionSystemName = InSystemName.TrimStartAndEnd();
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::SetRoutePath(const TArray<FString>& InRouteSystems)
{
    RoutePathSystems.Empty();
    RoutePathSystems.Reserve(InRouteSystems.Num());

    for (const FString& Name : InRouteSystems)
    {
        RoutePathSystems.Add(Name.TrimStartAndEnd());
    }

    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::RefreshSelectionVisuals()
{
    for (TPair<FString, USystemMarker*>& Pair : MarkerMap)
    {
        if (Pair.Value)
        {
            Pair.Value->SetSelected(Pair.Key.Equals(SelectedSystemName, ESearchCase::IgnoreCase));
        }
    }
}

void UGalaxyMapPanel::RebuildNormalizationBounds()
{
    bHasBounds = false;

    MaxAbsX = 1.0f;
    MaxAbsY = 1.0f;

    int32 Count = 0;

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        StarSystem* System = Pair.Value;
        if (!System)
        {
            continue;
        }

        const FVector SystemLoc = System->GetLocation();

        const float RawX = (float)SystemLoc.X;
        const float RawY = (float)(-SystemLoc.Y * VerticalDisplayScale);

        MaxAbsX = FMath::Max(MaxAbsX, FMath::Abs(RawX));
        MaxAbsY = FMath::Max(MaxAbsY, FMath::Abs(RawY));
        ++Count;
    }

    bHasBounds = (Count > 0);

    UE_LOG(LogTemp, Warning,
        TEXT("[GalaxyMapPanel] Bounds: MaxAbsX=%.2f MaxAbsY=%.2f Count=%d"),
        MaxAbsX, MaxAbsY, Count);
}

FSlateRect UGalaxyMapPanel::GetUsablePanelRect(const FVector2D& PanelSize) const
{
    return FSlateRect(
        LeftMargin,
        TopMargin,
        PanelSize.X - RightMargin,
        PanelSize.Y - BottomMargin);
}

FSlateRect UGalaxyMapPanel::GetClipPanelRect(const FVector2D& PanelSize) const
{
    return FSlateRect(
        LeftMargin,
        TopMargin - 50.0f,
        PanelSize.X - RightMargin,
        PanelSize.Y - BottomMargin + 50.0f);
}

FVector2D UGalaxyMapPanel::ProjectToPanel(const FVector& WorldLocation) const
{
    FVector2D PanelSize(1024.0f, 768.0f);

    if (MapCanvas)
    {
        const FVector2D Cached = MapCanvas->GetCachedGeometry().GetLocalSize();
        if (Cached.X > 1.0f && Cached.Y > 1.0f)
        {
            PanelSize = Cached;
        }
    }

    const FVector2D RawXY(
        (float)WorldLocation.X,
        (float)(-WorldLocation.Y * VerticalDisplayScale));

    return NormalizeToPanel(RawXY, PanelSize);
}

FVector2D UGalaxyMapPanel::NormalizeToPanel(const FVector2D& RawXY, const FVector2D& PanelSize) const
{
    const FVector2D PanelCenter = PanelSize * 0.5f;

    if (!bHasBounds)
    {
        return PanelCenter;
    }

    const FSlateRect SafeRect = GetUsablePanelRect(PanelSize);
    const float HalfWidth = 0.5f * (SafeRect.Right - SafeRect.Left);
    const float HalfHeight = 0.5f * (SafeRect.Bottom - SafeRect.Top);

    const float NX = RawXY.X / MaxAbsX;
    const float NY = RawXY.Y / MaxAbsY;

    return FVector2D(
        PanelCenter.X + NX * HalfWidth,
        PanelCenter.Y + NY * HalfHeight);
}

FVector2D UGalaxyMapPanel::ApplyViewTransformToPoint(const FVector2D& InPoint, const FVector2D& PanelSize) const
{
    const FVector2D PanelCenter = PanelSize * 0.5f;

    FVector2D P = InPoint;
    P = PanelCenter + (P - PanelCenter) * MapZoomLevel;
    P += CurrentPan + ScreenOffset;

    return P;
}

bool UGalaxyMapPanel::HitTestSystemAtLocalPoint(const FVector2D& LocalPoint, FString& OutSystemName) const
{
    OutSystemName.Empty();

    if (CachedSystemPositions.Num() == 0 || SystemLookup.Num() == 0)
    {
        return false;
    }

    FVector2D PanelSize(1024.0f, 768.0f);

    if (MapCanvas)
    {
        const FVector2D Cached = MapCanvas->GetCachedGeometry().GetLocalSize();
        if (Cached.X > 1.0f && Cached.Y > 1.0f)
        {
            PanelSize = Cached;
        }
    }

    float BestDistSq = TNumericLimits<float>::Max();
    FString BestName;

    for (const TPair<FString, StarSystem*>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;

        const FVector2D* RawPos = CachedSystemPositions.Find(SystemName);
        if (!RawPos)
        {
            continue;
        }

        const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);

        float BaseMarkerSize = 28.0f;
        if (SystemName.Equals(SelectedSystemName, ESearchCase::IgnoreCase))
        {
            BaseMarkerSize = 38.0f;
        }
        else if (SystemName.Equals(CurrentMissionSystemName, ESearchCase::IgnoreCase))
        {
            BaseMarkerSize = 34.0f;
        }

        const float Radius = (0.5f * BaseMarkerSize * MarkerRenderScale) + 8.0f;
        const float DistSq = FVector2D::DistSquared(LocalPoint, ScreenPos);

        if (DistSq <= Radius * Radius && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestName = SystemName;
        }
    }

    if (!BestName.IsEmpty())
    {
        OutSystemName = BestName;
        return true;
    }

    return false;
}

void UGalaxyMapPanel::ZoomIn()
{
    StopCameraAnimation();
    MapZoomLevel = FMath::Clamp(MapZoomLevel + 0.1f, MinZoom, MaxZoom);
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::ZoomOut()
{
    StopCameraAnimation();
    MapZoomLevel = FMath::Clamp(MapZoomLevel - 0.1f, MinZoom, MaxZoom);
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::ResetView()
{
    MapZoomLevel = 1.0f;
    CurrentPan = FVector2D::ZeroVector;
    ScreenOffset = InitialScreenOffset;
    bIsPanning = false;

    Invalidate(EInvalidateWidget::Paint);
}

FReply UGalaxyMapPanel::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    // ---------------------------------------------------
    // RIGHT MOUSE -> PAN (STOP CAMERA ANIMATION FIRST)
    // ---------------------------------------------------
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        // CRITICAL: stop any focus animation so user control wins
        StopCameraAnimation();

        bIsPanning = true;
        PanStartMouse = InMouseEvent.GetScreenSpacePosition();
        PanStartOffset = CurrentPan;

        return FReply::Handled();
    }

    // ---------------------------------------------------
    // LEFT MOUSE -> SELECTION
    // ---------------------------------------------------
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPoint =
            InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        FString HitSystemName;
        if (HitTestSystemAtLocalPoint(LocalPoint, HitSystemName))
        {
            // Selection now optionally drives focus (handled inside)
            SetSelectedSystem(HitSystemName, true);

            if (OwnerNavDlg)
            {
                OwnerNavDlg->HandleGalaxySystemSelected(HitSystemName);
            }

            UE_LOG(LogTemp, Warning,
                TEXT("[GalaxyMapPanel] Selected system: %s"),
                *HitSystemName);

            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGalaxyMapPanel::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = false;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UGalaxyMapPanel::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsPanning)
    {
        const FVector2D MouseDelta = InMouseEvent.GetScreenSpacePosition() - PanStartMouse;
        CurrentPan = PanStartOffset + MouseDelta;
        Invalidate(EInvalidateWidget::Paint);
        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UGalaxyMapPanel::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const float Delta = InMouseEvent.GetWheelDelta();

    if (Delta > 0.0f)
    {
        ZoomIn();
    }
    else if (Delta < 0.0f)
    {
        ZoomOut();
    }

    return FReply::Handled();
}

FReply UGalaxyMapPanel::NativeOnMouseButtonDoubleClick(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
    {
        return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
    }

    const FVector2D LocalPoint =
        InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

    FString HitSystemName;
    if (!HitTestSystemAtLocalPoint(LocalPoint, HitSystemName))
    {
        return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
    }

    SetSelectedSystem(HitSystemName);

    if (OwnerNavDlg)
    {
        OwnerNavDlg->HandleGalaxySystemSelected(HitSystemName);
        OwnerNavDlg->HandleGalaxySystemActivated(HitSystemName);
    }

    UE_LOG(LogTemp, Warning,
        TEXT("[GalaxyMapPanel] Activated system via double-click: %s"),
        *HitSystemName);

    return FReply::Handled();
}

bool UGalaxyMapPanel::IsRouteLink(const FString& A, const FString& B) const
{
    if (RoutePathSystems.Num() < 2)
    {
        return false;
    }

    const FString AKey = A.TrimStartAndEnd();
    const FString BKey = B.TrimStartAndEnd();

    for (int32 i = 0; i < RoutePathSystems.Num() - 1; ++i)
    {
        const FString R0 = RoutePathSystems[i].TrimStartAndEnd();
        const FString R1 = RoutePathSystems[i + 1].TrimStartAndEnd();

        const bool bForward =
            R0.Equals(AKey, ESearchCase::IgnoreCase) &&
            R1.Equals(BKey, ESearchCase::IgnoreCase);

        const bool bReverse =
            R0.Equals(BKey, ESearchCase::IgnoreCase) &&
            R1.Equals(AKey, ESearchCase::IgnoreCase);

        if (bForward || bReverse)
        {
            return true;
        }
    }

    return false;
}


void UGalaxyMapPanel::SetSelectedSystem(const FString& InSystemName, bool bAutoFocus)
{
    SelectedSystemName = InSystemName.TrimStartAndEnd();
    RefreshSelectionVisuals();

    if (bAutoFocus && !SelectedSystemName.IsEmpty())
    {
        FVector2D PanelSize(1024.0f, 768.0f);

        if (MapCanvas)
        {
            const FVector2D Cached = MapCanvas->GetCachedGeometry().GetLocalSize();
            if (Cached.X > 1.0f && Cached.Y > 1.0f)
            {
                PanelSize = Cached;
            }
        }

        if (!IsSystemComfortablyVisible(SelectedSystemName, PanelSize))
        {
            FocusOnSystem(SelectedSystemName, true, false);
        }
    }

    Invalidate(EInvalidateWidget::Paint);
}

bool UGalaxyMapPanel::GetSystemRawPosition(const FString& InSystemName, FVector2D& OutRawPos) const
{
    if (const FVector2D* Found = CachedSystemPositions.Find(InSystemName))
    {
        OutRawPos = *Found;
        return true;
    }

    return false;
}

float UGalaxyMapPanel::GetFocusZoomForSystem(const FString& InSystemName) const
{
    return FMath::Clamp(
        FMath::Max(MapZoomLevel, FocusMinReadableZoom),
        MinZoom,
        MaxZoom);
}

FVector2D UGalaxyMapPanel::ComputeFocusPanForRawPoint(
    const FVector2D& RawPoint,
    const FVector2D& PanelSize,
    float InZoom) const
{
    const FVector2D PanelCenter = PanelSize * 0.5f;

    FVector2D FocusedPoint = PanelCenter + (RawPoint - PanelCenter) * InZoom;
    FocusedPoint += ScreenOffset;

    return PanelCenter - FocusedPoint;
}

bool UGalaxyMapPanel::IsSystemComfortablyVisible(
    const FString& InSystemName,
    const FVector2D& PanelSize) const
{
    const FVector2D* RawPos = CachedSystemPositions.Find(InSystemName);
    if (!RawPos)
    {
        return false;
    }

    const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);
    const FSlateRect ClipRect = GetClipPanelRect(PanelSize);

    return
        ScreenPos.X >= (ClipRect.Left + FocusVisibleMargin) &&
        ScreenPos.X <= (ClipRect.Right - FocusVisibleMargin) &&
        ScreenPos.Y >= (ClipRect.Top + FocusVisibleMargin) &&
        ScreenPos.Y <= (ClipRect.Bottom - FocusVisibleMargin);
}

void UGalaxyMapPanel::FocusOnSystem(
    const FString& InSystemName,
    bool bAnimate,
    bool bAllowZoomAdjust)
{
    FVector2D RawPos;
    if (!GetSystemRawPosition(InSystemName, RawPos))
    {
        return;
    }

    FVector2D PanelSize(1024.0f, 768.0f);

    if (MapCanvas)
    {
        const FVector2D Cached = MapCanvas->GetCachedGeometry().GetLocalSize();
        if (Cached.X > 1.0f && Cached.Y > 1.0f)
        {
            PanelSize = Cached;
        }
    }

    const float DesiredZoom =
        bAllowZoomAdjust ? GetFocusZoomForSystem(InSystemName) : MapZoomLevel;

    const FVector2D DesiredPan =
        ComputeFocusPanForRawPoint(RawPos, PanelSize, DesiredZoom);

    if (bAnimate)
    {
        TargetZoom = DesiredZoom;
        TargetPan = DesiredPan;
        bCameraAnimating = true;
    }
    else
    {
        MapZoomLevel = DesiredZoom;
        CurrentPan = DesiredPan;
        bCameraAnimating = false;
        Invalidate(EInvalidateWidget::Paint);
    }
}

void UGalaxyMapPanel::SaveViewState()
{
    SavedZoom = MapZoomLevel;
    SavedPan = CurrentPan;
    SavedScreenOffset = ScreenOffset;
    bHasSavedViewState = true;
}

void UGalaxyMapPanel::RestoreViewState(bool bAnimate)
{
    if (!bHasSavedViewState)
    {
        return;
    }

    if (bAnimate)
    {
        TargetZoom = SavedZoom;
        TargetPan = SavedPan;
        bCameraAnimating = true;
    }
    else
    {
        MapZoomLevel = SavedZoom;
        CurrentPan = SavedPan;
        ScreenOffset = SavedScreenOffset;
        bCameraAnimating = false;
        Invalidate(EInvalidateWidget::Paint);
    }
}

void UGalaxyMapPanel::StopCameraAnimation()
{
    bCameraAnimating = false;
    TargetZoom = MapZoomLevel;
    TargetPan = CurrentPan;
}