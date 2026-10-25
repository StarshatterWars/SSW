/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (c) 2025-2026.

    SUBSYSTEM:    Stars.exe
    FILE:         CampaignSelectDlg.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    Code-built campaign selection screen with a custom styled
    campaign dropdown list.
*/


#include "CampaignSelectDlg.h"
#include "AudioRouting.h"

#include "MissionUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Campaign.h"
#include "CampaignSave.h"
#include "CmpLoadDlg.h"
#include "Game.h"
#include "Keyboard.h"
#include "MenuScreen.h"
#include "Mouse.h"
#include "SSWGameInstance.h"
#include "Starshatter.h"
#include "StarshatterGameDataSubsystem.h"
#include "StarshatterPlayerSubsystem.h"
#include "StarshatterUIStyleSubsystem.h"
#include "SSWRuntimeSubsystem.h"
#include "TimerSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
    static int32 GCampaignSelectUniqueNameCounter = 0;

    static FName MakeUniqueWidgetName(const TCHAR* BaseName)
    {
        return FName(*FString::Printf(TEXT("%s_%d"), BaseName, ++GCampaignSelectUniqueNameCounter));
    }
}

UCampaignSelectDlg::UCampaignSelectDlg(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UCampaignSelectDlg::NativePreConstruct()
{
    Super::NativePreConstruct();
}

void UCampaignSelectDlg::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    stars = Starshatter::GetInstance();
    select_msg = "CmpSelectDlg Select Msg";
}

void UCampaignSelectDlg::NativeConstruct()
{
    Super::NativeConstruct();

    BuildWidgetTreeIfNeeded();
    HookupEvents();
    RegisterControls();
    RefreshUIFromSubsystem();
}

void UCampaignSelectDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
}

void UCampaignSelectDlg::SetMenuManager(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCampaignSelectDlg::InitializeDlg(UMenuScreen* InManager)
{
    manager = InManager;
}

void UCampaignSelectDlg::BuildWidgetTreeIfNeeded()
{
    if (!WidgetTree) return;

    // The Blueprint supplies Campaign -> Background -> RootCanvas.
    // Keep that authored hierarchy and build the interactive content only once.
    RootCanvas = Cast<UCanvasPanel>(WidgetTree->FindWidget(TEXT("RootCanvas")));
    if (!RootCanvas)
        RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
    if (!RootCanvas)
    {
        RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
        WidgetTree->RootWidget = RootCanvas;
    }
    if (WidgetTree->FindWidget(TEXT("CampaignSelect_MainBorder"))) return;

    // Tactical UI palette: background, raised panels, muted labels and icy cyan.
    const FLinearColor Back(0.012f, 0.025f, 0.045f, 1.f);
    const FLinearColor Panel(0.025f, 0.055f, 0.083f, 0.97f);
    const FLinearColor Raised(0.052f, 0.098f, 0.138f, 1.f);
    const FLinearColor Accent(0.16f, 0.77f, 0.91f, 1.f);
    const FLinearColor Primary(0.92f, 0.96f, 0.98f, 1.f);
    const FLinearColor Muted(0.56f, 0.68f, 0.75f, 1.f);

    UBorder* Backdrop = CreatePanelBorder(TEXT("CampaignSelect_Backdrop"), Back);
    if (UCanvasPanelSlot* WidgetSlot = RootCanvas->AddChildToCanvas(Backdrop))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(0.f));
        WidgetSlot->SetZOrder(0);
    }

    // Small top-level margins allow the whole layout to scale with the viewport.
    MainBorder = CreatePanelBorder(TEXT("CampaignSelect_MainBorder"), Panel);
    if (UCanvasPanelSlot* WidgetSlot = RootCanvas->AddChildToCanvas(MainBorder))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(32.f, 24.f, 32.f, 24.f));
        WidgetSlot->SetZOrder(1);
    }
    UCanvasPanel* MainCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("CampaignSelect_MainCanvas"));
    MainBorder->SetContent(MainCanvas);

    // Dark-grey header band, matching the Single Missions title treatment.
    // Add before header text so all title and pilot labels remain visible above it.
    UBorder* TitleBand = CreatePanelBorder(TEXT("CampaignSelect_TitleBand"),
        FLinearColor(0.105f, 0.113f, 0.125f, 1.0f));
    if (UCanvasPanelSlot* BandSlot = MainCanvas->AddChildToCanvas(TitleBand))
    {
        BandSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
        BandSlot->SetOffsets(FMargin(20.f, 35.f, 20.f, 47.f));
        BandSlot->SetZOrder(-1);
    }

    // HEADER: overline / page title / active pilot identity.
    UTextBlock* Overline = CreateText(TEXT("CampaignSelect_Overline"),
        TEXT("STARSHATTER WARS   /   CAMPAIGN COMMAND"), 12, Accent);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(Overline))
    {
        WidgetSlot->SetPosition(FVector2D(30.f, 18.f));
        WidgetSlot->SetAutoSize(true);
    }
    TitleText = CreateText(TEXT("CampaignSelect_TitleText"),
        TEXT("SELECT CAMPAIGN"), 30, Primary);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(TitleText))
    {
        WidgetSlot->SetPosition(FVector2D(30.f, 37.f));
        WidgetSlot->SetAutoSize(true);
    }
    UTextBlock* PilotCaption = CreateText(TEXT("CampaignSelect_PilotCaption"),
        TEXT("ACTIVE PILOT"), 11, Muted, ETextJustify::Right);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(PilotCaption))
    {
        WidgetSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
        WidgetSlot->SetAlignment(FVector2D(1.f, 0.f));
        WidgetSlot->SetPosition(FVector2D(-30.f, 24.f));
        WidgetSlot->SetAutoSize(true);
    }
    PlayerNameText = CreateText(TEXT("CampaignSelect_PlayerNameText"),
        TEXT("PLAYER"), 16, Primary, ETextJustify::Right);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(PlayerNameText))
    {
        WidgetSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
        WidgetSlot->SetAlignment(FVector2D(1.f, 0.f));
        WidgetSlot->SetPosition(FVector2D(-30.f, 45.f));
        WidgetSlot->SetAutoSize(true);
    }
    UBorder* Divider = CreatePanelBorder(TEXT("CampaignSelect_HeaderDivider"), Accent);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(Divider))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
        WidgetSlot->SetOffsets(FMargin(30.f, 90.f, 30.f, 2.f));
    }

    BuildCampaignDropdown(MainCanvas);

    // The selected campaign name takes the rest of the selection-row width.
    CampaignNameText = CreateText(TEXT("CampaignSelect_CampaignNameText"),
        TEXT("CAMPAIGN"), 23, Primary, ETextJustify::Left, false);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(CampaignNameText))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
        WidgetSlot->SetOffsets(FMargin(405.f, 131.f, 28.f, 38.f));
    }

    // BODY: responsive artwork column and independently scrolling briefing.
    UHorizontalBox* ContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignSelect_ContentRow"));
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(ContentRow))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(30.f, 202.f, 30.f, 91.f));
    }
    UBorder* ArtPanel = CreatePanelBorder(TEXT("CampaignSelect_ArtPanel"), Raised);
    if (UHorizontalBoxSlot* WidgetSlot = ContentRow->AddChildToHorizontalBox(ArtPanel))
    {
        WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        WidgetSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
        WidgetSlot->SetVerticalAlignment(VAlign_Fill);
    }
    UCanvasPanel* ArtCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
        UCanvasPanel::StaticClass(), TEXT("CampaignSelect_ArtCanvas"));
    ArtPanel->SetContent(ArtCanvas);
    CampaignImage = WidgetTree->ConstructWidget<UImage>(
        UImage::StaticClass(), TEXT("CampaignSelect_CampaignImage"));
    CampaignImage->SetBrushSize(FVector2D(1024.f, 1024.f));
    CampaignImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (UCanvasPanelSlot* WidgetSlot = ArtCanvas->AddChildToCanvas(CampaignImage))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(5.f));
    }
    UBorder* ArtCaptionBg = CreatePanelBorder(TEXT("CampaignSelect_ArtCaptionBG"), FLinearColor(0.012f,0.025f,0.04f,0.95f));
    if (UCanvasPanelSlot* WidgetSlot = ArtCanvas->AddChildToCanvas(ArtCaptionBg))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(5.f, -45.f, 5.f, 39.f));
        WidgetSlot->SetZOrder(2);
    }
    ArtCaptionBg->SetContent(CreateText(TEXT("CampaignSelect_ArtCaption"),
        TEXT(" CAMPAIGN INTELLIGENCE  /  ARCHIVE PREVIEW"), 12, Accent));

    UBorder* BriefingPanel = CreatePanelBorder(TEXT("CampaignSelect_BriefingPanel"), Raised);
    if (UHorizontalBoxSlot* WidgetSlot = ContentRow->AddChildToHorizontalBox(BriefingPanel))
    {
        WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        WidgetSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
        WidgetSlot->SetVerticalAlignment(VAlign_Fill);
    }
    UScrollBox* BriefingScroll = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(), TEXT("CampaignSelect_BriefingScroll"));
    BriefingPanel->SetContent(BriefingScroll);
    UVerticalBox* BriefingVBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(), TEXT("CampaignSelect_BriefingContent"));
    BriefingScroll->AddChild(BriefingVBox);

    auto AddSection = [&](const TCHAR* Heading, TObjectPtr<UTextBlock>& Output,
                          const TCHAR* TextName, float MinHeight)
    {
        UTextBlock* Caption = CreateText(MakeUniqueWidgetName(TEXT("CampaignSelect_SectionCaption")),
            Heading, 13, Accent);
        if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(Caption))
            WidgetSlot->SetPadding(FMargin(20.f, 20.f, 18.f, 9.f));
        Output = CreateText(TextName, TEXT(""), 16, Primary, ETextJustify::Left, true);
        Output->SetWrapTextAt(520.f);
        USizeBox* SectionSize = WidgetTree->ConstructWidget<USizeBox>(
            USizeBox::StaticClass(), MakeUniqueWidgetName(TEXT("CampaignSelect_SectionSize")));
        SectionSize->SetMinDesiredHeight(MinHeight);
        SectionSize->SetContent(Output);
        if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(SectionSize))
            WidgetSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 6.f));
    };
    AddSection(TEXT("01  /  OVERVIEW"), DescriptionText, TEXT("CampaignSelect_DescriptionText"), 72.f);
    AddSection(TEXT("02  /  TACTICAL SITUATION"), SituationText, TEXT("CampaignSelect_SituationText"), 108.f);
    UTextBlock* OrdersCaption = CreateText(TEXT("CampaignSelect_OrdersCaption"),
        TEXT("03  /  MISSION DIRECTIVES"), 13, Accent);
    if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(OrdersCaption))
        WidgetSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 12.f));
    auto AddOrder = [&](TObjectPtr<UTextBlock>& Output, const TCHAR* Name)
    {
        Output = CreateText(Name, TEXT(""), 15, Primary, ETextJustify::Left, true);
        Output->SetWrapTextAt(520.f);
        if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(Output))
            WidgetSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 8.f));
    };
    AddOrder(Orders1Text, TEXT("CampaignSelect_Orders1Text"));
    AddOrder(Orders2Text, TEXT("CampaignSelect_Orders2Text"));
    AddOrder(Orders3Text, TEXT("CampaignSelect_Orders3Text"));
    AddOrder(Orders4Text, TEXT("CampaignSelect_Orders4Text"));
    UTextBlock* MetaCaption = CreateText(TEXT("CampaignSelect_MetaCaption"),
        TEXT("04  /  DEPLOYMENT PARAMETERS"), 13, Accent);
    if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(MetaCaption))
        WidgetSlot->SetPadding(FMargin(20.f, 20.f, 20.f, 10.f));
    UHorizontalBox* MetaRow = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignSelect_MetaRow"));
    if (UVerticalBoxSlot* WidgetSlot = BriefingVBox->AddChildToVerticalBox(MetaRow))
        WidgetSlot->SetPadding(FMargin(20.f, 0.f, 20.f, 24.f));
    auto AddMeta = [&](const TCHAR* Label, TObjectPtr<UTextBlock>& Output, const TCHAR* Name)
    {
        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
            UVerticalBox::StaticClass(), MakeUniqueWidgetName(TEXT("CampaignSelect_MetaColumn")));
        if (UHorizontalBoxSlot* WidgetSlot = MetaRow->AddChildToHorizontalBox(Column))
            WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        UTextBlock* Caption = CreateText(MakeUniqueWidgetName(TEXT("CampaignSelect_MetaLabel")),
            Label, 11, Muted);
        Column->AddChildToVerticalBox(Caption);
        Output = CreateText(Name, TEXT(""), 14, Primary, ETextJustify::Left, true);
        if (UVerticalBoxSlot* WidgetSlot = Column->AddChildToVerticalBox(Output))
            WidgetSlot->SetPadding(FMargin(0.f, 7.f, 8.f, 0.f));
    };
    AddMeta(TEXT("STAR SYSTEM  /  REGION"), LocationSystemText, TEXT("CampaignSelect_LocationSystemText"));
    AddMeta(TEXT("OPERATIONS START"), CampaignStartTimeText, TEXT("CampaignSelect_CampaignStartTimeText"));

    // ACTION BAR: status hint on the left, all original actions to the right.
    UBorder* FooterLine = CreatePanelBorder(TEXT("CampaignSelect_FooterLine"), Accent);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(FooterLine))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
        WidgetSlot->SetOffsets(FMargin(30.f, -77.f, 30.f, 2.f));
    }
    UTextBlock* FooterLabel = CreateText(TEXT("CampaignSelect_FooterLabel"),
        TEXT("OPERATIONS READY   /   SELECT A CAMPAIGN TO DEPLOY"), 12, Muted);
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(FooterLabel))
    {
        WidgetSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
        WidgetSlot->SetPosition(FVector2D(30.f, -45.f));
        WidgetSlot->SetAutoSize(true);
    }
    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>(
        UHorizontalBox::StaticClass(), TEXT("CampaignSelect_ButtonRow"));
    if (UCanvasPanelSlot* WidgetSlot = MainCanvas->AddChildToCanvas(Actions))
    {
        WidgetSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
        WidgetSlot->SetAlignment(FVector2D(1.f, 1.f));
        WidgetSlot->SetPosition(FVector2D(-30.f, -18.f));
        WidgetSlot->SetAutoSize(true);
    }
    auto AddAction = [&](UWidget* ButtonWidget)
    {
        if (UHorizontalBoxSlot* WidgetSlot = Actions->AddChildToHorizontalBox(ButtonWidget))
            WidgetSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
    };
    AddAction(CreateMenuButton(TEXT("CampaignSelect_CancelButton"), CancelButton, CancelButtonText, TEXT("BACK")));
    AddAction(CreateMenuButton(TEXT("CampaignSelect_RestartButton"), RestartButton, RestartButtonText, TEXT("RESTART")));
    AddAction(CreateMenuButton(TEXT("CampaignSelect_PlayButton"), PlayButton, PlayButtonText, TEXT("START")));
    ApplyButtonStyle(PlayButton);
    ApplyButtonStyle(RestartButton);
    ApplyButtonStyle(CancelButton);
}

void UCampaignSelectDlg::BuildCampaignDropdown(UCanvasPanel* MainCanvas)
{
    UBorder* CampaignSelectHeader = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownHeader"));
    CampaignSelectHeader->SetBrushColor(FLinearColor(0.052f, 0.098f, 0.138f, 1.f));

    UTextBlock* CampaignSelectHeaderText = CreateText(
        TEXT("CampaignSelect_DropdownHeaderText"),
        TEXT("AVAILABLE CAMPAIGNS"),
        12,
        FLinearColor(0.16f, 0.77f, 0.91f, 1.f),
        ETextJustify::Left,
        false);

    CampaignSelectHeader->SetContent(CampaignSelectHeaderText);

    if (UCanvasPanelSlot* DropdownHeaderCanvasSlot = MainCanvas->AddChildToCanvas(CampaignSelectHeader))
    {
        DropdownHeaderCanvasSlot->SetPosition(FVector2D(30.f, 112.f));
        DropdownHeaderCanvasSlot->SetSize(FVector2D(340.f, 22.f));
        DropdownHeaderCanvasSlot->SetZOrder(3);
    }

    UBorder* CampaignDropdownFrame = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownFrame"));
    CampaignDropdownFrame->SetBrushColor(FLinearColor(0.16f, 0.77f, 0.91f, 1.f));

    if (UCanvasPanelSlot* FrameCanvasSlot = MainCanvas->AddChildToCanvas(CampaignDropdownFrame))
    {
        FrameCanvasSlot->SetPosition(FVector2D(30.f, 139.f));
        FrameCanvasSlot->SetSize(FVector2D(340.f, 42.f));
        FrameCanvasSlot->SetZOrder(3);
    }

    UBorder* CampaignDropdownInner = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownInner"));
    // The currently chosen campaign remains black even while the list is closed.
    CampaignDropdownInner->SetBrushColor(FLinearColor::Black);
    CampaignDropdownFrame->SetContent(CampaignDropdownInner);

    CampaignDropdownButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(),
        TEXT("CampaignSelect_DropdownButton"));
    CampaignDropdownInner->SetContent(CampaignDropdownButton);

    // Apply opaque black in every interactive state. A default UButton brush
    // otherwise draws its own normal/hover highlight over the selected value.
    FButtonStyle SelectedCampaignButtonStyle = CampaignDropdownButton->GetStyle();
    SelectedCampaignButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::Black);
    SelectedCampaignButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor::Black);
    SelectedCampaignButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::Black);
    SelectedCampaignButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor::Black);
    CampaignDropdownButton->SetStyle(SelectedCampaignButtonStyle);

    CampaignDropdownButtonText = CreateText(
        TEXT("CampaignSelect_DropdownButtonText"),
        TEXT("SELECT CAMPAIGN"),
        16,
        FLinearColor(0.92f, 0.96f, 0.98f, 1.f),
        ETextJustify::Left,
        false);

    if (UButtonSlot* DropdownButtonSlot = Cast<UButtonSlot>(CampaignDropdownButton->AddChild(CampaignDropdownButtonText)))
    {
        DropdownButtonSlot->SetPadding(FMargin(8.f, 4.f, 8.f, 4.f));
        DropdownButtonSlot->SetHorizontalAlignment(HAlign_Fill);
        DropdownButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    CampaignDropdownPopupBorder = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        TEXT("CampaignSelect_DropdownPopupBorder"));
    CampaignDropdownPopupBorder->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.083f, 1.f));
    CampaignDropdownPopupBorder->SetVisibility(ESlateVisibility::Collapsed);

    if (UCanvasPanelSlot* PopupCanvasSlot = MainCanvas->AddChildToCanvas(CampaignDropdownPopupBorder))
    {
        PopupCanvasSlot->SetPosition(FVector2D(30.f, 181.f));
        PopupCanvasSlot->SetSize(FVector2D(340.f, 260.f));
        PopupCanvasSlot->SetZOrder(20);
    }

    CampaignDropdownScrollBox = WidgetTree->ConstructWidget<UScrollBox>(
        UScrollBox::StaticClass(),
        TEXT("CampaignSelect_DropdownScrollBox"));
    CampaignDropdownPopupBorder->SetContent(CampaignDropdownScrollBox);

    CampaignDropdownListBox = WidgetTree->ConstructWidget<UVerticalBox>(
        UVerticalBox::StaticClass(),
        TEXT("CampaignSelect_DropdownListBox"));
    CampaignDropdownScrollBox->AddChild(CampaignDropdownListBox);
}

void UCampaignSelectDlg::RebuildCampaignDropdownOptions()
{
    if (!CampaignDropdownListBox || !WidgetTree)
    {
        return;
    }

    CampaignDropdownListBox->ClearChildren();
    CampaignOptionButtons.Reset();
    CampaignOptionButtonTexts.Reset();

    const FLinearColor PopupBG = FLinearColor(0.025f, 0.055f, 0.083f, 1.0f);
    const FLinearColor RowBG = FLinearColor(0.052f, 0.098f, 0.138f, 1.0f);
    const FLinearColor RowSelectedBG = FLinearColor::Black;
    const FLinearColor RowTextColor = FLinearColor(0.92f, 0.93f, 0.95f, 1.0f);

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetBrushColor(PopupBG);
    }

    for (int32 i = 0; i < CampaignDisplayNamesByOptionIndex.Num(); ++i)
    {
        const bool bSelected = (i == Selected);

        UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(),
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionRowBorder")));
        RowBorder->SetBrushColor(bSelected ? RowSelectedBG : RowBG);

        UButton* RowButton = WidgetTree->ConstructWidget<UButton>(
            UButton::StaticClass(),
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionButton")));

        // Keep button visually transparent so the border drives the row color.
        FButtonStyle TransparentButtonStyle = RowButton->GetStyle();
        TransparentButtonStyle.Normal.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Hovered.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Pressed.TintColor = FSlateColor(FLinearColor::Transparent);
        TransparentButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor::Transparent);
        RowButton->SetStyle(TransparentButtonStyle);

        UTextBlock* RowText = CreateText(
            MakeUniqueWidgetName(TEXT("CampaignSelect_OptionText")),
            CampaignDisplayNamesByOptionIndex[i],
            14,
            RowTextColor,
            ETextJustify::Left,
            false);

        if (UButtonSlot* RowButtonSlot = Cast<UButtonSlot>(RowButton->AddChild(RowText)))
        {
            RowButtonSlot->SetPadding(FMargin(12.f, 3.f, 8.f, 3.f));
            RowButtonSlot->SetHorizontalAlignment(HAlign_Fill);
            RowButtonSlot->SetVerticalAlignment(VAlign_Center);
        }

        RowButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCampaignOptionClicked);
        RowButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCampaignOptionClicked);

        RowBorder->SetContent(RowButton);

        if (UVerticalBoxSlot* RowVBoxSlot = CampaignDropdownListBox->AddChildToVerticalBox(RowBorder))
        {
            RowVBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            RowVBoxSlot->SetHorizontalAlignment(HAlign_Fill);
            RowVBoxSlot->SetVerticalAlignment(VAlign_Top);
        }

        CampaignOptionButtons.Add(RowButton);
        CampaignOptionButtonTexts.Add(RowText);
    }
}

void UCampaignSelectDlg::UpdateCampaignDropdownLabel()
{
    if (!CampaignDropdownButtonText)
    {
        return;
    }

    const FString Label =
        CampaignDisplayNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignDisplayNamesByOptionIndex[Selected]
        : TEXT("SELECT CAMPAIGN");

    CampaignDropdownButtonText->SetText(FText::FromString(Label));
}

void UCampaignSelectDlg::HideCampaignDropdown()
{
    bCampaignDropdownOpen = false;

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UCampaignSelectDlg::SelectCampaignOption(int32 NewIndex)
{
    if (!CampaignIndexByOptionIndex.IsValidIndex(NewIndex))
    {
        return;
    }

    Selected = NewIndex;
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(NewIndex)
        ? CampaignRowNamesByOptionIndex[NewIndex]
        : NAME_None;

    UpdateCampaignDropdownLabel();
    HideCampaignDropdown();
    RefreshFromSelection();
    RebuildCampaignDropdownOptions();
}

void UCampaignSelectDlg::HookupEvents()
{
    if (CampaignDropdownButton)
    {
        CampaignDropdownButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCampaignDropdownClicked);
        CampaignDropdownButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCampaignDropdownClicked);
    }

    if (PlayButton)
    {
        PlayButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);
        PlayButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonClicked);

        PlayButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);
        PlayButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonHovered);

        PlayButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);
        PlayButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnPlayButtonUnHovered);
    }

    if (RestartButton)
    {
        RestartButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);
        RestartButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonClicked);

        RestartButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);
        RestartButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonHovered);

        RestartButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);
        RestartButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnRestartButtonUnHovered);
    }

    if (CancelButton)
    {
        CancelButton->OnClicked.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);
        CancelButton->OnClicked.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonClicked);

        CancelButton->OnHovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);
        CancelButton->OnHovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonHovered);

        CancelButton->OnUnhovered.RemoveDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);
        CancelButton->OnUnhovered.AddDynamic(this, &UCampaignSelectDlg::OnCancelButtonUnHovered);
    }
}

void UCampaignSelectDlg::RegisterControls()
{
    PopulateCampaignDropdown();
}

void UCampaignSelectDlg::PopulateCampaignDropdown()
{
    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem = GI->GetSubsystem<UStarshatterGameDataSubsystem>();
    UStarshatterPlayerSubsystem* PlayerSS = GI->GetSubsystem<UStarshatterPlayerSubsystem>();

    if (!DataSubsystem || !PlayerSS)
    {
        return;
    }

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    CampaignRowNamesByOptionIndex.Reset();
    CampaignIndexByOptionIndex.Reset();
    CampaignDisplayNamesByOptionIndex.Reset();

    const TArray<FS_Campaign>& Campaigns = DataSubsystem->GetAllCampaigns();

    for (const FS_Campaign& Row : Campaigns)
    {
        if (!Row.bAvailable)
        {
            continue;
        }

        CampaignDisplayNamesByOptionIndex.Add(Row.Name);
        CampaignRowNamesByOptionIndex.Add(Row.RowName);
        CampaignIndexByOptionIndex.Add(Row.Index + 1);
    }

    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerSS->GetPlayerInfo().Name));
    }

    int32 SelectedOptionIndex = 0;
    const int32 SavedCampaignIndex1Based = PlayerSS->GetPlayerInfo().Campaign;

    if (SavedCampaignIndex1Based > 0)
    {
        const int32 Found = CampaignIndexByOptionIndex.IndexOfByKey(SavedCampaignIndex1Based);
        if (Found != INDEX_NONE)
        {
            SelectedOptionIndex = Found;
        }
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(SelectedOptionIndex))
    {
        SelectedOptionIndex = CampaignIndexByOptionIndex.Num() > 0 ? 0 : INDEX_NONE;
    }

    Selected = SelectedOptionIndex;
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    UpdateCampaignDropdownLabel();
    RebuildCampaignDropdownOptions();

    if (Selected != INDEX_NONE)
    {
        RefreshFromSelection();
    }
    else
    {
        UpdateCampaignButtons();
    }
}

void UCampaignSelectDlg::RefreshFromSelection()
{
    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
    {
        return;
    }

    UStarshatterGameDataSubsystem* DataSubsystem =
        GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    if (!DataSubsystem)
    {
        return;
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(Selected))
    {
        return;
    }

    const int32 CampaignIndex1Based = CampaignIndexByOptionIndex[Selected];
    const FS_Campaign* CampaignData = DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);
    if (!CampaignData)
    {
        return;
    }

    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[CampaignScreen] RefreshFromSelection: Failed to build runtime campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    TArray<FString> Orders = CampaignData->Orders;
    Orders.SetNum(4);

    if (CampaignNameText)
    {
        CampaignNameText->SetText(FText::FromString(CampaignData->Name));
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(FText::FromString(CampaignData->Description));
    }

    if (SituationText)
    {
        SituationText->SetText(FText::FromString(CampaignData->Situation));
    }

    if (Orders1Text)
    {
        Orders1Text->SetText(FText::FromString(Orders[0]));
    }

    if (Orders2Text)
    {
        Orders2Text->SetText(FText::FromString(Orders[1]));
    }

    if (Orders3Text)
    {
        Orders3Text->SetText(FText::FromString(Orders[2]));
    }

    if (Orders4Text)
    {
        Orders4Text->SetText(FText::FromString(Orders[3]));
    }

    if (LocationSystemText)
    {
        LocationSystemText->SetText(
            FText::FromString(CampaignData->System + TEXT("/") + CampaignData->Region));
    }

    if (CampaignStartTimeText)
    {
        CampaignStartTimeText->SetText(FText::FromString(CampaignData->Start));
    }

    if (CampaignImage)
    {
        if (UTexture2D* Texture = LoadCampaignTexture(CampaignIndex1Based))
        {
            CampaignImage->SetBrushFromTexture(Texture, true);
        }
        else
        {
            CampaignImage->SetBrush(FSlateBrush());
        }
    }

    UpdateCampaignButtons();
}

void UCampaignSelectDlg::RefreshUIFromSubsystem()
{
    PopulateCampaignDropdown();
}

void UCampaignSelectDlg::UpdateCampaignButtons()
{
    const bool bHasSave = DoesSelectedCampaignSaveExist();

    if (PlayButton)
    {
        PlayButton->SetIsEnabled(Selected != INDEX_NONE);
    }

    if (PlayButtonText)
    {
        PlayButtonText->SetText(FText::FromString(
            bHasSave ? TEXT("CONTINUE") : TEXT("START")));
    }

    if (RestartButton)
    {
        RestartButton->SetIsEnabled(bHasSave);
    }

    if (CancelButton)
    {
        CancelButton->SetIsEnabled(true);
    }
}

bool UCampaignSelectDlg::DoesSelectedCampaignSaveExist() const
{
    const USSWGameInstance* GI = Cast<USSWGameInstance>(GetGameInstance());
    if (!GI || PickedRowName.IsNone())
    {
        return false;
    }

    const FString SlotName = UCampaignSave::MakeSlotNameFromRowName(PickedRowName);
    return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UCampaignSelectDlg::OnCampaignDropdownClicked()
{
    bCampaignDropdownOpen = !bCampaignDropdownOpen;

    if (CampaignDropdownPopupBorder)
    {
        CampaignDropdownPopupBorder->SetVisibility(
            bCampaignDropdownOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UCampaignSelectDlg::OnCampaignOptionClicked()
{
    for (int32 i = 0; i < CampaignOptionButtons.Num(); ++i)
    {
        UButton* Button = CampaignOptionButtons[i];
        if (Button && (Button->IsHovered() || Button->HasKeyboardFocus()))
        {
            SelectCampaignOption(i);
            return;
        }
    }
}

void UCampaignSelectDlg::OnPlayButtonClicked()
{
    PlayUISound(this, AcceptSound);
    StartSelectedCampaignFlow(false);
}

void UCampaignSelectDlg::OnRestartButtonClicked()
{
    PlayUISound(this, AcceptSound);
    StartSelectedCampaignFlow(true);
}

void UCampaignSelectDlg::OnPlayButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnPlayButtonUnHovered()
{
}

void UCampaignSelectDlg::OnRestartButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnRestartButtonUnHovered()
{
}

void UCampaignSelectDlg::OnCancelButtonClicked()
{
    HideCampaignDropdown();

    if (manager)
    {
        manager->ShowMenuDlg();
    }
    else
    {
        HideDlg();
    }
}

void UCampaignSelectDlg::OnCancelButtonHovered()
{
    PlayUISound(this, HoverSound);
}

void UCampaignSelectDlg::OnCancelButtonUnHovered()
{
}

void UCampaignSelectDlg::PlayUISound(UObject* WorldContext, USoundBase* UISound)
{
    if (UISound)
    {
        SSWAudioRouting::Play2D(WorldContext,UISound,EStarshatterAudioChannel::Effects);
    }
}

void UCampaignSelectDlg::StartSelectedCampaignFlow(bool bRestart)
{
    PickedRowName = CampaignRowNamesByOptionIndex.IsValidIndex(Selected)
        ? CampaignRowNamesByOptionIndex[Selected]
        : NAME_None;

    if (PickedRowName.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Campaign] StartSelectedCampaignFlow: no campaign selected"));
        return;
    }

    if (!manager)
    {
        UE_LOG(LogTemp, Error, TEXT("[Campaign] StartSelectedCampaignFlow: manager is null"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[Campaign] StartSelectedCampaignFlow: world is null"));
        return;
    }

    manager->ShowCmpLoadDlg();

    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateUObject(this, &UCampaignSelectDlg::FinishSelectedCampaignFlow, bRestart));
}

void UCampaignSelectDlg::FinishSelectedCampaignFlow(bool bRestart)
{
    UGameInstance* GIBase = GetGameInstance();
    if (!GIBase)
    {
        return;
    }

    UStarshatterPlayerSubsystem* PlayerSS = GIBase->GetSubsystem<UStarshatterPlayerSubsystem>();
    UStarshatterGameDataSubsystem* DataSubsystem = GIBase->GetSubsystem<UStarshatterGameDataSubsystem>();
    USSWGameInstance* GI = Cast<USSWGameInstance>(GIBase);

    if (!PlayerSS || !DataSubsystem || !GI)
    {
        return;
    }

    if (!CampaignIndexByOptionIndex.IsValidIndex(Selected))
    {
        return;
    }

    const int32 CampaignIndex1Based = CampaignIndexByOptionIndex[Selected];
    const FS_Campaign* CampaignData = DataSubsystem->GetCampaignByIndex1Based(CampaignIndex1Based);
    if (!CampaignData)
    {
        return;
    }

    if (!Campaign::SelectFromData(*CampaignData))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[Campaign] Failed to activate campaign '%s'"),
            *CampaignData->Name);
        return;
    }

    Campaign* CampaignPtr = Campaign::GetCampaign();
    if (CampaignPtr)
    {
        CampaignPtr->Start();
    }

    DataSubsystem->CampaignIndex = CampaignIndex1Based - 1;
    DataSubsystem->SelectedCampaignRowName = PickedRowName;
    DataSubsystem->BuildCombatRosterFromDataTables();

    GI->SelectedCampaignDisplayName = CampaignData->Name;
    GI->SelectedCampaignIndex = CampaignIndex1Based;
    GI->SelectedCampaignRowName = PickedRowName;

    if (!PlayerSS->HasLoaded())
    {
        PlayerSS->LoadPlayer();
    }

    {
        FS_PlayerGameInfo& PlayerInfo = PlayerSS->GetMutablePlayerInfo();
        PlayerInfo.Campaign = CampaignIndex1Based;
        PlayerInfo.CampaignRowName = PickedRowName;
        PlayerSS->SavePlayer(true);
    }

    if (bRestart)
    {
        GI->CreateNewCampaignSave(
            GI->SelectedCampaignIndex,
            GI->SelectedCampaignRowName,
            GI->SelectedCampaignDisplayName);
    }
    else
    {
        const bool bHasSave = DoesSelectedCampaignSaveExist();

        if (bHasSave)
        {
            GI->LoadOrCreateSelectedCampaignSave();
        }
        else
        {
            GI->CreateNewCampaignSave(
                GI->SelectedCampaignIndex,
                GI->SelectedCampaignRowName,
                GI->SelectedCampaignDisplayName);
        }

        if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
        {
            Timer->SetCampaignSave(GI->CampaignSave);

            if (!bHasSave)
            {
                Timer->RestartCampaignClock(true);
            }
        }
    }

    if (bRestart)
    {
        if (UTimerSubsystem* Timer = GIBase->GetSubsystem<UTimerSubsystem>())
        {
            Timer->SetCampaignSave(GI->CampaignSave);
            Timer->RestartCampaignClock(true);
        }
    }

    Mouse::Show(false);

    if (USSWRuntimeSubsystem* RuntimeSS = GIBase->GetSubsystem<USSWRuntimeSubsystem>())
    {
        RuntimeSS->SetGameMode(EGameMode::CLOD);
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        if (manager)
        {
            manager->ShowOperationsDlg();
        }
        return;
    }

    World->GetTimerManager().ClearTimer(CampaignLoadFinishTimer);
    World->GetTimerManager().SetTimer(
        CampaignLoadFinishTimer,
        this,
        &UCampaignSelectDlg::TryFinishCampaignLoadTransition,
        0.05f,
        true);
}

void UCampaignSelectDlg::TryFinishCampaignLoadTransition()
{
    if (!manager)
    {
        return;
    }

    UCmpLoadDlg* LoadDlg = manager->GetCmpLoadDlg();
    if (!LoadDlg)
    {
        manager->ShowOperationsDlg();
        return;
    }

    if (!LoadDlg->IsDone())
    {
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CampaignLoadFinishTimer);
    }

    manager->ShowOperationsDlg();
}

UTexture2D* UCampaignSelectDlg::LoadCampaignTexture(int32 CampaignIndex1Based) const
{
    if (CampaignIndex1Based <= 0)
    {
        return nullptr;
    }

    const FString IndexStr = FString::Printf(TEXT("%02d"), CampaignIndex1Based);
    const FString AssetPath = FString::Printf(
        TEXT("/Game/UI/Campaigns/%s/main.main"),
        *IndexStr);

    return LoadObject<UTexture2D>(nullptr, *AssetPath);
}

UTextBlock* UCampaignSelectDlg::CreateText(
    const FName Name,
    const FString& InText,
    int32 FontSize,
    const FLinearColor& Color,
    ETextJustify::Type Justification,
    bool bWrap)
{
    UTextBlock* TextWidget = WidgetTree->ConstructWidget<UTextBlock>(
        UTextBlock::StaticClass(),
        Name);

    if (!TextWidget)
    {
        return nullptr;
    }

    TextWidget->SetText(FText::FromString(InText));
    TextWidget->SetColorAndOpacity(FSlateColor(Color));
    TextWidget->SetJustification(Justification);
    TextWidget->SetAutoWrapText(bWrap);

    if (FontSize >= 18)
    {
        TextWidget->SetFont(MissionUIStyle::GetHeaderFont(FontSize));
    }
    else if (FontSize <= 13)
    {
        TextWidget->SetFont(MissionUIStyle::GetInfoLabelFont(FontSize));
    }
    else if (FontSize == 14)
    {
        TextWidget->SetFont(MissionUIStyle::GetInfoValueFont(FontSize));
    }
    else
    {
        TextWidget->SetFont(MissionUIStyle::GetRowFont(FontSize));
    }

    return TextWidget;
}

UBorder* UCampaignSelectDlg::CreatePanelBorder(const FName Name, const FLinearColor& Color)
{
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(
        UBorder::StaticClass(),
        Name);

    if (!Border)
    {
        return nullptr;
    }

    Border->SetBrushColor(Color);
    return Border;
}

UWidget* UCampaignSelectDlg::CreateMenuButton(
    const FName Name,
    TObjectPtr<UButton>& OutButton,
    TObjectPtr<UTextBlock>& OutText,
    const FString& Label)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    USizeBox* SizeWrapper = WidgetTree->ConstructWidget<USizeBox>(
        USizeBox::StaticClass(),
        MakeUniqueWidgetName(TEXT("CampaignSelect_ButtonSizeWrapper")));

    SizeWrapper->SetWidthOverride(148.f);
    SizeWrapper->SetHeightOverride(46.f);

    OutButton = WidgetTree->ConstructWidget<UButton>(
        UButton::StaticClass(),
        Name);

    OutText = CreateText(
        MakeUniqueWidgetName(TEXT("CampaignSelect_ButtonText")),
        Label,
        16,
        FLinearColor::Black,
        ETextJustify::Center,
        false);

    if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(OutButton->AddChild(OutText)))
    {
        ButtonSlot->SetHorizontalAlignment(HAlign_Center);
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(4.f, 2.f));
    }

    SizeWrapper->SetContent(OutButton);

    return SizeWrapper;
}

void UCampaignSelectDlg::ApplyButtonStyle(UButton* Button) const
{
    if (!Button)
    {
        return;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UStarshatterUIStyleSubsystem* StyleSS = GI->GetSubsystem<UStarshatterUIStyleSubsystem>())
        {
            StyleSS->ApplyMenuButtonStyle(Button);
        }
    }
}

void UCampaignSelectDlg::ExecFrame(double DeltaTime)
{
    if (Keyboard::KeyDown(VK_RETURN))
    {
        if (PlayButton && PlayButton->GetIsEnabled())
        {
            OnPlayButtonClicked();
        }
    }
}

bool UCampaignSelectDlg::CanClose()
{
    AutoThreadSync a(sync);
    return !loading;
}

void UCampaignSelectDlg::ShowNewCampaigns()
{
    show_saved = false;
}

void UCampaignSelectDlg::ShowSavedCampaigns()
{
    show_saved = true;
}

void UCampaignSelectDlg::OnCampaignSelect()
{
}

void UCampaignSelectDlg::OnNew()
{
    ShowNewCampaigns();
}

void UCampaignSelectDlg::OnSaved()
{
    ShowSavedCampaigns();
}

void UCampaignSelectDlg::OnDelete()
{
}

void UCampaignSelectDlg::OnConfirmDelete()
{
}

void UCampaignSelectDlg::OnAccept()
{
    OnPlayButtonClicked();
}

void UCampaignSelectDlg::StartLoadProc()
{
}

void UCampaignSelectDlg::StopLoadProc()
{
}

uint32 UCampaignSelectDlg::LoadProc()
{
    return 0;
}

void UCampaignSelectDlg::BindFormWidgets()
{
}

FString UCampaignSelectDlg::GetLegacyFormText() const
{
    return FString();
}

void UCampaignSelectDlg::ShowDlg()
{
    SetVisibility(ESlateVisibility::Visible);
    RefreshUIFromSubsystem();
}

void UCampaignSelectDlg::HideDlg()
{
    SetVisibility(ESlateVisibility::Collapsed);
}