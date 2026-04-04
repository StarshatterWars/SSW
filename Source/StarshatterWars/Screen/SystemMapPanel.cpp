/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         SystemMapPanel.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    USystemMapPanel
*/

#include "SystemMapPanel.h"

#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/SlateBrush.h"

void USystemMapPanel::NativeConstruct()
{
    Super::NativeConstruct();

    BuildRuntimeLayout();
    RefreshView();
}

void USystemMapPanel::SetViewedSystemName(const FString& InSystemName)
{
    ViewedSystemName = InSystemName;
    RefreshView();
}

void USystemMapPanel::BuildRuntimeLayout()
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

        WidgetTree->RootWidget = RootCanvas;
    }

    if (!RootBorder)
    {
        RootBorder =
            WidgetTree->ConstructWidget<UBorder>(
                UBorder::StaticClass(),
                TEXT("SystemMapRootBorder"));

        RootBorder->SetBrush(FSlateBrush());
        RootBorder->SetBrushColor(FLinearColor(0.02f, 0.04f, 0.08f, 0.35f));
        RootBorder->SetPadding(FMargin(24.f));

        if (UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(RootBorder))
        {
            BorderSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
            BorderSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
        }
    }

    UVerticalBox* ContentBox =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("SystemMapContentBox"));

    RootBorder->SetContent(ContentBox);

    HeaderText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("SystemMapHeaderText"));

    HeaderText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    HeaderText->SetFont(MissionUIStyle::GetHeaderFont(22));
    HeaderText->SetJustification(ETextJustify::Left);

    if (UVerticalBoxSlot* HeaderSlot = ContentBox->AddChildToVerticalBox(HeaderText))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
        HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    BodyText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("SystemMapBodyText"));

    BodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
    BodyText->SetFont(MissionUIStyle::GetInfoValueFont());
    BodyText->SetJustification(ETextJustify::Left);
    BodyText->SetAutoWrapText(true);

    if (UVerticalBoxSlot* BodySlot = ContentBox->AddChildToVerticalBox(BodyText))
    {
        BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }
}

void USystemMapPanel::RefreshView()
{
    if (HeaderText)
    {
        HeaderText->SetText(FText::FromString(TEXT("SYSTEM MAP")));
    }

    if (BodyText)
    {
        if (ViewedSystemName.IsEmpty())
        {
            BodyText->SetText(FText::FromString(
                TEXT("NO STAR SYSTEM SELECTED.")));
        }
        else
        {
            BodyText->SetText(FText::FromString(FString::Printf(
                TEXT("CURRENT SYSTEM: %s\n\nSYSTEM MAP PANEL ONLINE.\n\nNEXT STEP:\nRENDER STAR, PLANETS, MOONS, ORBITS, AND SYSTEM OBJECTS HERE."),
                *ViewedSystemName)));
        }
    }
}