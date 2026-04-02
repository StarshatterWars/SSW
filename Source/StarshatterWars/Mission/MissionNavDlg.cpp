/*  Project Starshatter Wars
    Fractal Dev Studios LLC
    Copyright (c) 2025-2026. All Rights Reserved.

    ORIGINAL SYSTEM
    ===============
    Starshatter 4.5 (Destroyer Studios)

    SUBSYSTEM:    Stars.exe (Unreal Port)
    FILE:         MissionNavDlg.cpp
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    UMissionNavDlg

    Stage 3 runtime NAV layout:

      - Compact NAV mode buttons across the top
      - Plain zoom buttons on the top right
      - Local WidgetSwitcher for GALAXY / SYSTEM / SECTOR
      - Right-side radio filter button block
      - Object list panel below filters
      - Detail panel below object list

    This pass builds structure and placeholder content.
*/

#include "MissionNavDlg.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "MenuButton.h"

#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

#include "Input/Reply.h"
#include "InputCoreTypes.h"

// -----------------------------------------------------------------------------
// Construction
// -----------------------------------------------------------------------------

UMissionNavDlg::UMissionNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

// -----------------------------------------------------------------------------
// Setup
// -----------------------------------------------------------------------------

void UMissionNavDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

// -----------------------------------------------------------------------------
// Mission Resolution
// -----------------------------------------------------------------------------

Mission* UMissionNavDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

// -----------------------------------------------------------------------------
// UE Lifecycle
// -----------------------------------------------------------------------------

void UMissionNavDlg::NativeConstruct()
{
    Super::NativeConstruct();

    ensureMsgf(RuntimeHost, TEXT("MissionNavDlg: RuntimeHost is not bound"));

    BuildRuntimeLayout();
    BuildNavModeButtons();

    if (ZoomInButton)
    {
        ZoomInButton->OnClicked.RemoveAll(this);
        ZoomInButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomInClicked);
    }

    if (ZoomOutButton)
    {
        ZoomOutButton->OnClicked.RemoveAll(this);
        ZoomOutButton->OnClicked.AddDynamic(this, &UMissionNavDlg::OnZoomOutClicked);
    }

    RefreshNavModeSelection();
    RefreshFilterSelection();
    SetNavMode(CurrentNavMode);
}

FReply UMissionNavDlg::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

// -----------------------------------------------------------------------------
// Refresh
// -----------------------------------------------------------------------------

void UMissionNavDlg::RefreshFromMission()
{
    MissionPtr = ResolveMission();

    if (NavBodyText)
    {
        switch (CurrentNavMode)
        {
        case EMissionNavMode::GALAXY:
            NavBodyText->SetText(FText::FromString(TEXT("GALAXY NAVIGATION")));
            break;

        case EMissionNavMode::SYSTEM:
            NavBodyText->SetText(FText::FromString(TEXT("SYSTEM NAVIGATION")));
            break;

        case EMissionNavMode::SECTOR:
            NavBodyText->SetText(FText::FromString(TEXT("SECTOR NAVIGATION")));
            break;

        default:
            NavBodyText->SetText(FText::GetEmpty());
            break;
        }
    }

    RefreshObjectListPanel();
    RefreshDetailPanel();
}

// -----------------------------------------------------------------------------
// Runtime Layout
// -----------------------------------------------------------------------------

void UMissionNavDlg::BuildRuntimeLayout()
{
    if (!WidgetTree || !RuntimeHost)
    {
        return;
    }

    if (MainColumn)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);

    MainColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavMainColumn"));

    RuntimeHost->SetContent(MainColumn);

    // -----------------------------------------------------------------
    // TOP CONTROL ROW
    // -----------------------------------------------------------------

    TopButtonRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavTopButtonRow"));

    if (UVerticalBoxSlot* TopRowSlot = MainColumn->AddChildToVerticalBox(TopButtonRow))
    {
        TopRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
        TopRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    NavModeButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavModeButtonBox"));

    if (UHorizontalBoxSlot* LeftSlot = TopButtonRow->AddChildToHorizontalBox(NavModeButtonBox))
    {
        LeftSlot->SetHorizontalAlignment(HAlign_Left);
        LeftSlot->SetVerticalAlignment(VAlign_Center);
        LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    USpacer* MiddleSpacer =
        WidgetTree->ConstructWidget<USpacer>(
            USpacer::StaticClass(),
            TEXT("MissionNavTopSpacer"));

    if (UHorizontalBoxSlot* SpacerSlot = TopButtonRow->AddChildToHorizontalBox(MiddleSpacer))
    {
        SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ZoomButtonBox =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavZoomButtonBox"));

    if (UHorizontalBoxSlot* RightSlot = TopButtonRow->AddChildToHorizontalBox(ZoomButtonBox))
    {
        RightSlot->SetHorizontalAlignment(HAlign_Right);
        RightSlot->SetVerticalAlignment(VAlign_Center);
        RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ZoomOutButton =
        WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            TEXT("MissionNavZoomOutButton"));

    if (UHorizontalBoxSlot* ZoomOutSlot = ZoomButtonBox->AddChildToHorizontalBox(ZoomOutButton))
    {
        ZoomOutSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        ZoomOutSlot->SetHorizontalAlignment(HAlign_Left);
        ZoomOutSlot->SetVerticalAlignment(VAlign_Center);
        ZoomOutSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ZoomOutText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavZoomOutText"));
    ZoomOutText->SetText(FText::FromString(TEXT("-")));
    ZoomOutButton->SetContent(ZoomOutText);

    ZoomInButton =
        WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            TEXT("MissionNavZoomInButton"));

    if (UHorizontalBoxSlot* ZoomInSlot = ZoomButtonBox->AddChildToHorizontalBox(ZoomInButton))
    {
        ZoomInSlot->SetPadding(FMargin(0.f));
        ZoomInSlot->SetHorizontalAlignment(HAlign_Left);
        ZoomInSlot->SetVerticalAlignment(VAlign_Center);
        ZoomInSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ZoomInText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavZoomInText"));
    ZoomInText->SetText(FText::FromString(TEXT("+")));
    ZoomInButton->SetContent(ZoomInText);

    // -----------------------------------------------------------------
    // CONTENT ROW
    // -----------------------------------------------------------------

    ContentRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavContentRow"));

    if (UVerticalBoxSlot* ContentRowSlot = MainColumn->AddChildToVerticalBox(ContentRow))
    {
        ContentRowSlot->SetPadding(FMargin(0.f));
        ContentRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    // Left side: switcher host
    MainViewHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavMainViewHost"));

    if (UHorizontalBoxSlot* MainViewSlot = ContentRow->AddChildToHorizontalBox(MainViewHost))
    {
        MainViewSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        MainViewSlot->SetHorizontalAlignment(HAlign_Fill);
        MainViewSlot->SetVerticalAlignment(VAlign_Fill);
        MainViewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    NavSwitcher =
        WidgetTree->ConstructWidget<UWidgetSwitcher>(
            UWidgetSwitcher::StaticClass(),
            TEXT("MissionNavSwitcher"));
    MainViewHost->SetContent(NavSwitcher);

    GalaxyPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavGalaxyPanelHost"));
    NavSwitcher->AddChild(GalaxyPanelHost);

    SystemPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavSystemPanelHost"));
    NavSwitcher->AddChild(SystemPanelHost);

    SectorPanelHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavSectorPanelHost"));
    NavSwitcher->AddChild(SectorPanelHost);

    NavBodyText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavBodyText"));
    NavBodyText->SetText(FText::FromString(TEXT("SYSTEM NAVIGATION")));
    SystemPanelHost->SetContent(NavBodyText);

    // Right side: filter + list + detail
    RightPanelColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavRightPanelColumn"));

    if (UHorizontalBoxSlot* RightPanelSlot = ContentRow->AddChildToHorizontalBox(RightPanelColumn))
    {
        RightPanelSlot->SetPadding(FMargin(0.f));
        RightPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        RightPanelSlot->SetVerticalAlignment(VAlign_Fill);
        RightPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    BuildRightPanels();
}

// -----------------------------------------------------------------------------
// Top NAV Mode Buttons
// -----------------------------------------------------------------------------

void UMissionNavDlg::BuildNavModeButtons()
{
    if (!NavModeButtonBox || !MenuButtonClass)
    {
        return;
    }

    NavModeButtonBox->ClearChildren();
    NavModeButtons.Empty();

    CreateNavModeButton(TEXT("GALAXY"), NavModeButtonBox);
    CreateNavModeButton(TEXT("SYSTEM"), NavModeButtonBox);
    CreateNavModeButton(TEXT("SECTOR"), NavModeButtonBox);
}

UMenuButton* UMissionNavDlg::CreateNavModeButton(const FString& Label, UHorizontalBox* ParentBox)
{
    if (!ParentBox || !MenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
    if (!NewButton)
    {
        return nullptr;
    }

    NewButton->MenuOption = Label;
    NewButton->WidthOverride = 132.f;
    NewButton->HeightOverride = 34.f;
    NewButton->LabelFontSize = 14;

    if (UTextBlock* LabelText = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
    {
        LabelText->SetText(FText::FromString(Label));
    }

    if (UHorizontalBoxSlot* ButtonSlot = ParentBox->AddChildToHorizontalBox(NewButton))
    {
        ButtonSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        ButtonSlot->SetHorizontalAlignment(HAlign_Left);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    NewButton->OnSelected.RemoveDynamic(this, &UMissionNavDlg::OnNavModeButtonSelected);
    NewButton->OnSelected.AddDynamic(this, &UMissionNavDlg::OnNavModeButtonSelected);

    NewButton->OnHovered.RemoveDynamic(this, &UMissionNavDlg::OnNavModeButtonHovered);
    NewButton->OnHovered.AddDynamic(this, &UMissionNavDlg::OnNavModeButtonHovered);

    NavModeButtons.Add(NewButton);
    return NewButton;
}

void UMissionNavDlg::RefreshNavModeSelection()
{
    FString ActiveLabel;

    switch (CurrentNavMode)
    {
    case EMissionNavMode::GALAXY: ActiveLabel = TEXT("GALAXY"); break;
    case EMissionNavMode::SYSTEM: ActiveLabel = TEXT("SYSTEM"); break;
    case EMissionNavMode::SECTOR: ActiveLabel = TEXT("SECTOR"); break;
    default: break;
    }

    for (UMenuButton* Button : NavModeButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }
}

void UMissionNavDlg::SetNavMode(EMissionNavMode NewMode)
{
    CurrentNavMode = NewMode;

    if (NavSwitcher)
    {
        NavSwitcher->SetActiveWidgetIndex(static_cast<int32>(CurrentNavMode));
    }

    RefreshNavModeSelection();

    switch (CurrentNavMode)
    {
    case EMissionNavMode::GALAXY:
        if (Manager) Manager->NavModeGalaxy();
        break;

    case EMissionNavMode::SYSTEM:
        if (Manager) Manager->NavModeSystem();
        break;

    case EMissionNavMode::SECTOR:
        if (Manager) Manager->NavModeSector();
        break;

    default:
        break;
    }

    RefreshFromMission();
}

// -----------------------------------------------------------------------------
// Right Panels
// -----------------------------------------------------------------------------

void UMissionNavDlg::BuildRightPanels()
{
    if (!WidgetTree || !RightPanelColumn)
    {
        return;
    }

    // -------------------------------------------------------------
    // Filter radio-button panel
    // -------------------------------------------------------------

    FilterPanelHost =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavFilterPanelHost"));

    if (UVerticalBoxSlot* FilterPanelSlot = RightPanelColumn->AddChildToVerticalBox(FilterPanelHost))
    {
        FilterPanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
        FilterPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    FilterButtonGrid =
        WidgetTree->ConstructWidget<UUniformGridPanel>(
            UUniformGridPanel::StaticClass(),
            TEXT("MissionNavFilterButtonGrid"));

    if (UVerticalBoxSlot* GridSlot = FilterPanelHost->AddChildToVerticalBox(FilterButtonGrid))
    {
        GridSlot->SetPadding(FMargin(0.f));
        GridSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    BuildFilterButtons();

    // -------------------------------------------------------------
    // Object list panel
    // -------------------------------------------------------------

    ObjectListBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavObjectListBorder"));

    if (UVerticalBoxSlot* ObjectListBorderSlot = RightPanelColumn->AddChildToVerticalBox(ObjectListBorder))
    {
        ObjectListBorderSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 8.f));
        ObjectListBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ObjectListPanel =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavObjectListPanel"));
    ObjectListBorder->SetContent(ObjectListPanel);

    ObjectListTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavObjectListTitleText"));
    ObjectListTitleText->SetText(FText::FromString(GetFilterModeLabel(CurrentFilterMode)));

    if (UVerticalBoxSlot* ObjectTitleSlot = ObjectListPanel->AddChildToVerticalBox(ObjectListTitleText))
    {
        ObjectTitleSlot->SetPadding(FMargin(8.f, 6.f, 8.f, 6.f));
        ObjectTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ObjectListHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavObjectListHost"));
    ObjectListHost->SetWidthOverride(300.f);
    ObjectListHost->SetHeightOverride(220.f);

    if (UVerticalBoxSlot* ObjectListHostSlot = ObjectListPanel->AddChildToVerticalBox(ObjectListHost))
    {
        ObjectListHostSlot->SetPadding(FMargin(0.f));
        ObjectListHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ObjectListView =
        WidgetTree->ConstructWidget<UListView>(
            UListView::StaticClass(),
            TEXT("MissionNavObjectListView"));
    ObjectListHost->SetContent(ObjectListView);

    // -------------------------------------------------------------
    // Detail panel
    // -------------------------------------------------------------

    DetailBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavDetailBorder"));

    if (UVerticalBoxSlot* DetailBorderSlot = RightPanelColumn->AddChildToVerticalBox(DetailBorder))
    {
        DetailBorderSlot->SetPadding(FMargin(0.f));
        DetailBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailPanel =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavDetailPanel"));
    DetailBorder->SetContent(DetailPanel);

    DetailTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavDetailTitleText"));
    DetailTitleText->SetText(FText::FromString(TEXT("DETAILS")));

    if (UVerticalBoxSlot* DetailTitleSlot = DetailPanel->AddChildToVerticalBox(DetailTitleText))
    {
        DetailTitleSlot->SetPadding(FMargin(8.f, 6.f, 8.f, 6.f));
        DetailTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavDetailHost"));
    DetailHost->SetWidthOverride(300.f);
    DetailHost->SetHeightOverride(120.f);

    if (UVerticalBoxSlot* DetailHostSlot = DetailPanel->AddChildToVerticalBox(DetailHost))
    {
        DetailHostSlot->SetPadding(FMargin(0.f));
        DetailHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailBodyText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavDetailBodyText"));
    DetailBodyText->SetText(FText::FromString(TEXT("NO OBJECT SELECTED")));
    DetailHost->SetContent(DetailBodyText);

    RefreshFilterSelection();
    RefreshObjectListPanel();
    RefreshDetailPanel();
}

void UMissionNavDlg::BuildFilterButtons()
{
    if (!FilterButtonGrid || !MenuButtonClass)
    {
        return;
    }

    FilterButtonGrid->ClearChildren();
    FilterButtons.Empty();

    CreateFilterButton(TEXT("SYSTEM"), 0, 0);
    CreateFilterButton(TEXT("PLANET"), 0, 1);
    CreateFilterButton(TEXT("SECTOR"), 1, 0);
    CreateFilterButton(TEXT("STATION"), 1, 1);
    CreateFilterButton(TEXT("STARSHIP"), 2, 0);
    CreateFilterButton(TEXT("FIGHTER"), 2, 1);
}

UMenuButton* UMissionNavDlg::CreateFilterButton(const FString& Label, int32 Row, int32 Column)
{
    if (!FilterButtonGrid || !MenuButtonClass)
    {
        return nullptr;
    }

    UMenuButton* NewButton = CreateWidget<UMenuButton>(this, MenuButtonClass);
    if (!NewButton)
    {
        return nullptr;
    }

    NewButton->MenuOption = Label;
    NewButton->WidthOverride = 118.f;
    NewButton->HeightOverride = 30.f;
    NewButton->LabelFontSize = 13;

    if (UTextBlock* LabelText = Cast<UTextBlock>(NewButton->GetWidgetFromName(TEXT("Label"))))
    {
        LabelText->SetText(FText::FromString(Label));
    }

    if (UUniformGridSlot* GridSlot = FilterButtonGrid->AddChildToUniformGrid(NewButton, Row, Column))
    {
        GridSlot->SetHorizontalAlignment(HAlign_Fill);
        GridSlot->SetVerticalAlignment(VAlign_Fill);
    }

    NewButton->OnSelected.RemoveDynamic(this, &UMissionNavDlg::OnFilterButtonSelected);
    NewButton->OnSelected.AddDynamic(this, &UMissionNavDlg::OnFilterButtonSelected);

    NewButton->OnHovered.RemoveDynamic(this, &UMissionNavDlg::OnFilterButtonHovered);
    NewButton->OnHovered.AddDynamic(this, &UMissionNavDlg::OnFilterButtonHovered);

    FilterButtons.Add(NewButton);
    return NewButton;
}

// -----------------------------------------------------------------------------
// Filter State
// -----------------------------------------------------------------------------

FString UMissionNavDlg::GetFilterModeLabel(EMissionNavFilterMode Mode) const
{
    switch (Mode)
    {
    case EMissionNavFilterMode::SYSTEM:   return TEXT("SYSTEM");
    case EMissionNavFilterMode::PLANET:   return TEXT("PLANET");
    case EMissionNavFilterMode::SECTOR:   return TEXT("SECTOR");
    case EMissionNavFilterMode::STATION:  return TEXT("STATION");
    case EMissionNavFilterMode::STARSHIP: return TEXT("STARSHIP");
    case EMissionNavFilterMode::FIGHTER:  return TEXT("FIGHTER");
    default:                              return TEXT("UNKNOWN");
    }
}

void UMissionNavDlg::RefreshFilterSelection()
{
    const FString ActiveLabel = GetFilterModeLabel(CurrentFilterMode);

    for (UMenuButton* Button : FilterButtons)
    {
        if (Button)
        {
            Button->SetSelected(Button->MenuOption == ActiveLabel);
        }
    }

    if (ObjectListTitleText)
    {
        ObjectListTitleText->SetText(FText::FromString(ActiveLabel));
    }
}

void UMissionNavDlg::SetFilterMode(EMissionNavFilterMode NewMode)
{
    CurrentFilterMode = NewMode;
    RefreshFilterSelection();
    RefreshObjectListPanel();
    RefreshDetailPanel();
}

// -----------------------------------------------------------------------------
// Right Panel Refresh
// -----------------------------------------------------------------------------

void UMissionNavDlg::RefreshObjectListPanel()
{
    if (ObjectListTitleText)
    {
        ObjectListTitleText->SetText(FText::FromString(GetFilterModeLabel(CurrentFilterMode)));
    }

    if (ObjectListView)
    {
        ObjectListView->ClearListItems();
    }
}

void UMissionNavDlg::RefreshDetailPanel()
{
    if (DetailTitleText)
    {
        DetailTitleText->SetText(FText::FromString(TEXT("DETAILS")));
    }

    if (DetailBodyText)
    {
        DetailBodyText->SetText(FText::FromString(TEXT("NO OBJECT SELECTED")));
    }
}

// -----------------------------------------------------------------------------
// Event Handlers
// -----------------------------------------------------------------------------

void UMissionNavDlg::OnNavModeButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("GALAXY"))
    {
        SetNavMode(EMissionNavMode::GALAXY);
    }
    else if (Option == TEXT("SYSTEM"))
    {
        SetNavMode(EMissionNavMode::SYSTEM);
    }
    else if (Option == TEXT("SECTOR"))
    {
        SetNavMode(EMissionNavMode::SECTOR);
    }
}

void UMissionNavDlg::OnNavModeButtonHovered(UMenuButton* HoveredButton)
{
}

void UMissionNavDlg::OnFilterButtonSelected(UMenuButton* SelectedButton)
{
    if (!SelectedButton)
    {
        return;
    }

    const FString& Option = SelectedButton->MenuOption;

    if (Option == TEXT("SYSTEM"))
    {
        SetFilterMode(EMissionNavFilterMode::SYSTEM);
    }
    else if (Option == TEXT("PLANET"))
    {
        SetFilterMode(EMissionNavFilterMode::PLANET);
    }
    else if (Option == TEXT("SECTOR"))
    {
        SetFilterMode(EMissionNavFilterMode::SECTOR);
    }
    else if (Option == TEXT("STATION"))
    {
        SetFilterMode(EMissionNavFilterMode::STATION);
    }
    else if (Option == TEXT("STARSHIP"))
    {
        SetFilterMode(EMissionNavFilterMode::STARSHIP);
    }
    else if (Option == TEXT("FIGHTER"))
    {
        SetFilterMode(EMissionNavFilterMode::FIGHTER);
    }
}

void UMissionNavDlg::OnFilterButtonHovered(UMenuButton* HoveredButton)
{
}

void UMissionNavDlg::OnZoomInClicked()
{
    if (Manager)
    {
        Manager->NavZoomIn();
    }
}

void UMissionNavDlg::OnZoomOutClicked()
{
    if (Manager)
    {
        Manager->NavZoomOut();
    }
}