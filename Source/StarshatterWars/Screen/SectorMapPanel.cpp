/*  Project Starshatter Wars
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
*/

#include "SectorMapPanel.h"

#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"

void USectorMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();
    RefreshView();
}

void USectorMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    ViewedSystemName = InSystemName;
    RefreshView();
}

void USectorMapPanel::SetViewedSectorName(const FString& InSectorName)
{
    ViewedSectorName = InSectorName;
    RefreshView();
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

    if (!RootBorder)
    {
        RootBorder =
            WidgetTree->ConstructWidget<UBorder>(
                UBorder::StaticClass(),
                TEXT("SectorMapRootBorder"));

        RootBorder->SetBrush(FSlateBrush());
        RootBorder->SetBrushColor(FLinearColor(0.03f, 0.04f, 0.08f, 0.40f));
        RootBorder->SetPadding(FMargin(24.f));

        if (UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(RootBorder))
        {
            BorderSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            BorderSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        }
    }

    if (!ContentBox)
    {
        ContentBox =
            WidgetTree->ConstructWidget<UVerticalBox>(
                UVerticalBox::StaticClass(),
                TEXT("SectorMapContentBox"));

        RootBorder->SetContent(ContentBox);
    }

    if (!HeaderText)
    {
        HeaderText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SectorMapHeaderText"));

        HeaderText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        HeaderText->SetFont(MissionUIStyle::GetHeaderFont(22));
        HeaderText->SetJustification(ETextJustify::Left);

        if (UVerticalBoxSlot* HeaderSlot = ContentBox->AddChildToVerticalBox(HeaderText))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
            HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    if (!BodyText)
    {
        BodyText =
            WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(),
                TEXT("SectorMapBodyText"));

        BodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        BodyText->SetFont(MissionUIStyle::GetInfoValueFont());
        BodyText->SetJustification(ETextJustify::Left);
        BodyText->SetAutoWrapText(true);

        if (UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(BodyText))
        {
            BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }
}

void USectorMapPanel::RefreshView()
{
    if (HeaderText)
    {
        HeaderText->SetText(FText::FromString(TEXT("SECTOR MAP")));
    }

    FString DisplayText;

    if (ViewedSystemName.IsEmpty() && ViewedSectorName.IsEmpty())
    {
        DisplayText =
            TEXT("NO SYSTEM OR SECTOR SELECTED.\n\n")
            TEXT("SECTOR MAP PANEL ONLINE.\n\n")
            TEXT("NEXT STEP:\n")
            TEXT("RENDER REGIONS, STATIONS, STARSHIPS, FIGHTERS, AND MISSION OBJECTS HERE.");
    }
    else
    {
        DisplayText =
            FString::Printf(
                TEXT("CURRENT SYSTEM: %s\n")
                TEXT("CURRENT SECTOR: %s\n\n")
                TEXT("SECTOR MAP PANEL ONLINE.\n\n")
                TEXT("NEXT STEP:\n")
                TEXT("RENDER REGIONS, STATIONS, STARSHIPS, FIGHTERS, AND MISSION OBJECTS HERE."),
                ViewedSystemName.IsEmpty() ? TEXT("NONE") : *ViewedSystemName,
                ViewedSectorName.IsEmpty() ? TEXT("NONE") : *ViewedSectorName);
    }

    if (BodyText)
    {
        BodyText->SetText(FText::FromString(DisplayText));
    }
}