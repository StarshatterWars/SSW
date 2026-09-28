/*
    Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         SectorMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    USectorMapPanel

    Shared read-only region view for Mission Navigation and Operations.
*/

#include "SectorMapPanel.h"

#include "MissionNavDlg.h"
#include "StarshatterEnvironmentSubsystem.h"
#include "StarSystem.h"
#include "OrbitalRegion.h"
#include "Mission.h"
#include "MissionElement.h"
#include "Instruction.h"
#include "GameStructs.h"
#include "FormattingUtils.h"
#include "GameStructs_System.h"

#include "CombatUnit.h"
#include "CombatGroup.h"
#include "CombatGroupRegistry.h"
#include "ShipDesignRegistry.h"

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

    static FLinearColor GetMapIFFColor(const MissionElement* Element)
    {
        if (!Element)
        {
            return FLinearColor::White;
        }

        const int32 IFF = Element->GetIFF();

        // Adjust these to your actual alliance/player IFF rules if needed
        if (IFF == 1)
        {
            return FLinearColor(0.25f, 0.65f, 1.0f, 1.0f); // allied blue
        }

        if (IFF == 0)
        {
            return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f); // neutral gray
        }

        return FLinearColor(1.0f, 0.25f, 0.25f, 1.0f); // enemy red
    }
}

void USectorMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();

    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    if (RootCanvas)
    {
        RootCanvas->SetVisibility(ESlateVisibility::Visible);
        RootCanvas->SetClipping(EWidgetClipping::ClipToBounds);
    }

    PanOffset = FVector2D::ZeroVector;
    ZoomScale = bOperationsView ? 1.0f : 12.0f;
    bDraggingMap = false;
    DragStartScreenPosition = FVector2D::ZeroVector;
    DragStartPanOffset = FVector2D::ZeroVector;

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
    auto DrawRegionInfo = [&](int32 BaseLayer) -> int32
    {
        if (!bOperationsView) return BaseLayer;

        const FString RegionName = ANSI_TO_TCHAR(CachedRegion->GetName());
        const Orbital* Parent = CachedRegion->Primary();
        const FString ParentName = Parent
            ? FString(ANSI_TO_TCHAR(Parent->GetName())) : TEXT("Unknown parent");

        TArray<FString> ObjectNames;
        for (const FS_CombatGroup* Group : CombatGroupRegistry::FindByRegion(RegionName))
        {
            if (!Group || !OperationsGroupMatchesView(*Group)) continue;
            const FString DisplayName = Group->DisplayName.TrimStartAndEnd();
            ObjectNames.Add(DisplayName.IsEmpty() ? Group->Name : DisplayName);
        }

        FLinearColor EmpireColor = FLinearColor::Gray;
        if (UGameInstance* GI = GetGameInstance())
        {
            if (UStarshatterEnvironmentSubsystem* Env =
                GI->GetSubsystem<UStarshatterEnvironmentSubsystem>())
            {
                if (const FS_Galaxy* Data = Env->FindGalaxyByName(ViewedSystemName))
                {
                    switch (Data->Empire)
                    {
                    case EEMPIRE_NAME::Terellian: EmpireColor = FLinearColor::Green; break;
                    case EEMPIRE_NAME::Marakan: EmpireColor = FLinearColor::Red; break;
                    default: break;
                    }
                }
            }
        }

        const float Width = FMath::Max(1.0f,
            FMath::Min(280.0f, static_cast<float>(PanelSize.X) - 32.0f));
        const float Height = 104.0f + FMath::Max(1, ObjectNames.Num()) * 20.0f;
        const FVector2D Origin(
            FMath::Max(0.0, PanelSize.X - Width - 16.0),
            FMath::Max(0.0, PanelSize.Y - Height - 16.0));

        FSlateDrawElement::MakeBox(
            OutDrawElements, ++BaseLayer,
            AllottedGeometry.ToPaintGeometry(Origin, FVector2D(Width, Height)),
            FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None,
            FLinearColor(0.025f, 0.04f, 0.065f, 0.94f));

        auto DrawLine = [&](const FString& Text, float Y, int32 FontSize,
            const FLinearColor& Color)
        {
            FSlateDrawElement::MakeText(
                OutDrawElements, ++BaseLayer,
                AllottedGeometry.ToPaintGeometry(
                    Origin + FVector2D(14.0f, Y), FVector2D(Width - 28.0f, 22.0f)),
                Text, FCoreStyle::GetDefaultFontStyle("Regular", FontSize),
                ESlateDrawEffect::None, Color);
        };
        DrawLine(RegionName, 12.0f, 16, EmpireColor);
        DrawLine(ParentName, 38.0f, 11, EmpireColor);
        DrawLine(TEXT("MAJOR OBJECTS"), 70.0f, 11,
            FLinearColor(0.65f, 0.78f, 0.92f));
        if (ObjectNames.IsEmpty())
            DrawLine(TEXT("None"), 94.0f, 11, FLinearColor::White);
        for (int32 Index = 0; Index < ObjectNames.Num(); ++Index)
            DrawLine(TEXT("- ") + ObjectNames[Index], 94.0f + Index * 20.0f,
                11, FLinearColor::White);
        return BaseLayer;
    };

    const int32 RegionRadius = static_cast<int32>(CachedRegion->Radius());
    const int32 GridStep = static_cast<int32>(CachedRegion->GetGridSpace());

    if (RegionRadius <= 0 || GridStep <= 0)
    {
        return DrawRegionInfo(LayerId);
    }

    const double C = FMath::Min(PanelSize.X * 0.5, PanelSize.Y * 0.5);
    const double R = GetDisplayRadius() / ZoomScale;
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

    if (bOperationsView)
    {
        // Operations/Theater uses the persistent/static combat-group registry.
        DrawOperationsGroups(
            OutDrawElements,
            AllottedGeometry,
            LayerId + 200,
            Center,
            Scale,
            Rep);
    }
    else
    {
        // Mission Navigation remains mission-instance driven.
        DrawMissionNavRoutes(
            OutDrawElements,
            AllottedGeometry,
            LayerId + 100,
            Center,
            Scale,
            Rep);

        DrawMissionElements(
            OutDrawElements,
            AllottedGeometry,
            LayerId + 200,
            Center,
            Scale,
            Rep);
    }

    const FString InfoString = FString::Printf(
        TEXT("SECTOR: %s    REP: %d    SCALE: %.4f    RANGE: %.0f"),
        ViewedSectorName.IsEmpty() ? TEXT("NONE") : *ViewedSectorName,
        Rep,
        Scale,
        R * 2.0);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 500,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(12.0f, 10.0f),
            FVector2D(520.0f, 20.0f)),
        InfoString,
        FCoreStyle::GetDefaultFontStyle("Regular", 11),
        ESlateDrawEffect::None,
        FLinearColor(0.85f, 0.90f, 1.0f, 0.95f));

    return DrawRegionInfo(LayerId + 500);
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

        return FReply::Handled().CaptureMouse(TakeWidget());
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        if (!bValidView || !CachedRegion)
        {
            return FReply::Handled();
        }

        const FVector2D PanelSize = InGeometry.GetLocalSize();
        const FVector2D Center = (PanelSize * 0.5f) + PanOffset;

        const double C = FMath::Min(PanelSize.X * 0.5, PanelSize.Y * 0.5);
        const double R = GetDisplayRadius() / ZoomScale;
        const float Scale = (R > 0.0) ? static_cast<float>(C / R) : 1.0f;

        const FVector2D LocalPoint =
            InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());

        if (bOperationsView)
        {
            const FS_CombatGroup* HitGroup =
                HitTestOperationsGroupAtLocalPoint(LocalPoint, Center, Scale);

            if (HitGroup)
            {
                SelectedOperationsGroup = HitGroup;
                SelectedElement = nullptr;
                Invalidate(EInvalidateWidget::Paint);

                if (OnOperationsGroupSelected.IsBound())
                {
                    OnOperationsGroupSelected.Execute(HitGroup);
                }
            }
        }
        else
        {
            MissionElement* HitElement =
                HitTestMissionElementAtLocalPoint(LocalPoint, Center, Scale);

            if (HitElement)
            {
                SelectedElement = HitElement;
                SelectedOperationsGroup = nullptr;
                Invalidate(EInvalidateWidget::Paint);

                if (OnElementSelected.IsBound())
                {
                    OnElementSelected.Execute(HitElement);
                }
                else if (OwnerNavDlg)
                {
                    OwnerNavDlg->HandleSectorMissionElementSelected(HitElement);
                }
            }
        }

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
        return FReply::Handled().ReleaseMouseCapture();
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

    UE_LOG(LogTemp, Warning, TEXT("[SectorMapPanel] NativeOnMouseWheel Delta=%.3f"), WheelDelta);

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

void USectorMapPanel::SetOwnerNavDlg(UMissionNavDlg* InOwnerNavDlg)
{
    OwnerNavDlg = InOwnerNavDlg;
}

void USectorMapPanel::SetOperationsView(bool bInOperationsView)
{
    if (bOperationsView != bInOperationsView)
    {
        ZoomScale = bInOperationsView ? 1.0f : 12.0f;
        PanOffset = FVector2D::ZeroVector;
    }
    bOperationsView = bInOperationsView;

    if (bOperationsView)
    {
        SelectedElement = nullptr;
    }
    else
    {
        SelectedOperationsGroup = nullptr;
    }

    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    if (bOperationsView && ViewedSystemName != InSystemName.TrimStartAndEnd())
    {
        ZoomScale = 1.0f;
        PanOffset = FVector2D::ZeroVector;
    }
    ViewedSystemName = InSystemName.TrimStartAndEnd();
    RefreshView();
}

void USectorMapPanel::SetViewedSectorName(const FString& InSectorName)
{
    if (bOperationsView && ViewedSectorName != InSectorName.TrimStartAndEnd())
    {
        ZoomScale = 1.0f;
        PanOffset = FVector2D::ZeroVector;
    }
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
    if (InElement)
    {
        SelectedOperationsGroup = nullptr;
    }

    Invalidate(EInvalidateWidget::Paint);
}

void USectorMapPanel::SetSelectedOperationsGroup(const FS_CombatGroup* InGroup)
{
    SelectedOperationsGroup = InGroup;
    if (InGroup)
    {
        SelectedElement = nullptr;
    }

    Invalidate(EInvalidateWidget::Paint);
}

bool USectorMapPanel::CenterOnElement(MissionElement* InElement)
{
    if (!InElement || !CachedRegion)
    {
        return false;
    }

    if (bOperationsView)
    {
        return false;
    }

    const FVector2D PanelSize = GetCachedGeometry().GetLocalSize();
    if (PanelSize.X <= 1.0f || PanelSize.Y <= 1.0f)
    {
        return false;
    }

    const double C = FMath::Min(PanelSize.X * 0.5, PanelSize.Y * 0.5);
    const double R = GetDisplayRadius() / ZoomScale;
    const float Scale = (R > 0.0) ? static_cast<float>(C / R) : 1.0f;

    const FVector ElementLocation = InElement->GetLocation();
    const FVector2D Offset(
        ElementLocation.X * Scale,
        ElementLocation.Y * Scale);

    PanOffset = -Offset;
    PanOffset = ClampPanOffset(PanOffset, PanelSize);

    Invalidate(EInvalidateWidget::Paint);
    return true;
}

bool USectorMapPanel::CenterOnOperationsGroup(const FS_CombatGroup* InGroup)
{
    if (!bOperationsView || !InGroup || !CachedRegion)
    {
        return false;
    }

    if (!OperationsGroupMatchesView(*InGroup))
    {
        return false;
    }

    const FVector2D PanelSize = GetCachedGeometry().GetLocalSize();
    if (PanelSize.X <= 1.0f || PanelSize.Y <= 1.0f)
    {
        return false;
    }

    const double C = FMath::Min(PanelSize.X * 0.5, PanelSize.Y * 0.5);
    const double R = GetDisplayRadius() / ZoomScale;
    const float Scale = (R > 0.0) ? static_cast<float>(C / R) : 1.0f;

    const FVector2D Offset(
        InGroup->Location.X * Scale,
        InGroup->Location.Y * Scale);

    SelectedOperationsGroup = InGroup;
    SelectedElement = nullptr;

    PanOffset = ClampPanOffset(-Offset, PanelSize);

    Invalidate(EInvalidateWidget::Paint);
    return true;
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

double USectorMapPanel::GetDisplayRadius() const
{
    if (!CachedRegion) return 1.0;
    const double RegionRadius = FMath::Max(1.0, CachedRegion->Radius());
    if (!bOperationsView) return RegionRadius;

    double ExtentX = RegionRadius;
    double ExtentY = RegionRadius;
    for (const FS_CombatGroup* Group :
        CombatGroupRegistry::FindByRegion(ANSI_TO_TCHAR(CachedRegion->GetName())))
    {
        if (!Group || !OperationsGroupMatchesView(*Group)) continue;
        ExtentX = FMath::Max(ExtentX, FMath::Abs(static_cast<double>(Group->Location.X)));
        ExtentY = FMath::Max(ExtentY, FMath::Abs(static_cast<double>(Group->Location.Y)));
    }

    const FVector2D Size = GetCachedGeometry().GetLocalSize();
    if (Size.X <= 0.0 || Size.Y <= 0.0)
        return FMath::Max(ExtentX, ExtentY) * 1.15;

    const double HalfMin = FMath::Min(Size.X, Size.Y) * 0.5;
    // Leave the right-hand info panel clear, plus room for marker labels.
    const double AvailableX = FMath::Max(32.0, Size.X * 0.5 - 320.0);
    const double AvailableY = FMath::Max(32.0, Size.Y * 0.5 - 48.0);
    return FMath::Max(ExtentX * HalfMin / AvailableX,
        ExtentY * HalfMin / AvailableY) * 1.10;
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

    // 1. Try the explicitly requested sector first
    if (!ResolveViewedRegion(CachedRuntimeSystem, CachedRegion) || !CachedRegion)
    {
        // 2. Fallback to active region only if no valid mission/requested region was found
        CachedRegion = CachedRuntimeSystem->ActiveRegion();

        // 3. Final fallback to first available region
        if (!CachedRegion)
        {
            ListIter<OrbitalRegion> RegionIter = CachedRuntimeSystem->GetAllRegions();
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

        if (CachedRegion)
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

    ListIter<OrbitalRegion> RegionIter = InSystem->GetAllRegions();
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

float USectorMapPanel::GetElementSpriteSize(const MissionElement* Element, int32 Rep) const
{
    float Size = 24.0f;

    if (!Element)
        return Size;

    if (Element->IsStatic())
    {
        CombatUnit* Unit = Element->GetCombatUnit();
        if (Unit)
        {
            const CLASSIFICATION Type = (CLASSIFICATION)Unit->GetType();

            switch (Type)
            {
            case CLASSIFICATION::STATION:
            case CLASSIFICATION::STARBASE:
                Size = 34.0f; // large
                break;

            case CLASSIFICATION::MINE:
                Size = 12.0f; // small
                break;

            default:
                Size = 20.0f;
                break;
            }
        }
        else
        {
            Size = 22.0f;
        }
    }
    else if (Element->IsStarship())
    {
        Size = 28.0f;
    }
    else if (Element->IsDropship())
    {
        Size = 22.0f;
    }
    else
    {
        Size = 20.0f;
    }

    if (Rep <= 1)
    {
        Size *= 0.85f;
    }
    else if (Rep >= 3)
    {
        Size *= 1.05f;
    }

    return Size;
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

    const FString ActiveRegionName = FString(ANSI_TO_TCHAR(CachedRegion->GetName())).TrimStartAndEnd();

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

        if (!ShouldRenderElement(Element))
        {
            continue;
        }

        const FString ElemRegion = FString(ANSI_TO_TCHAR(Element->GetRegion())).TrimStartAndEnd();

        if (!ElemRegion.Equals(ActiveRegionName, ESearchCase::IgnoreCase))
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

    const bool bIsSelected = (Element == SelectedElement);
    const FLinearColor MarkerColor = ToLinearColor(Element->GetMarkerColor());

    float HalfSize = 3.0f;

    if (Rep <= 1)
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

    bool bDrewSprite = false;

    const FShipDesign* Design = ResolveShipDesignForElement(Element);
    if (Design && Design->Map.Num() > 0)
    {
        const int32 FacingIndex = 0;

        if (Design->Map.IsValidIndex(FacingIndex))
        {
            const FString ShipName = Design->ShipName;
            const FString SpriteName = Design->Map[FacingIndex].SpriteName;

            if (!ShipName.IsEmpty() && !SpriteName.IsEmpty())
            {
                UTexture2D* SpriteTex =
                    const_cast<USectorMapPanel*>(this)->GetShipMapSprite(ShipName, SpriteName);

                if (SpriteTex)
                {
                    FSlateBrush Brush;
                    Brush.SetResourceObject(SpriteTex);

                    float SpriteSize = GetElementSpriteSize(Element, Rep);

                    Brush.ImageSize = FVector2D(SpriteSize, SpriteSize);

                    const FVector2D DrawPos =
                        ScreenPos - (Brush.ImageSize * 0.5f);

                    FSlateDrawElement::MakeBox(
                        OutDrawElements,
                        BaseLayerId + 1,
                        AllottedGeometry.ToPaintGeometry(DrawPos, Brush.ImageSize),
                        &Brush,
                        ESlateDrawEffect::None,
                        GetMapIFFColor(Element));

                    bDrewSprite = true;
                    HalfSize = SpriteSize * 0.5f;
                }
            }
        }
    }

    if (!bDrewSprite)
    {
        const FVector2D TopLeft(ScreenPos.X - HalfSize, ScreenPos.Y - HalfSize);
        const FVector2D DrawSize(HalfSize * 2.0f, HalfSize * 2.0f);

        FSlateDrawElement::MakeBox(
            OutDrawElements,
            BaseLayerId + 1,
            AllottedGeometry.ToPaintGeometry(TopLeft, DrawSize),
            FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None,
            MarkerColor);
    }

    if (bIsSelected)
    {
        const FVector2D SelTopLeft(
            ScreenPos.X - HalfSize - 2.0f,
            ScreenPos.Y - HalfSize - 2.0f);

        const FVector2D SelSize(
            (HalfSize + 2.0f) * 2.0f,
            (HalfSize + 2.0f) * 2.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 2,
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

        DrawSelectionCrosshair(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId + 3,
            ScreenPos,
            HalfSize + 8.0f);

        DrawSelectedElementTag(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId + 5,
            ScreenPos,
            Element);
    }

    const bool bCrowded = IsElementCrowded(Element, Scale);
    bool bDrawLabel = false;

    if (bIsSelected)
    {
        bDrawLabel = true;
    }
    else if (Rep >= 3)
    {
        bDrawLabel = !bCrowded;
    }
    else if (Rep == 2)
    {
        bDrawLabel = !bCrowded;
    }
    else
    {
        bDrawLabel = false;
    }

    if (bDrawLabel)
    {
        FString LabelText;

        if (Element->Count() > 1)
        {
            LabelText = FString::Printf(
                TEXT("%s x %d"),
                ANSI_TO_TCHAR(Element->GetName()),
                Element->Count());
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

bool USectorMapPanel::OperationsGroupMatchesView(const FS_CombatGroup& Group) const
{
    if (!bOperationsView || !CachedRegion)
    {
        return false;
    }

    if (Group.Type != ECOMBATGROUP_TYPE::STATION &&
        Group.Type != ECOMBATGROUP_TYPE::STARBASE &&
        Group.Type != ECOMBATGROUP_TYPE::FLEET &&
        Group.Type != ECOMBATGROUP_TYPE::CARRIER_GROUP &&
        Group.Type != ECOMBATGROUP_TYPE::DESTROYER_SQUADRON &&
        Group.Type != ECOMBATGROUP_TYPE::BATTLE_GROUP)
    {
        return false;
    }

    const FString ActiveRegionName =
        FString(ANSI_TO_TCHAR(CachedRegion->GetName())).TrimStartAndEnd();

    if (!Group.Region.TrimStartAndEnd().Equals(
            ActiveRegionName,
            ESearchCase::IgnoreCase))
    {
        return false;
    }

    // Existing roster rows may not populate System. Only enforce it when
    // the static row actually provides one.
    const FString GroupSystem = Group.System.TrimStartAndEnd();
    if (!GroupSystem.IsEmpty() &&
        !ViewedSystemName.IsEmpty() &&
        !GroupSystem.Equals(ViewedSystemName, ESearchCase::IgnoreCase))
    {
        return false;
    }

    return true;
}

void USectorMapPanel::DrawOperationsGroups(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 BaseLayerId,
    const FVector2D& Center,
    float Scale,
    int32 Rep) const
{
    if (!bOperationsView || !CachedRegion)
    {
        return;
    }

    const FString ActiveRegionName =
        FString(ANSI_TO_TCHAR(CachedRegion->GetName())).TrimStartAndEnd();

    const TArray<const FS_CombatGroup*> Groups =
        CombatGroupRegistry::FindByRegion(ActiveRegionName);

    for (const FS_CombatGroup* Group : Groups)
    {
        if (!Group || !OperationsGroupMatchesView(*Group))
        {
            continue;
        }

        DrawOperationsGroup(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId,
            Center,
            Scale,
            Rep,
            Group);
    }
}

void USectorMapPanel::DrawOperationsGroup(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 BaseLayerId,
    const FVector2D& Center,
    float Scale,
    int32 Rep,
    const FS_CombatGroup* Group) const
{
    if (!Group || !OperationsGroupMatchesView(*Group))
    {
        return;
    }

    const FVector2D ScreenPos(
        Center.X + (Group->Location.X * Scale),
        Center.Y + (Group->Location.Y * Scale));

    const FVector2D PanelSize = AllottedGeometry.GetLocalSize();
    if (ScreenPos.X < 0.0f || ScreenPos.X >= PanelSize.X ||
        ScreenPos.Y < 0.0f || ScreenPos.Y >= PanelSize.Y)
    {
        return;
    }

    const bool bSelected = (Group == SelectedOperationsGroup);
    const FLinearColor MarkerColor = GetOperationsIFFColor(Group);

    float HalfSize = 5.0f;

    if (Rep <= 1)
    {
        HalfSize = 3.0f;
    }
    else if (Group->Type == ECOMBATGROUP_TYPE::STARBASE)
    {
        HalfSize = 8.0f;
    }
    else
    {
        HalfSize = 6.0f;
    }

    const FVector2D TopLeft(
        ScreenPos.X - HalfSize,
        ScreenPos.Y - HalfSize);

    const FVector2D DrawSize(
        HalfSize * 2.0f,
        HalfSize * 2.0f);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        BaseLayerId + 1,
        AllottedGeometry.ToPaintGeometry(TopLeft, DrawSize),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        MarkerColor);

    // Distinguish a planet-side starbase from an orbital station.
    if (Group->Type == ECOMBATGROUP_TYPE::STARBASE)
    {
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(ScreenPos.X - HalfSize - 3.0f, ScreenPos.Y),
                FVector2D(ScreenPos.X + HalfSize + 3.0f, ScreenPos.Y)
            },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            1.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(ScreenPos.X, ScreenPos.Y - HalfSize - 3.0f),
                FVector2D(ScreenPos.X, ScreenPos.Y + HalfSize + 3.0f)
            },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            1.0f);
    }

    if (bSelected)
    {
        const FVector2D SelTopLeft(
            ScreenPos.X - HalfSize - 3.0f,
            ScreenPos.Y - HalfSize - 3.0f);

        const FVector2D SelSize(
            (HalfSize + 3.0f) * 2.0f,
            (HalfSize + 3.0f) * 2.0f);

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

        DrawSelectionCrosshair(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId + 4,
            ScreenPos,
            HalfSize + 8.0f);

        DrawSelectedOperationsGroupTag(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId + 6,
            ScreenPos,
            Group);
    }

    const bool bCrowded = IsOperationsGroupCrowded(Group, Scale);
    const bool bDrawLabel = bSelected || ((Rep >= 2) && !bCrowded);

    if (bDrawLabel)
    {
        FString Label = Group->DisplayName.TrimStartAndEnd();

        if (Label.IsEmpty())
        {
            Label = Group->Name.TrimStartAndEnd();
        }

        if (Label.IsEmpty())
        {
            Label = UFormattingUtils::GetGroupTypeDisplayName(Group->Type);
        }

        FSlateDrawElement::MakeText(
            OutDrawElements,
            BaseLayerId + 5,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(ScreenPos.X + HalfSize + 4.0f, ScreenPos.Y - 6.0f),
                FVector2D(240.0f, 16.0f)),
            Label,
            FCoreStyle::GetDefaultFontStyle("Regular", 10),
            ESlateDrawEffect::None,
            FLinearColor::White);
    }
}

const FS_CombatGroup* USectorMapPanel::HitTestOperationsGroupAtLocalPoint(
    const FVector2D& LocalPoint,
    const FVector2D& Center,
    float Scale) const
{
    if (!bOperationsView || !CachedRegion)
    {
        return nullptr;
    }

    const FString ActiveRegionName =
        FString(ANSI_TO_TCHAR(CachedRegion->GetName())).TrimStartAndEnd();

    const TArray<const FS_CombatGroup*> Groups =
        CombatGroupRegistry::FindByRegion(ActiveRegionName);

    const FS_CombatGroup* BestGroup = nullptr;
    double BestDistSq = TNumericLimits<double>::Max();

    for (const FS_CombatGroup* Group : Groups)
    {
        if (!Group || !OperationsGroupMatchesView(*Group))
        {
            continue;
        }

        const FVector2D ScreenPos(
            Center.X + (Group->Location.X * Scale),
            Center.Y + (Group->Location.Y * Scale));

        const double DX = LocalPoint.X - ScreenPos.X;
        const double DY = LocalPoint.Y - ScreenPos.Y;
        const double DistSq = (DX * DX) + (DY * DY);
        const double PickRadiusSq = 196.0; // 14 px

        if (DistSq <= PickRadiusSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestGroup = Group;
        }
    }

    return BestGroup;
}

bool USectorMapPanel::FindOperationsGroupScreenPosition(
    const FS_CombatGroup* Group,
    const FVector2D& Center,
    float Scale,
    FVector2D& OutScreenPos) const
{
    OutScreenPos = FVector2D::ZeroVector;

    if (!Group || !OperationsGroupMatchesView(*Group))
    {
        return false;
    }

    OutScreenPos = FVector2D(
        Center.X + (Group->Location.X * Scale),
        Center.Y + (Group->Location.Y * Scale));

    return true;
}

bool USectorMapPanel::IsOperationsGroupCrowded(
    const FS_CombatGroup* TestGroup,
    float Scale) const
{
    if (!TestGroup || !CachedRegion)
    {
        return false;
    }

    const FString ActiveRegionName =
        FString(ANSI_TO_TCHAR(CachedRegion->GetName())).TrimStartAndEnd();

    const TArray<const FS_CombatGroup*> Groups =
        CombatGroupRegistry::FindByRegion(ActiveRegionName);

    for (const FS_CombatGroup* RefGroup : Groups)
    {
        if (!RefGroup ||
            RefGroup == TestGroup ||
            !OperationsGroupMatchesView(*RefGroup))
        {
            continue;
        }

        const double DX =
            (TestGroup->Location.X - RefGroup->Location.X) * Scale;

        const double DY =
            (TestGroup->Location.Y - RefGroup->Location.Y) * Scale;

        const double DistSq = (DX * DX) + (DY * DY);

        if (DistSq <= 100.0)
        {
            return true;
        }
    }

    return false;
}

void USectorMapPanel::DrawSelectedOperationsGroupTag(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 LayerId,
    const FVector2D& ScreenPos,
    const FS_CombatGroup* Group) const
{
    if (!Group)
    {
        return;
    }

    const FLinearColor IFFColor = GetOperationsIFFColor(Group);

    FString NameLine = Group->DisplayName.TrimStartAndEnd();
    if (NameLine.IsEmpty())
    {
        NameLine = Group->Name.TrimStartAndEnd();
    }

    if (NameLine.IsEmpty())
    {
        NameLine = UFormattingUtils::GetGroupTypeDisplayName(Group->Type);
    }

    const FString TypeLine =
        UFormattingUtils::GetGroupTypeDisplayName(Group->Type);

    const FString LocLine = FString::Printf(
        TEXT("%s  LOC %.0f, %.0f, %.0f"),
        *TypeLine,
        Group->Location.X,
        Group->Location.Y,
        Group->Location.Z);

    const FVector2D TagPos(ScreenPos.X + 18.0f, ScreenPos.Y - 28.0f);
    const FVector2D TagSize(260.0f, 34.0f);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(TagPos, TagSize),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor(
            IFFColor.R * 0.10f,
            IFFColor.G * 0.10f,
            IFFColor.B * 0.10f,
            0.82f));

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(TagPos.X, TagPos.Y),
            FVector2D(TagPos.X + TagSize.X, TagPos.Y)
        },
        ESlateDrawEffect::None,
        IFFColor,
        true,
        1.5f);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(TagPos.X + 6.0f, TagPos.Y + 3.0f),
            FVector2D(250.0f, 14.0f)),
        NameLine,
        FCoreStyle::GetDefaultFontStyle("Regular", 10),
        ESlateDrawEffect::None,
        IFFColor);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(TagPos.X + 6.0f, TagPos.Y + 17.0f),
            FVector2D(250.0f, 14.0f)),
        LocLine,
        FCoreStyle::GetDefaultFontStyle("Regular", 9),
        ESlateDrawEffect::None,
        FLinearColor(0.85f, 0.90f, 1.0f, 1.0f));
}

FLinearColor USectorMapPanel::GetOperationsIFFColor(
    const FS_CombatGroup* Group) const
{
    if (!Group)
    {
        return FLinearColor::White;
    }

    if (Group->Iff == 1)
    {
        return FLinearColor(0.25f, 0.65f, 1.0f, 1.0f);
    }

    if (Group->Iff == 0)
    {
        return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f);
    }

    return FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);
}

void USectorMapPanel::DrawMissionNavRoutes(
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

        DrawMissionNavRouteForElement(
            OutDrawElements,
            AllottedGeometry,
            BaseLayerId,
            Center,
            Scale,
            Rep,
            Element);
    }
}

void USectorMapPanel::DrawMissionNavRouteForElement(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 BaseLayerId,
    const FVector2D& Center,
    float Scale,
    int32 Rep,
    MissionElement* Element) const
{
    if (!Element || !CachedRegion)
    {
        return;
    }

    const bool bIsSelected = (Element == SelectedElement);

    FLinearColor RouteColor = ToLinearColor(Element->GetMarkerColor());
    RouteColor.A = bIsSelected ? 0.95f : 0.55f;

    const float RouteThickness = bIsSelected ? 2.0f : 1.0f;

    const FVector ElementLocation = Element->GetLocation();
    const FVector2D ElementScreenPos(
        Center.X + (ElementLocation.X * Scale),
        Center.Y + (ElementLocation.Y * Scale));

    bool bHaveFirstPointInRegion = false;
    FVector2D FirstNavScreen = FVector2D::ZeroVector;

    bool bHavePrevPointInRegion = false;
    FVector2D PrevNavScreen = FVector2D::ZeroVector;

    int32 NavIndex = 0;

    ListIter<Instruction> NavIter = Element->NavList();
    while (++NavIter)
    {
        Instruction* Nav = NavIter.value();
        if (!Nav)
        {
            ++NavIndex;
            continue;
        }

        if (_stricmp(Nav->GetRegionName(), CachedRegion->GetName()) != 0)
        {
            ++NavIndex;
            continue;
        }

        const FVector NavWorld = Nav->GetLocation();
        const FVector2D NavScreen(
            Center.X + (NavWorld.X * Scale),
            Center.Y + (NavWorld.Y * Scale));

        if (!bHaveFirstPointInRegion)
        {
            bHaveFirstPointInRegion = true;
            FirstNavScreen = NavScreen;
        }

        if (bHavePrevPointInRegion)
        {
            FSlateDrawElement::MakeLines(
                OutDrawElements,
                BaseLayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                { PrevNavScreen, NavScreen },
                ESlateDrawEffect::None,
                RouteColor,
                true,
                RouteThickness);
        }

        const float MarkerHalf = (Rep <= 1) ? 2.0f : 3.0f;

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(NavScreen.X - MarkerHalf, NavScreen.Y - MarkerHalf),
                FVector2D(NavScreen.X + MarkerHalf, NavScreen.Y + MarkerHalf)
            },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            1.0f);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 2,
            AllottedGeometry.ToPaintGeometry(),
            {
                FVector2D(NavScreen.X - MarkerHalf, NavScreen.Y + MarkerHalf),
                FVector2D(NavScreen.X + MarkerHalf, NavScreen.Y - MarkerHalf)
            },
            ESlateDrawEffect::None,
            FLinearColor::White,
            true,
            1.0f);

        if (Rep >= 2)
        {
            const FString NavLabel = FString::Printf(TEXT("%d"), NavIndex + 1);

            FSlateDrawElement::MakeText(
                OutDrawElements,
                BaseLayerId + 3,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(NavScreen.X + MarkerHalf + 2.0f, NavScreen.Y - 8.0f),
                    FVector2D(24.0f, 14.0f)),
                NavLabel,
                FCoreStyle::GetDefaultFontStyle("Regular", 9),
                ESlateDrawEffect::None,
                FLinearColor::White);
        }

        PrevNavScreen = NavScreen;
        bHavePrevPointInRegion = true;
        ++NavIndex;
    }

    if (bHaveFirstPointInRegion)
    {
        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BaseLayerId + 1,
            AllottedGeometry.ToPaintGeometry(),
            { ElementScreenPos, FirstNavScreen },
            ESlateDrawEffect::None,
            RouteColor,
            true,
            RouteThickness);
    }
}

void USectorMapPanel::DrawSelectionCrosshair(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 LayerId,
    const FVector2D& Center,
    float Radius) const
{
    const float Gap = Radius * 0.45f;
    const float Reach = Radius + 8.0f;
    const FLinearColor CrosshairColor(0.25f, 1.0f, 1.0f, 1.0f);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(Center.X - Reach, Center.Y),
            FVector2D(Center.X - Gap, Center.Y)
        },
        ESlateDrawEffect::None,
        CrosshairColor,
        true,
        1.5f);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(Center.X + Gap, Center.Y),
            FVector2D(Center.X + Reach, Center.Y)
        },
        ESlateDrawEffect::None,
        CrosshairColor,
        true,
        1.5f);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(Center.X, Center.Y - Reach),
            FVector2D(Center.X, Center.Y - Gap)
        },
        ESlateDrawEffect::None,
        CrosshairColor,
        true,
        1.5f);

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(Center.X, Center.Y + Gap),
            FVector2D(Center.X, Center.Y + Reach)
        },
        ESlateDrawEffect::None,
        CrosshairColor,
        true,
        1.5f);
}

bool USectorMapPanel::IsElementCrowded(MissionElement* TestElement, float Scale) const
{
    if (!CachedMission || !CachedRegion || !TestElement)
    {
        return false;
    }

    const FVector TestLocation = TestElement->GetLocation();

    ListIter<MissionElement> ElementIter = CachedMission->GetElements();
    while (++ElementIter)
    {
        MissionElement* RefElement = ElementIter.value();
        if (!RefElement || RefElement == TestElement)
        {
            continue;
        }

        if (RefElement->IsSquadron())
        {
            continue;
        }

        if (_stricmp(RefElement->GetRegion(), CachedRegion->GetName()) != 0)
        {
            continue;
        }

        const FVector RefLocation = RefElement->GetLocation();

        const double DX = (TestLocation.X - RefLocation.X) * Scale;
        const double DY = (TestLocation.Y - RefLocation.Y) * Scale;
        const double DistSq = (DX * DX) + (DY * DY);

        if (DistSq <= 64.0)
        {
            return true;
        }
    }

    return false;
}

MissionElement* USectorMapPanel::HitTestMissionElementAtLocalPoint(
    const FVector2D& LocalPoint,
    const FVector2D& Center,
    float Scale) const
{
    if (!CachedMission || !CachedRegion)
    {
        return nullptr;
    }

    MissionElement* BestElement = nullptr;
    double BestDistSq = TNumericLimits<double>::Max();

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

        const FVector ElementLocation = Element->GetLocation();
        const FVector2D ScreenPos(
            Center.X + (ElementLocation.X * Scale),
            Center.Y + (ElementLocation.Y * Scale));

        const double DX = LocalPoint.X - ScreenPos.X;
        const double DY = LocalPoint.Y - ScreenPos.Y;
        const double DistSq = (DX * DX) + (DY * DY);
        const double PickRadiusSq = 100.0;

        if (DistSq <= PickRadiusSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestElement = Element;
        }
    }

    return BestElement;
}

bool USectorMapPanel::FindMissionElementScreenPosition(
    MissionElement* Element,
    const FVector2D& Center,
    float Scale,
    FVector2D& OutScreenPos) const
{
    OutScreenPos = FVector2D::ZeroVector;

    if (!Element || !CachedRegion)
    {
        return false;
    }

    if (_stricmp(Element->GetRegion(), CachedRegion->GetName()) != 0)
    {
        return false;
    }

    const FVector ElementLocation = Element->GetLocation();

    OutScreenPos = FVector2D(
        Center.X + (ElementLocation.X * Scale),
        Center.Y + (ElementLocation.Y * Scale));

    return true;
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

const FShipDesign* USectorMapPanel::ResolveShipDesignForElement(MissionElement* Element) const
{
    if (!Element)
    {
        return nullptr;
    }

    return Element->GetShipDesign();
}

UTexture2D* USectorMapPanel::GetShipMapSprite(
    const FString& ShipName,
    const FString& SpriteName)
{
    const FString Key = ShipName + TEXT("_") + SpriteName;

    if (UTexture2D** Found = MapSpriteCache.Find(Key))
    {
        return *Found;
    }

    const FString Path = FString::Printf(
        TEXT("/Game/UI/Ships/%s/%s.%s"),
        *ShipName,
        *SpriteName,
        *SpriteName);

    UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Path);

    MapSpriteCache.Add(Key, Tex);

    return Tex;
}

int32 USectorMapPanel::ComputeFacingIndex(float YawRadians) const
{
    float Angle = FMath::Fmod(YawRadians, 2.0f * PI);
    if (Angle < 0.0f)
    {
        Angle += 2.0f * PI;
    }

    const float Slice = (2.0f * PI) / 8.0f;
    return FMath::FloorToInt((Angle + Slice * 0.5f) / Slice) % 8;
}

double USectorMapPanel::ResolveElementHeadingRadians(MissionElement* Element) const
{
    if (!Element)
    {
        return 0.0;
    }

    const double StoredHeading = Element->GetHeading();
    if (!FMath::IsNearlyZero(StoredHeading, 0.0001))
    {
        return StoredHeading;
    }

    const FVector ElemLoc = Element->GetLocation();

    ListIter<Instruction> NavIter = Element->NavList();
    while (++NavIter)
    {
        Instruction* Nav = NavIter.value();
        if (!Nav)
        {
            continue;
        }

        if (_stricmp(Nav->GetRegionName(), Element->GetRegion()) != 0)
        {
            continue;
        }

        const FVector NavLoc = Nav->GetLocation();
        const FVector Delta = NavLoc - ElemLoc;

        if (!Delta.IsNearlyZero())
        {
            return FMath::Atan2(Delta.Y, Delta.X);
        }
    }

    return 0.0;
}

bool USectorMapPanel::IsMajorStructureElement(MissionElement* Element) const
{
    if (!Element)
    {
        return false;
    }

    // Prefer the concrete combat-unit classification when available.
    if (CombatUnit* Unit = Element->GetCombatUnit())
    {
        const CLASSIFICATION Type =
            static_cast<CLASSIFICATION>(Unit->GetType());

        switch (Type)
        {
        case CLASSIFICATION::STATION:
        case CLASSIFICATION::STARBASE:
            return true;

        default:
            break;
        }
    }

    // Some mission elements are represented primarily by their combat group.
    // Keep only persistent major structures; batteries and minefields remain
    // tactical and are intentionally excluded from Operations.
    if (CombatGroup* Group = Element->GetCombatGroup())
    {
        const ECOMBATGROUP_TYPE GroupType = Group->GetType();

        switch (GroupType)
        {
        case ECOMBATGROUP_TYPE::STATION:
        case ECOMBATGROUP_TYPE::STARBASE:
        case ECOMBATGROUP_TYPE::FLEET:
        case ECOMBATGROUP_TYPE::CARRIER_GROUP:
        case ECOMBATGROUP_TYPE::DESTROYER_SQUADRON:
        case ECOMBATGROUP_TYPE::BATTLE_GROUP:
            return true;

        default:
            break;
        }
    }

    return false;
}

bool USectorMapPanel::ShouldRenderElement(MissionElement* Element) const
{
    if (!Element)
    {
        return false;
    }

    // Mission Navigation uses MissionElement visibility/intel rules.
    // Operations uses the separate FS_CombatGroup path.
    return ShouldShowMissionElementInBriefing(Element);
}

bool USectorMapPanel::ShouldShowMissionElementInBriefing(const MissionElement* Element) const
{
    if (!Element || !CachedMission)
    {
        return false;
    }

    const int32 ElemIFF = Element->GetIFF();
    const int32 MissionTeam = CachedMission->GetTeam();

    if (ElemIFF == 0)
    {
        return true;
    }

    if (ElemIFF == MissionTeam)
    {
        return true;
    }

    if (Element->GetIntelLevel() >= EIntel::KNOWN)
    {
        return true;
    }

    return false;
}

void USectorMapPanel::DrawSelectedElementTag(
    FSlateWindowElementList& OutDrawElements,
    const FGeometry& AllottedGeometry,
    int32 LayerId,
    const FVector2D& ScreenPos,
    MissionElement* Element) const
{
    if (!Element)
    {
        return;
    }

    const FVector Loc = Element->GetLocation();
    const FLinearColor IFFColor = GetMapIFFColor(Element);

    const FString NameLine = ANSI_TO_TCHAR(Element->GetName());

    const FString LocLine = FString::Printf(
        TEXT("LOC %.0f, %.0f, %.0f"),
        Loc.X,
        Loc.Y,
        Loc.Z);

    const FVector2D TagPos(ScreenPos.X + 18.0f, ScreenPos.Y - 28.0f);
    const FVector2D TagSize(220.0f, 34.0f);

    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        AllottedGeometry.ToPaintGeometry(TagPos, TagSize),
        FCoreStyle::Get().GetBrush("WhiteBrush"),
        ESlateDrawEffect::None,
        FLinearColor(IFFColor.R * 0.10f, IFFColor.G * 0.10f, IFFColor.B * 0.10f, 0.82f));

    FSlateDrawElement::MakeLines(
        OutDrawElements,
        LayerId + 1,
        AllottedGeometry.ToPaintGeometry(),
        {
            FVector2D(TagPos.X, TagPos.Y),
            FVector2D(TagPos.X + TagSize.X, TagPos.Y)
        },
        ESlateDrawEffect::None,
        IFFColor,
        true,
        1.5f);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(TagPos.X + 6.0f, TagPos.Y + 3.0f),
            FVector2D(210.0f, 14.0f)),
        NameLine,
        FCoreStyle::GetDefaultFontStyle("Regular", 10),
        ESlateDrawEffect::None,
        IFFColor);

    FSlateDrawElement::MakeText(
        OutDrawElements,
        LayerId + 2,
        AllottedGeometry.ToPaintGeometry(
            FVector2D(TagPos.X + 6.0f, TagPos.Y + 17.0f),
            FVector2D(210.0f, 14.0f)),
        LocLine,
        FCoreStyle::GetDefaultFontStyle("Regular", 9),
        ESlateDrawEffect::None,
        FLinearColor(0.85f, 0.90f, 1.0f, 1.0f));
}

FLinearColor USectorMapPanel::GetMapIFFColor(const MissionElement* Element) const
{
    if (!Element)
    {
        return FLinearColor::White;
    }

    const int32 IFF = Element->GetIFF();

    if (CachedMission && IFF == CachedMission->GetTeam())
    {
        return FLinearColor(0.25f, 0.65f, 1.0f, 1.0f); // allied blue
    }

    if (IFF == 0)
    {
        return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f); // neutral gray
    }

    return FLinearColor(1.0f, 0.25f, 0.25f, 1.0f); // enemy red
}