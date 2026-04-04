/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         GalaxyMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy map panel.

    - Loads galaxy systems from the environment subsystem
    - Caches star textures once
    - Projects all systems into panel space
    - Draws jump links in NativePaint
    - Draws star markers in NativePaint using Slate
    - Draws IFF rings in NativePaint
    - Supports zoom and right-mouse panning
    - Supports left-click selection using hit-testing
    - Uses separate projection rect and clip rect
*/

#include "GalaxyMapPanel.h"

#include "MissionNavDlg.h"
#include "SystemMarker.h"
#include "StarshatterEnvironmentSubsystem.h"

#include "Components/CanvasPanel.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"

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
            BuildGalaxyMap(Env->GalaxyDataArray);
        }
    }
}

void UGalaxyMapPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
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

    // Projection/layout rect:
    const FSlateRect SafeRect = GetUsablePanelRect(PanelSize);

    // Visibility/culling rect:
    const FSlateRect ClipRect = GetClipPanelRect(PanelSize);

    // ---------------------------------------------------------------------
    // Draw jump links
    // ---------------------------------------------------------------------
    for (const TPair<FString, FS_Galaxy>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;
        const FS_Galaxy& SystemRow = Pair.Value;

        const FVector2D* RawStart = CachedSystemPositions.Find(SystemName);
        if (!RawStart)
        {
            continue;
        }

        for (const FString& LinkedName : SystemRow.Link)
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

            FLinearColor LinkColor = FLinearColor(0.70f, 0.78f, 0.92f, 0.65f);

            if (SystemName == SelectedSystemName || LinkedName == SelectedSystemName)
            {
                LinkColor = FLinearColor(1.0f, 0.95f, 0.55f, 0.95f);
            }

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                Points,
                ESlateDrawEffect::None,
                LinkColor,
                true,
                1.5f);
        }
    }

    // ---------------------------------------------------------------------
    // Draw star markers + IFF rings + selection ring
    // ---------------------------------------------------------------------
    for (const TPair<FString, FS_Galaxy>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;
        const FS_Galaxy& SystemRow = Pair.Value;

        const FVector2D* RawPos = CachedSystemPositions.Find(SystemName);
        if (!RawPos)
        {
            continue;
        }

        const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);

        UTexture2D* StarTex = GetCachedStarTextureForClass(SystemRow.Class);
        if (!StarTex)
        {
            continue;
        }

        float BaseMarkerSize = 28.0f;

        if (SystemName == SelectedSystemName)
        {
            BaseMarkerSize = 38.0f;
        }
        else if (SystemName == CurrentMissionSystemName)
        {
            BaseMarkerSize = 34.0f;
        }

        const FVector2D DrawSize(BaseMarkerSize * MarkerRenderScale, BaseMarkerSize * MarkerRenderScale);
        const FVector2D DrawPos = ScreenPos - (DrawSize * 0.5f);

        const float Pad = 48.0f;
        const bool bOutside =
            DrawPos.X + DrawSize.X < ClipRect.Left - Pad ||
            DrawPos.X > ClipRect.Right + Pad ||
            DrawPos.Y + DrawSize.Y < ClipRect.Top - Pad ||
            DrawPos.Y > ClipRect.Bottom + Pad;

        if (bOutside)
        {
            continue;
        }

        // -------------------------------------------------
        // IFF ring
        // -------------------------------------------------
        {
            const float RingPad = 6.0f;
            const FVector2D RingMin = DrawPos - FVector2D(RingPad, RingPad);
            const FVector2D RingMax = DrawPos + DrawSize + FVector2D(RingPad, RingPad);

            TArray<FVector2D> RingPoints;
            RingPoints.Add(FVector2D(RingMin.X, RingMin.Y));
            RingPoints.Add(FVector2D(RingMax.X, RingMin.Y));
            RingPoints.Add(FVector2D(RingMax.X, RingMax.Y));
            RingPoints.Add(FVector2D(RingMin.X, RingMax.Y));
            RingPoints.Add(FVector2D(RingMin.X, RingMin.Y));

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 2,
                AllottedGeometry.ToPaintGeometry(),
                RingPoints,
                ESlateDrawEffect::None,
                GetIFFRingColor(SystemRow),
                true,
                1.5f);
        }

        // -------------------------------------------------
        // Star marker
        // -------------------------------------------------
        FSlateBrush MarkerBrush;
        MarkerBrush.DrawAs = ESlateBrushDrawType::Image;
        MarkerBrush.SetResourceObject(StarTex);
        MarkerBrush.ImageSize = DrawSize;

        FLinearColor StarTint = FLinearColor::White;

        if (SystemName == CurrentMissionSystemName)
        {
            StarTint = FLinearColor(0.85f, 1.00f, 0.85f, 1.0f);
        }

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            LayerId + 3,
            AllottedGeometry.ToPaintGeometry(DrawPos, DrawSize),
            &MarkerBrush,
            ESlateDrawEffect::None,
            StarTint);

        // -------------------------------------------------
        // Selected system ring
        // -------------------------------------------------
        if (SystemName == SelectedSystemName)
        {
            const float SelPad = 12.0f;
            const FVector2D SelMin = DrawPos - FVector2D(SelPad, SelPad);
            const FVector2D SelMax = DrawPos + DrawSize + FVector2D(SelPad, SelPad);

            TArray<FVector2D> SelPoints;
            SelPoints.Add(FVector2D(SelMin.X, SelMin.Y));
            SelPoints.Add(FVector2D(SelMax.X, SelMin.Y));
            SelPoints.Add(FVector2D(SelMax.X, SelMax.Y));
            SelPoints.Add(FVector2D(SelMin.X, SelMax.Y));
            SelPoints.Add(FVector2D(SelMin.X, SelMin.Y));

            FSlateDrawElement::MakeLines(
                OutDrawElements,
                LayerId + 4,
                AllottedGeometry.ToPaintGeometry(),
                SelPoints,
                ESlateDrawEffect::None,
                FLinearColor(1.00f, 0.95f, 0.40f, 1.0f),
                true,
                1.75f);
        }
    }

    return LayerId + 4;
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

FLinearColor UGalaxyMapPanel::GetIFFRingColor(const FS_Galaxy& SystemRow) const
{
    if (SystemRow.Iff == 1)
    {
        return FLinearColor(0.20f, 1.00f, 0.20f, 0.95f);
    }

    if (SystemRow.Iff == 2)
    {
        return FLinearColor(1.00f, 0.25f, 0.25f, 0.95f);
    }

    if (SystemRow.Iff == 3)
    {
        return FLinearColor(1.00f, 0.85f, 0.25f, 0.95f);
    }

    return FLinearColor(0.55f, 0.75f, 1.00f, 0.90f);
}

void UGalaxyMapPanel::BuildGalaxyMap(const TArray<FS_Galaxy>& InSystems)
{
    GalaxySystems = InSystems;

    SystemLookup.Empty();
    CachedSystemPositions.Empty();

    for (const FS_Galaxy& SystemRow : GalaxySystems)
    {
        if (!SystemRow.Name.IsEmpty())
        {
            SystemLookup.Add(SystemRow.Name, SystemRow);
        }
    }

    RebuildNormalizationBounds();

    for (const FS_Galaxy& SystemRow : GalaxySystems)
    {
        const FVector2D Pos = ProjectToPanel(SystemRow.Location);
        CachedSystemPositions.Add(SystemRow.Name, Pos);
    }

    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::ClearGalaxyMap()
{
    GalaxySystems.Empty();
    SystemLookup.Empty();
    CachedSystemPositions.Empty();
    MarkerMap.Empty();
    SelectedSystemName.Empty();
    CurrentMissionSystemName.Empty();
    bHasBounds = false;

    if (MapCanvas)
    {
        MapCanvas->ClearChildren();
    }

    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::SetSelectedSystem(const FString& InSystemName)
{
    SelectedSystemName = InSystemName;
    RefreshSelectionVisuals();
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::RefreshSelectionVisuals()
{
    for (TPair<FString, USystemMarker*>& Pair : MarkerMap)
    {
        if (Pair.Value)
        {
            Pair.Value->SetSelected(Pair.Key == SelectedSystemName);
        }
    }
}

void UGalaxyMapPanel::RebuildNormalizationBounds()
{
    bHasBounds = false;

    MaxAbsX = 1.0f;
    MaxAbsY = 1.0f;

    int32 Count = 0;

    for (const FS_Galaxy& SystemRow : GalaxySystems)
    {
        const float RawX = (float)SystemRow.Location.X;
        const float RawY = (float)(-SystemRow.Location.Y * VerticalDisplayScale);

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

    for (const TPair<FString, FS_Galaxy>& Pair : SystemLookup)
    {
        const FString& SystemName = Pair.Key;

        const FVector2D* RawPos = CachedSystemPositions.Find(SystemName);
        if (!RawPos)
        {
            continue;
        }

        const FVector2D ScreenPos = ApplyViewTransformToPoint(*RawPos, PanelSize);

        float BaseMarkerSize = 28.0f;
        if (SystemName == SelectedSystemName)
        {
            BaseMarkerSize = 38.0f;
        }
        else if (SystemName == CurrentMissionSystemName)
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
    MapZoomLevel = FMath::Clamp(MapZoomLevel + 0.1f, MinZoom, MaxZoom);
    Invalidate(EInvalidateWidget::Paint);
}

void UGalaxyMapPanel::ZoomOut()
{
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

FReply UGalaxyMapPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bIsPanning = true;
        PanStartMouse = InMouseEvent.GetScreenSpacePosition();
        PanStartOffset = CurrentPan;
        return FReply::Handled();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const FVector2D LocalPoint = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        FString HitSystemName;
        if (HitTestSystemAtLocalPoint(LocalPoint, HitSystemName))
        {
            SetSelectedSystem(HitSystemName);

            UE_LOG(LogTemp, Warning, TEXT("[GalaxyMapPanel] Selected system: %s"), *HitSystemName);

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