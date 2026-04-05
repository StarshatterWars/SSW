#include "SectorMapPanel.h"

#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

static StarSystem* ResolveRuntimeSystem(
    UGameInstance* GI,
    const FString& Name)
{
    if (!GI || Name.IsEmpty())
    {
        return nullptr;
    }

    UStarshatterEnvironmentSubsystem* Env =
        GI->GetSubsystem<UStarshatterEnvironmentSubsystem>();

    if (!Env)
    {
        return nullptr;
    }

    for (StarSystem* Sys : Env->GetRuntimeStarSystems())
    {
        if (Sys && Name.Equals(ANSI_TO_TCHAR(Sys->GetName()), ESearchCase::IgnoreCase))
        {
            return Sys;
        }
    }

    return nullptr;
}

void USectorMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();

    PanOffset = FVector2D::ZeroVector;
    ZoomScale = 1.0f;
    bDraggingMap = false;

    RefreshView();
}

int32 USectorMapPanel::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& Geo,
    const FSlateRect& Clip,
    FSlateWindowElementList& Out,
    int32 Layer,
    const FWidgetStyle& Style,
    bool bParentEnabled) const
{
    Layer = Super::NativePaint(Args, Geo, Clip, Out, Layer, Style, bParentEnabled);

    if (!bValidView || !CachedRegion)
    {
        return Layer;
    }

    const FVector2D Size = Geo.GetLocalSize();
    const FVector2D Center = (Size * 0.5f) + PanOffset;

    const int32 Radius = (int32)CachedRegion->Radius();
    const int32 Step = (int32)CachedRegion->GetGridSpace();

    if (Radius <= 0 || Step <= 0)
    {
        return Layer;
    }

    const double C = FMath::Min(Size.X * 0.5, Size.Y * 0.5);
    const double R = CachedRegion->Radius() * ZoomScale;
    const float Scale = (R > 0.0) ? (float)(C / R) : 1.0f;

    DrawRegionGrid(Out, Geo, Layer + 1, Center, Scale, Radius, Step);

    const int32 Rep = ComputeRepLevel(R);

    const FString Info = FString::Printf(
        TEXT("SECTOR: %s  REP: %d  SCALE: %.4f"),
        *ViewedSectorName,
        Rep,
        Scale);

    FSlateDrawElement::MakeText(
        Out,
        Layer + 20,
        Geo.ToPaintGeometry(FVector2D(10, 30), FVector2D(400, 20)),
        Info,
        FCoreStyle::GetDefaultFontStyle("Regular", 10),
        ESlateDrawEffect::None,
        FLinearColor::White);

    return Layer + 20;
}

void USectorMapPanel::BuildRuntimeLayout()
{
    if (!WidgetTree)
    {
        return;
    }

    // Root canvas only (paint-driven panel)
    if (!RootCanvas)
    {
        RootCanvas =
            WidgetTree->ConstructWidget<UCanvasPanel>(
                UCanvasPanel::StaticClass(),
                TEXT("SectorMapRootCanvas"));

        WidgetTree->RootWidget = RootCanvas;
    }

    // Remove all text UI — everything rendered in NativePaint
    HeaderText = nullptr;
    InfoText = nullptr;
}

void USectorMapPanel::DrawRegionGrid(
    FSlateWindowElementList& Out,
    const FGeometry& Geo,
    int32 Layer,
    const FVector2D& Center,
    float Scale,
    int32 Radius,
    int32 Step) const
{
    const int32 Left = (int32)(-Radius * Scale + Center.X);
    const int32 Right = (int32)(Radius * Scale + Center.X);
    const int32 Top = (int32)(-Radius * Scale + Center.Y);
    const int32 Bottom = (int32)(Radius * Scale + Center.Y);

    int Tick = 0;

    for (int X = 0; X <= Radius; X += Step)
    {
        const float SX = X * Scale;

        const FLinearColor Col = (Tick == 0)
            ? FLinearColor(0.2f, 0.2f, 0.2f)
            : FLinearColor(0.08f, 0.08f, 0.08f);

        FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(),
            { FVector2D(Center.X + SX, Top), FVector2D(Center.X + SX, Bottom) },
            ESlateDrawEffect::None, Col, true);

        FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(),
            { FVector2D(Center.X - SX, Top), FVector2D(Center.X - SX, Bottom) },
            ESlateDrawEffect::None, Col, true);

        Tick = (Tick + 1) % 4;
    }

    Tick = 0;

    for (int Y = 0; Y <= Radius; Y += Step)
    {
        const float SY = Y * Scale;

        const FLinearColor Col = (Tick == 0)
            ? FLinearColor(0.2f, 0.2f, 0.2f)
            : FLinearColor(0.08f, 0.08f, 0.08f);

        FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(),
            { FVector2D(Left, Center.Y + SY), FVector2D(Right, Center.Y + SY) },
            ESlateDrawEffect::None, Col, true);

        FSlateDrawElement::MakeLines(Out, Layer, Geo.ToPaintGeometry(),
            { FVector2D(Left, Center.Y - SY), FVector2D(Right, Center.Y - SY) },
            ESlateDrawEffect::None, Col, true);

        Tick = (Tick + 1) % 4;
    }
}

int32 USectorMapPanel::ComputeRepLevel(double R) const
{
    if (R > 250000.0) return 1;
    if (R > 70000.0) return 2;
    return 3;
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

void USectorMapPanel::RefreshView()
{
    CachedRuntimeSystem = ResolveRuntimeSystem(GetGameInstance(), ViewedSystemName);
    CachedRegion = nullptr;
    bValidView = false;

    if (!CachedRuntimeSystem)
    {
        Invalidate(EInvalidateWidget::Paint);
        return;
    }

    // 1. Try explicit viewed sector name first
    if (!ViewedSectorName.IsEmpty())
    {
        ListIter<OrbitalRegion> RegionIter = CachedRuntimeSystem->AllRegions();
        while (++RegionIter)
        {
            OrbitalRegion* Region = RegionIter.value();
            if (!Region)
            {
                continue;
            }

            if (ViewedSectorName.Equals(ANSI_TO_TCHAR(Region->GetName()), ESearchCase::IgnoreCase))
            {
                CachedRegion = Region;
                break;
            }
        }
    }

    // 2. Fallback to active region if no named sector was resolved
    if (!CachedRegion)
    {
        CachedRegion = CachedRuntimeSystem->ActiveRegion();
    }

    // 3. Final fallback to first available region
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

    bValidView = (CachedRegion != nullptr);

    if (bValidView && ViewedSectorName.IsEmpty())
    {
        ViewedSectorName = ANSI_TO_TCHAR(CachedRegion->GetName());
    }

    Invalidate(EInvalidateWidget::Paint);
}

FReply USectorMapPanel::NativeOnMouseButtonDown(
    const FGeometry& Geo,
    const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bDraggingMap = true;
        DragStartScreenPosition = Event.GetScreenSpacePosition();
        DragStartPanOffset = PanOffset;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(Geo, Event);
}

FReply USectorMapPanel::NativeOnMouseButtonUp(
    const FGeometry& Geo,
    const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() == EKeys::RightMouseButton)
    {
        bDraggingMap = false;
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonUp(Geo, Event);
}

FReply USectorMapPanel::NativeOnMouseMove(
    const FGeometry& Geo,
    const FPointerEvent& Event)
{
    if (bDraggingMap)
    {
        const FVector2D Delta =
            Event.GetScreenSpacePosition() - DragStartScreenPosition;

        PanOffset = ClampPanOffset(
            DragStartPanOffset + Delta,
            Geo.GetLocalSize());

        Invalidate(EInvalidateWidget::Paint);
        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(Geo, Event);
}

FReply USectorMapPanel::NativeOnMouseWheel(
    const FGeometry& Geo,
    const FPointerEvent& Event)
{
    if (Event.GetWheelDelta() > 0)
    {
        ZoomIn();
        return FReply::Handled();
    }

    if (Event.GetWheelDelta() < 0)
    {
        ZoomOut();
        return FReply::Handled();
    }

    return Super::NativeOnMouseWheel(Geo, Event);
}

FVector2D USectorMapPanel::ClampPanOffset(
    const FVector2D& In,
    const FVector2D& Size) const
{
    const float LimitX = FMath::Max(100.0f, Size.X * (ZoomScale - 1.0f));
    const float LimitY = FMath::Max(100.0f, Size.Y * (ZoomScale - 1.0f));

    return FVector2D(
        FMath::Clamp(In.X, -LimitX, LimitX),
        FMath::Clamp(In.Y, -LimitY, LimitY));
}

void USectorMapPanel::ZoomIn()
{
    ZoomScale = FMath::Clamp(ZoomScale + 0.1f, MinZoomScale, MaxZoomScale);
    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::ZoomOut()
{
    ZoomScale = FMath::Clamp(ZoomScale - 0.1f, MinZoomScale, MaxZoomScale);
    Invalidate(EInvalidateWidget::Paint);
}