/*
    Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026.

    SUBSYSTEM:    UI
    FILE:         SectorMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Sector (Region) Map Panel - Read Only Pass 2

    - Runtime StarSystem + OrbitalRegion driven
    - Legacy DrawRegion() style grid and rep scaling
    - Read-only mission element rendering
    - No nav editing
*/

#include "SectorMapPanel.h"

#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Mission.h"
#include "MissionElement.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
    static StarSystem* ResolveRuntimeSystem(
        UGameInstance* GameInstance,
        const FString& SystemName)
    {
        if (!GameInstance || SystemName.IsEmpty())
        {
            return nullptr;
        }

        UStarshatterEnvironmentSubsystem* EnvironmentSubsystem =
            GameInstance->GetSubsystem<UStarshatterEnvironmentSubsystem>();

        if (!EnvironmentSubsystem)
        {
            return nullptr;
        }

        for (StarSystem* System : EnvironmentSubsystem->GetRuntimeStarSystems())
        {
            if (!System)
            {
                continue;
            }

            if (SystemName.Equals(ANSI_TO_TCHAR(System->GetName()), ESearchCase::IgnoreCase))
            {
                return System;
            }
        }

        return nullptr;
    }

    static FLinearColor ToLinearColor(const FColor& InColor)
    {
        return FLinearColor(
            InColor.R / 255.0f,
            InColor.G / 255.0f,
            InColor.B / 255.0f,
            InColor.A / 255.0f);
    }
}

void USectorMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();

    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    PanOffset = FVector2D::ZeroVector;
    ZoomScale = 4.0f;
    bDraggingMap = false;

    RefreshView();
}

int32 USectorMapPanel::NativePaint(
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

    if (!bValidView || !CachedRuntimeSystem || !CachedRegion)
    {
        return LayerId;
    }

    const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
    const FVector2D Center = (PanelSize * 0.5f) + PanOffset;

    const int32 RegionRadius = static_cast<int32>(CachedRegion->Radius());
    const int32 GridStep = static_cast<int32>(CachedRegion->GetGridSpace());

    if (RegionRadius <= 0 || GridStep <= 0)
    {
        return LayerId;
    }

    const double C = FMath::Min(PanelSize.X * 0.5, PanelSize.Y * 0.5);
    const double R = CachedRegion->Radius() / ZoomScale;
    const float Scale = (R > 0.0) ? static_cast<float>(C / R) : 1.0f;

    DrawRegionGrid(
        OutDrawElements,
        AllottedGeometry,
        LayerId + 1,
        Center,
        Scale,
        RegionRadius,
        GridStep);

    const int32 Rep = ComputeRepLevel(R);

    DrawMissionElements(
        OutDrawElements,
        AllottedGeometry,
        LayerId + 20,
        Center,
        Scale,
        Rep);

    const FString InfoString = FString::Printf(
        TEXT("SECTOR: %s    REP: %d    SCALE: %.4f    RANGE: %.0f"),
        ViewedSectorName.IsEmpty() ? TEXT("NONE") : *ViewedSectorName,
        Rep,
        Scale,
        R * 2.0);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 200,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(12.0f, 10.0f),
            FVector2D(520.0f, 20.0f)),
        InfoString,
        FCoreStyle::GetDefaultFontStyle("Regular", 11),
        ESlateDrawEffect::None,
        FLinearColor(0.85f, 0.90f, 1.0f, 0.95f));

    return LayerId + 200;
}

FReply USectorMapPanel::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bDraggingMap = true;
        DragStartScreenPosition = InMouseEvent.GetScreenSpacePosition();
        DragStartPanOffset = PanOffset;
        return FReply::Handled();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USectorMapPanel::NativeOnMouseButtonUp(
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

FReply USectorMapPanel::NativeOnMouseMove(
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

FReply USectorMapPanel::NativeOnMouseWheel(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    const float WheelDelta = InMouseEvent.GetWheelDelta();

    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] MouseWheel Delta=%.3f"), WheelDelta);

    if (WheelDelta > 0.0f)
    {
        ZoomIn();
        return FReply::Handled();
    }

    if (WheelDelta < 0.0f)
    {
        ZoomOut();
        return FReply::Handled();
    }

    return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void USectorMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    ViewedSystemName = InSystemName.TrimStartAndEnd();
    RefreshView();
}

void USectorMapPanel::SetViewedSectorName(const FString& InSectorName)
{
    ViewedSectorName = InSectorName.TrimStartAndEnd();
    RefreshView();
}

void USectorMapPanel::SetMission(Mission* InMission)
{
    CachedMission = InMission;
    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::SetSelectedElement(MissionElement* InElement)
{
    SelectedElement = InElement;
    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::BuildRuntimeLayout()
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
                TEXT("SectorMapRootCanvas"));

        WidgetTree->RootWidget = RootCanvas;
    }
}

void USectorMapPanel::RefreshView()
{
    CachedRuntimeSystem = nullptr;
    CachedRegion = nullptr;
    bValidView = false;

    if (!ResolveViewedSystem(CachedRuntimeSystem) || !CachedRuntimeSystem)
    {
        Invalidate(EInvalidateWidget::Paint);
        return;
    }

    if (!ResolveViewedRegion(CachedRuntimeSystem, CachedRegion) || !CachedRegion)
    {
        CachedRegion = CachedRuntimeSystem->ActiveRegion();

        if (!CachedRegion)
        {
            ListIter<OrbitalRegion> RegionIter = CachedRuntimeSystem->AllRegions();
            while (++RegionIter)
            {
                OrbitalRegion* Region = RegionIter.value();
                if (Region)
                {
                    CachedRegion = Region;
                    break;
                }
            }
        }

        if (CachedRegion && ViewedSectorName.IsEmpty())
        {
            ViewedSectorName = ANSI_TO_TCHAR(CachedRegion->GetName());
        }
    }

    bValidView = (CachedRegion != nullptr);

    Invalidate(EInvalidateWidget::Paint);
}

bool USectorMapPanel::ResolveViewedSystem(StarSystem*& OutSystem) const
{
    OutSystem = ResolveRuntimeSystem(GetGameInstance(), ViewedSystemName);
    return (OutSystem != nullptr);
}

bool USectorMapPanel::ResolveViewedRegion(StarSystem* InSystem, OrbitalRegion*& OutRegion) const
{
    OutRegion = nullptr;

    if (!InSystem || ViewedSectorName.IsEmpty())
    {
        return false;
    }

    ListIter<OrbitalRegion> RegionIter = InSystem->AllRegions();
    while (++RegionIter)
    {
        OrbitalRegion* Region = RegionIter.value();
        if (!Region)
        {
            continue;
        }

        if (ViewedSectorName.Equals(ANSI_TO_TCHAR(Region->GetName()), ESearchCase::IgnoreCase))
        {
            OutRegion = Region;
            return true;
        }
    }

    return false;
}

void USectorMapPanel::DrawRegionGrid(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 LayerId,
    const FVector2D& Center,
    float Scale,
    int32 RegionRadius,
    int32 GridStep) const
{
    const FLinearColor MajorColor(0.19f, 0.19f, 0.19f, 0.85f);
    const FLinearColor MinorColor(0.09f, 0.09f, 0.09f, 0.80f);

    const int32 Left = FMath::RoundToInt((-RegionRadius * Scale) + Center.X);
    const int32 Right = FMath::RoundToInt((RegionRadius * Scale) + Center.X);
    const int32 Top = FMath::RoundToInt((-RegionRadius * Scale) + Center.Y);
    const int32 Bottom = FMath::RoundToInt((RegionRadius * Scale) + Center.Y);

    int32 Tick = 0;

    for (int32 X = 0; X <= RegionRadius; X += GridStep)
    {
        const int32 LX1 = FMath::RoundToInt((X * Scale) + Center.X);
        const int32 LX2 = FMath::RoundToInt((-X * Scale) + Center.X);
        const FLinearColor LineColor = (Tick == 0) ? MajorColor : MinorColor;

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { FVector2D(LX1, Top), FVector2D(LX1, Bottom) },
            ESlateDrawEffect::None,
            LineColor,
            true,
            1.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { FVector2D(LX2, Top), FVector2D(LX2, Bottom) },
            ESlateDrawEffect::None,
            LineColor,
            true,
            1.0f);

        ++Tick;
        if (Tick > 3)
        {
            Tick = 0;
        }
    }

    Tick = 0;

    for (int32 Y = 0; Y <= RegionRadius; Y += GridStep)
    {
        const int32 LY1 = FMath::RoundToInt((Y * Scale) + Center.Y);
        const int32 LY2 = FMath::RoundToInt((-Y * Scale) + Center.Y);
        const FLinearColor LineColor = (Tick == 0) ? MajorColor : MinorColor;

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { FVector2D(Left, LY1), FVector2D(Right, LY1) },
            ESlateDrawEffect::None,
            LineColor,
            true,
            1.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            LayerId,
            AllottedGeometry.ToPaintGeometry(),
            { FVector2D(Left, LY2), FVector2D(Right, LY2) },
            ESlateDrawEffect::None,
            LineColor,
            true,
            1.0f);

        ++Tick;
        if (Tick > 3)
        {
            Tick = 0;
        }
    }
}

void USectorMapPanel::DrawMissionElements(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 BaseLayerId,
    const FVector2D& Center,
    float Scale,
    int32 Rep) const
{
    if (!CachedMission || !CachedRegion)
    {
        return;
    }

    ListIter<MissionElement> ElementIter = CachedMission->GetElements();
    while (++ElementIter)
    {
        MissionElement* Element = ElementIter.value();
        if (!Element)
        {
            continue;
        }

        if (Element->IsSquadron())
        {
            continue;
        }

        if (_stricmp(Element->GetRegion(), CachedRegion->GetName()) != 0)
        {
            continue;
        }

        DrawMissionElement(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId,
            Center,
            Scale,
            Rep,
            Element);
    }
}

void USectorMapPanel::DrawMissionElement(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 BaseLayerId,
    const FVector2D& Center,
    float Scale,
    int32 Rep,
    MissionElement* Element) const
{
    if (!Element)
    {
        return;
    }

    const FVector ElementLocation = Element->GetLocation();

    const FVector2D ScreenPos(
        Center.X + (ElementLocation.X * Scale),
        Center.Y + (ElementLocation.Y * Scale));

    const bool bVisible =
        ScreenPos.X >= 0.0f && ScreenPos.X < AllottedGeometry.GetLocalSize().X &&
        ScreenPos.Y >= 0.0f && ScreenPos.Y < AllottedGeometry.GetLocalSize().Y;

    if (!bVisible)
    {
        return;
    }

    const FLinearColor MarkerColor = ToLinearColor(Element->MarkerColor());

    float HalfSize = 3.0f;
    if (Rep < 3)
    {
        HalfSize = 2.0f;
    }
    else if (Element->IsStatic())
    {
        HalfSize = 6.0f;
    }
    else if (Element->IsStarship())
    {
        HalfSize = 4.0f;
    }
    else
    {
        HalfSize = 3.0f;
    }

    const FVector2D TopLeft(ScreenPos.X - HalfSize, ScreenPos.Y - HalfSize);
    const FVector2D DrawSize(HalfSize * 2.0f, HalfSize * 2.0f);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        BaseLayerId + 1,
        AllottedGeometry.ToPaintGeometry(TopLeft, DrawSize),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        MarkerColor);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        BaseLayerId + 2,
        AllottedGeometry.ToPaintGeometry(TopLeft, DrawSize),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor::Transparent);

    if (Element == SelectedElement)
    {
        const FVector2D SelTopLeft(ScreenPos.X - HalfSize - 2.0f, ScreenPos.Y - HalfSize - 2.0f);
        const FVector2D SelSize((HalfSize + 2.0f) * 2.0f, (HalfSize + 2.0f) * 2.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 3,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(SelTopLeft.X, SelTopLeft.Y),
                FVector2D(SelTopLeft.X + SelSize.X, SelTopLeft.Y),
                FVector2D(SelTopLeft.X + SelSize.X, SelTopLeft.Y + SelSize.Y),
                FVector2D(SelTopLeft.X, SelTopLeft.Y + SelSize.Y),
                FVector2D(SelTopLeft.X, SelTopLeft.Y)
            },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            1.0f);
    }

    if (Rep >= 2)
    {
        FString LabelText;
        if (Element->Count() > 1)
        {
            LabelText = FString::Printf(TEXT("%s x %d"), ANSI_TO_TCHAR(Element->GetName()), Element->Count());
        }
        else
        {
            LabelText = ANSI_TO_TCHAR(Element->GetName());
        }

        FSlateDrawElement::MakeText(
            OutDrawElements,
            BaseLayerId + 4,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(ScreenPos.X + HalfSize + 3.0f, ScreenPos.Y - 6.0f),
                FVector2D(220.0f, 16.0f)),
            LabelText,
            FCoreStyle::GetDefaultFontStyle("Regular", 10),
            ESlateDrawEffect::None,
            FLinearColor::White);
    }
}

int32 USectorMapPanel::ComputeRepLevel(double ZoomedRadius) const
{
    int32 Rep = 3;

    if (ZoomedRadius > 70000.0)
    {
        Rep = 2;
    }

    if (ZoomedRadius > 250000.0)
    {
        Rep = 1;
    }

    return Rep;
}

FVector2D USectorMapPanel::ClampPanOffset(const FVector2D& InOffset, const FVector2D& PanelSize) const
{
    const float DragAllowanceX = FMath::Max(120.0f, PanelSize.X * (ZoomScale - 1.0f) * 0.75f);
    const float DragAllowanceY = FMath::Max(120.0f, PanelSize.Y * (ZoomScale - 1.0f) * 0.75f);

    return FVector2D(
        FMath::Clamp(InOffset.X, -DragAllowanceX, DragAllowanceX),
        FMath::Clamp(InOffset.Y, -DragAllowanceY, DragAllowanceY));
}

void USectorMapPanel::ZoomIn()
{
    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] ZoomIn before: %.3f"), ZoomScale);

    ZoomScale = FMath::Clamp(ZoomScale * 1.25f, MinZoomScale, MaxZoomScale);
    PanOffset = ClampPanOffset(PanOffset, GetCachedGeometry().GetLocalSize());

    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] ZoomIn after: %.3f"), ZoomScale);

    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::ZoomOut()
{
    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] ZoomOut before: %.3f"), ZoomScale);

    ZoomScale = FMath::Clamp(ZoomScale * 0.8f, MinZoomScale, MaxZoomScale);
    PanOffset = ClampPanOffset(PanOffset, GetCachedGeometry().GetLocalSize());

    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] ZoomOut after: %.3f"), ZoomScale);

    Invalidate(EInvalidateWidget::Paint);
}
