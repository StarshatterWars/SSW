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

    Runtime NAV layout:

      - Left panel contains:
          * NAV mode buttons
          * zoom buttons
          * GALAXY / SYSTEM / SECTOR switcher content

      - Right panel contains:
          * filter buttons
          * object list panel with title bar
          * detail panel with title bar
*/

#include "MissionNavDlg.h"

#include "MissionBriefingDlg.h"
#include "MissionPlanner.h"
#include "MenuButton.h"
#include "MissionNavObjectListObject.h"
#include "MissionNavObjectListView.h"
#include "MissionNavObjectLVElement.h"
#include "MissionUIStyle.h"

#include "Campaign.h"
#include "Mission.h"
#include "MissionInfo.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"
#include "UObject/ConstructorHelpers.h"

UMissionNavDlg::UMissionNavDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FObjectFinder<UTexture2D> PanelTexObj(
        TEXT("/Game/UI/Panel.Panel"));

    if (PanelTexObj.Succeeded())
    {
        RightPanelBackgroundTexture = PanelTexObj.Object;
    }
}

void UMissionNavDlg::SetParentDlg(UMissionBriefingDlg* InParentDlg)
{
    ParentDlg = InParentDlg;
}

Mission* UMissionNavDlg::ResolveMission() const
{
    return ParentDlg ? ParentDlg->GetMissionPtr() : nullptr;
}

void UMissionNavDlg::NativeConstruct()
{
    Super::NativeConstruct();

    ensureMsgf(RuntimeHost, TEXT("MissionNavDlg: RuntimeHost is not bound"));

    if (!ObjectListEntryWidgetClass)
    {
        UClass* RowWidgetClass = LoadClass<UUserWidget>(
            nullptr,
            TEXT("/Game/Screens/Mission/WBP_MissionNavObjectRow.WBP_MissionNavObjectRow_C"));

        if (RowWidgetClass)
        {
            ObjectListEntryWidgetClass = RowWidgetClass;
        }
        else
        {
            ObjectListEntryWidgetClass = UMissionNavObjectLVElement::StaticClass();
        }
    }

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

void UMissionNavDlg::RefreshFromMission()
{
    MissionPtr = ResolveMission();

    if (NavBodyText)
    {
        NavBodyText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        NavBodyText->SetFont(MissionUIStyle::GetHeaderFont(20));

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

void UMissionNavDlg::BuildRuntimeLayout()
{
    if (!WidgetTree || !RuntimeHost)
    {
        return;
    }

    if (RootContentRow)
    {
        return;
    }

    RuntimeHost->SetContent(nullptr);

    RootContentRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavRootContentRow"));

    RuntimeHost->SetContent(RootContentRow);

    // -----------------------------------------------------------------
    // LEFT PANEL
    // -----------------------------------------------------------------

    LeftPanelBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavLeftPanelBorder"));

    LeftPanelBorder->SetPadding(FMargin(0.f));
    LeftPanelBorder->SetBrushColor(FLinearColor(1.f, 1.f, 1.f, 0.f));

    if (UHorizontalBoxSlot* LeftPanelSlot = RootContentRow->AddChildToHorizontalBox(LeftPanelBorder))
    {
        LeftPanelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
        LeftPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        LeftPanelSlot->SetVerticalAlignment(VAlign_Fill);
        LeftPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    LeftPanelColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavLeftPanelColumn"));
    LeftPanelBorder->SetContent(LeftPanelColumn);

    TopButtonRow =
        WidgetTree->ConstructWidget<UHorizontalBox>(
            UHorizontalBox::StaticClass(),
            TEXT("MissionNavTopButtonRow"));

    if (UVerticalBoxSlot* TopRowSlot = LeftPanelColumn->AddChildToVerticalBox(TopButtonRow))
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
    ZoomOutText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ZoomOutText->SetFont(MissionUIStyle::GetHeaderFont(18));
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
    ZoomInText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ZoomInText->SetFont(MissionUIStyle::GetHeaderFont(18));
    ZoomInButton->SetContent(ZoomInText);

    MainViewHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavMainViewHost"));

    if (UVerticalBoxSlot* MainViewSlot = LeftPanelColumn->AddChildToVerticalBox(MainViewHost))
    {
        MainViewSlot->SetPadding(FMargin(0.f));
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
    NavBodyText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    NavBodyText->SetFont(MissionUIStyle::GetHeaderFont(20));
    SystemPanelHost->SetContent(NavBodyText);

    // -----------------------------------------------------------------
    // RIGHT PANEL
    // -----------------------------------------------------------------

    RightPanelBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavRightPanelBorder"));

    RightPanelBorder->SetPadding(FMargin(0.f));

    if (UHorizontalBoxSlot* RightPanelSlot = RootContentRow->AddChildToHorizontalBox(RightPanelBorder))
    {
        RightPanelSlot->SetPadding(FMargin(0.f));
        RightPanelSlot->SetHorizontalAlignment(HAlign_Fill);
        RightPanelSlot->SetVerticalAlignment(VAlign_Fill);
        RightPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    RightPanelColumn =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavRightPanelColumn"));
    RightPanelBorder->SetContent(RightPanelColumn);

    BuildRightPanels();
    ApplyPanelStyles();
}

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

void UMissionNavDlg::BuildRightPanels()
{
    if (!WidgetTree || !RightPanelColumn)
    {
        return;
    }

    RightPanelColumn->ClearChildren();

    // -------------------------------------------------------------
    // FILTER PANEL
    // -------------------------------------------------------------

    FilterPanelHost =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavFilterPanelHost"));

    if (UVerticalBoxSlot* FilterPanelSlot = RightPanelColumn->AddChildToVerticalBox(FilterPanelHost))
    {
        FilterPanelSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
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
    // OBJECT LIST PANEL
    // -------------------------------------------------------------

    ObjectListBorder =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavObjectListBorder"));

    if (UVerticalBoxSlot* ObjectListBorderSlot = RightPanelColumn->AddChildToVerticalBox(ObjectListBorder))
    {
        ObjectListBorderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
        ObjectListBorderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    ObjectListPanel =
        WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(),
            TEXT("MissionNavObjectListPanel"));
    ObjectListBorder->SetContent(ObjectListPanel);

    // ---- TITLE BAR ----

    ObjectListTitleBar =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavObjectTitleBar"));

    if (UVerticalBoxSlot* TitleSlot = ObjectListPanel->AddChildToVerticalBox(ObjectListTitleBar))
    {
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    ObjectListTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavObjectListTitleText"));

    ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
    ObjectListTitleText->SetJustification(ETextJustify::Left);
    ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));

    ObjectListTitleBar->SetContent(ObjectListTitleText);

    // ---- LIST HOST ----

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

    // ---- LIST VIEW (CRITICAL FIX) ----

    ObjectListView =
        WidgetTree->ConstructWidget<UMissionNavObjectListView>(
            UMissionNavObjectListView::StaticClass(),
            TEXT("MissionNavObjectListView"));

    if (ObjectListView)
    {
        // Already safe because constructor sets EntryWidgetClass,
        // but we reinforce it here for clarity.
        if (!ObjectListEntryWidgetClass)
        {
            ObjectListEntryWidgetClass = UMissionNavObjectLVElement::StaticClass();
        }

        ObjectListView->SetEntryWidgetClassPublic(ObjectListEntryWidgetClass);
        ObjectListView->SetSelectionMode(ESelectionMode::Single);

        ObjectListView->OnItemSelectionChanged().Clear();
        ObjectListView->OnItemSelectionChanged().AddUObject(this, &UMissionNavDlg::OnObjectSelectionChanged);

        ObjectListHost->SetContent(ObjectListView);
    }

    // -------------------------------------------------------------
    // DETAIL PANEL
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

    // ---- TITLE ----

    DetailTitleBar =
        WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            TEXT("MissionNavDetailTitleBar"));

    if (UVerticalBoxSlot* TitleSlot = DetailPanel->AddChildToVerticalBox(DetailTitleBar))
    {
        TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    DetailTitleText =
        WidgetTree->ConstructWidget<UTextBlock>(
            UTextBlock::StaticClass(),
            TEXT("MissionNavDetailTitleText"));

    DetailTitleText->SetText(FText::FromString(GetDetailPanelTitle()));
    DetailTitleText->SetJustification(ETextJustify::Left);
    DetailTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
    DetailTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));

    DetailTitleBar->SetContent(DetailTitleText);

    // ---- BODY ----

    DetailHost =
        WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(),
            TEXT("MissionNavDetailHost"));

    DetailHost->SetWidthOverride(300.f);
    DetailHost->SetHeightOverride(140.f);

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
    DetailBodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
    DetailBodyText->SetFont(MissionUIStyle::GetInfoValueFont());
    DetailBodyText->SetJustification(ETextJustify::Left);

    DetailHost->SetContent(DetailBodyText);

    // -------------------------------------------------------------
    // FINALIZE
    // -------------------------------------------------------------

    ApplyPanelStyles();
    RefreshFilterSelection();
    RebuildObjectList();
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

void UMissionNavDlg::ApplyPanelStyles()
{
    auto ApplyTexturedPanelWhite = [this](UBorder* BorderWidget)
        {
            if (!BorderWidget)
            {
                return;
            }

            if (RightPanelBackgroundTexture)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(RightPanelBackgroundTexture);
                Brush.ImageSize = FVector2D(256.f, 256.f);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                BorderWidget->SetBrush(Brush);
            }

            BorderWidget->SetBrushColor(FLinearColor::White);
        };

    auto ApplyTexturedPanelDark = [this](UBorder* BorderWidget)
        {
            if (!BorderWidget)
            {
                return;
            }

            if (RightPanelBackgroundTexture)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(RightPanelBackgroundTexture);
                Brush.ImageSize = FVector2D(256.f, 256.f);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                BorderWidget->SetBrush(Brush);
            }

            BorderWidget->SetBrushColor(MissionUIStyle::PanelBG);
        };

    if (LeftPanelBorder)
    {
        LeftPanelBorder->SetBrush(FSlateBrush());
        LeftPanelBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.f));
    }

    ApplyTexturedPanelDark(RightPanelBorder);

    ApplyTexturedPanelWhite(ObjectListBorder);
    ApplyTexturedPanelWhite(ObjectListTitleBar);
    ApplyTexturedPanelWhite(DetailBorder);
    ApplyTexturedPanelWhite(DetailTitleBar);
}

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

FString UMissionNavDlg::GetObjectPanelTitle() const
{
    return FString::Printf(TEXT("%s LIST"), *GetFilterModeLabel(CurrentFilterMode));
}

FString UMissionNavDlg::GetDetailPanelTitle() const
{
    return TEXT("DETAIL PANEL");
}

EMissionNavObjectType UMissionNavDlg::GetCurrentObjectType() const
{
    switch (CurrentFilterMode)
    {
    case EMissionNavFilterMode::SYSTEM:   return EMissionNavObjectType::System;
    case EMissionNavFilterMode::PLANET:   return EMissionNavObjectType::Planet;
    case EMissionNavFilterMode::SECTOR:   return EMissionNavObjectType::Sector;
    case EMissionNavFilterMode::STATION:  return EMissionNavObjectType::Station;
    case EMissionNavFilterMode::STARSHIP: return EMissionNavObjectType::Starship;
    case EMissionNavFilterMode::FIGHTER:  return EMissionNavObjectType::Fighter;
    default:                              return EMissionNavObjectType::None;
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
        ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
        ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }
}

void UMissionNavDlg::SetFilterMode(EMissionNavFilterMode NewMode)
{
    CurrentFilterMode = NewMode;
    RefreshFilterSelection();
    RefreshObjectListPanel();
    RefreshDetailPanel();
}

void UMissionNavDlg::RefreshObjectListPanel()
{
    if (ObjectListTitleText)
    {
        ObjectListTitleText->SetText(FText::FromString(GetObjectPanelTitle()));
        ObjectListTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        ObjectListTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }

    RebuildObjectList();
}

void UMissionNavDlg::RebuildObjectList()
{
    ObjectItems.Empty();
    SelectedObjectItem = nullptr;

    if (!ObjectListView)
    {
        return;
    }

    ObjectListView->ClearListItems();

    const EMissionNavObjectType ObjectType = GetCurrentObjectType();

    for (int32 Index = 0; Index < 8; ++Index)
    {
        UMissionNavObjectListObject* Item = NewObject<UMissionNavObjectListObject>(this);
        if (!Item)
        {
            continue;
        }

        const FString Primary = FString::Printf(TEXT("%s %02d"), *GetFilterModeLabel(CurrentFilterMode), Index + 1);
        const FString Secondary = FString::Printf(TEXT("ID %02d"), Index + 1);
        const FString Detail = FString::Printf(
            TEXT("%s\n\nINDEX: %d\nFILTER: %s\n\nDETAIL TEXT PLACEHOLDER."),
            *Primary,
            Index,
            *GetFilterModeLabel(CurrentFilterMode));

        Item->InitObjectRow(ObjectType, Index, Primary, Secondary, Detail);

        ObjectItems.Add(Item);
        ObjectListView->AddItem(Item);
    }

    if (ObjectItems.Num() > 0)
    {
        ObjectListView->SetSelectedItem(ObjectItems[0]);
    }
}

void UMissionNavDlg::RefreshDetailPanel()
{
    if (DetailTitleText)
    {
        DetailTitleText->SetText(FText::FromString(GetDetailPanelTitle()));
        DetailTitleText->SetColorAndOpacity(MissionUIStyle::HeaderText);
        DetailTitleText->SetFont(MissionUIStyle::GetHeaderFont(16));
    }

    if (DetailBodyText)
    {
        DetailBodyText->SetColorAndOpacity(MissionUIStyle::InfoValueText);
        DetailBodyText->SetFont(MissionUIStyle::GetInfoValueFont());

        if (SelectedObjectItem)
        {
            DetailBodyText->SetText(FText::FromString(SelectedObjectItem->GetDetailText()));
        }
        else
        {
            DetailBodyText->SetText(FText::FromString(TEXT("NO OBJECT SELECTED")));
        }
    }
}

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

void UMissionNavDlg::OnObjectSelectionChanged(UObject* SelectedItem)
{
    SelectedObjectItem = Cast<UMissionNavObjectListObject>(SelectedItem);
    RefreshDetailPanel();
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