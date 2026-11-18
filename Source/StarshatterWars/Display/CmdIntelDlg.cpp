#include "RecordedCutsceneDlg.h"
#include "StarshatterGameDataSubsystem.h"
#include "CampaignSave.h"
// +----------------------------------------------------------------------+
// | UCmdIntelDlg                                                         |
// +----------------------------------------------------------------------+

#include "CmdIntelDlg.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Components/WidgetSwitcher.h"
#include "FlightPanelStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"

#include "IntelListObject.h"
#include "IntelLVElement.h"
#include "MissionUIStyle.h"
#include "ShipSystemIcon.h"
#include "Components/NativeWidgetHost.h"
#include "CmpnScreen.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "Game.h"
#include "Starshatter.h"
#include "PlayerCharacter.h"
#include "FormattingUtils.h"
#include "Misc/Paths.h"
#include "Mouse.h"
#include "SSWGameInstance.h"

UCmdIntelDlg::UCmdIntelDlg(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// +----------------------------------------------------------------------+
// | SetManager                                                           |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::SetManager(UCmpnScreen* InManager)
{
	Manager = InManager;
}

// +----------------------------------------------------------------------+
// | SetParentCmdDlg                                                      |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
	ParentCmdDlg = InParentCmdDlg;
}

// +----------------------------------------------------------------------+
// | NativeConstruct                                                      |
// +----------------------------------------------------------------------+

TSharedRef<SWidget> UCmdIntelDlg::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    BindFormWidgets();
    BuildTwoPanelLayout();
    return Super::RebuildWidget();
}

void UCmdIntelDlg::NativeConstruct()
{
	Super::NativeConstruct();

	Stars = Starshatter::GetInstance();
	CampaignPtr = Campaign::GetCampaign();
	UpdateTime = CampaignPtr ? CampaignPtr->GetUpdateTime() : 0.0;

	BindFormWidgets();

	if (AudioButton)
	{
		AudioButton->OnClicked.RemoveDynamic(this, &UCmdIntelDlg::OnPlayClicked);
		AudioButton->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnPlayClicked);
	}

	if (btn_save)
	{
		btn_save->OnClicked.RemoveDynamic(this, &UCmdIntelDlg::OnSaveClicked);
		btn_save->OnClicked.AddDynamic(this, &UCmdIntelDlg::OnSaveClicked);
	}

	if (IntelList)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CmdIntelDlg] IntelList OK: %s"), *IntelList->GetName());

		IntelList->OnItemSelectionChanged().RemoveAll(this);
		IntelList->OnItemSelectionChanged().AddUObject(this, &UCmdIntelDlg::OnNewsSelectionChanged);
        if (bReportSubpanels) IntelList->OnItemClicked().AddUObject(this, &UCmdIntelDlg::OnNewsSelectionChanged);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CmdIntelDlg] IntelList is NULL"));
	}

	ClearNewsDetails();
	BuildTwoPanelLayout();
	AppendNewEventsIfAny();
}

// +----------------------------------------------------------------------+
// | NativeTick                                                           |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ExecFrame();
}

// +----------------------------------------------------------------------+
// | BindFormWidgets                                                      |
// +----------------------------------------------------------------------+

void UCmdIntelRowProxy::Activate()
{
	if (Owner && Item)
		Owner->OnNewsSelectionChanged(Item);
}

void UCmdIntelDlg::BuildTwoPanelLayout()
{
	if (!WidgetTree || WidgetTree->FindWidget(TEXT("IntelTwoPanel")))
		return;

	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasRoot"));
		WidgetTree->RootWidget = Root;
	}

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IntelPanelBackground"));
	if (UTexture2D* PanelTex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Panel.Panel")))
		Background->SetBrushFromTexture(PanelTex, true);
	Background->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* BgSlot = Root->AddChildToCanvas(Background))
	{
		BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BgSlot->SetOffsets(FMargin(0.f));
		BgSlot->SetZOrder(0);
	}

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("IntelTwoPanel"));
	if (UCanvasPanelSlot* ColumnsSlot = Root->AddChildToCanvas(Columns))
	{
		ColumnsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ColumnsSlot->SetOffsets(FMargin(12.f));
		ColumnsSlot->SetZOrder(10);
	}

	auto MakeText = [this](const FString& Value, int32 Size, const FLinearColor& Color)->UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
		Text->SetText(FText::FromString(Value));
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetAutoWrapText(true);
		FSlateFontInfo Font = Text->GetFont();
		Font = MissionUIStyle::GetRowFont(FMath::Min(Size,16));
		Text->SetFont(Font);
		return Text;
	};

	UBorder* LeftOutline = WidgetTree->ConstructWidget<UBorder>();
	LeftOutline->SetPadding(FMargin(2.f));
	LeftOutline->SetBrushColor(FLinearColor(0.72f, 0.72f, 0.72f, 1.f));
	UBorder* LeftCard = WidgetTree->ConstructWidget<UBorder>();
	LeftCard->SetPadding(FMargin(16.f));
	LeftCard->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.96f));
	UScrollBox* LeftScroll = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBox* LeftContents = WidgetTree->ConstructWidget<UVerticalBox>();
	LeftScroll->AddChild(LeftContents);
	LeftCard->SetContent(LeftScroll);
	LeftOutline->SetContent(LeftCard);
	if (UHorizontalBoxSlot* LeftSlot = Columns->AddChildToHorizontalBox(LeftOutline))
	{
		LeftSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LeftSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}
	LeftContents->AddChildToVerticalBox(MakeText(TEXT("INTEL"), 18, FLinearColor(0.95f, 0.97f, 1.f)));
	IntelCountText = MakeText(TEXT("0 REPORTS"), 14, FLinearColor(0.71f, 0.81f, 0.88f));
	LeftContents->AddChildToVerticalBox(IntelCountText)->SetPadding(FMargin(0.f, 4.f, 0.f, 12.f));
	// Replace the Blueprint list with a native virtualized report list.
    if (IntelList) { IntelList->OnItemSelectionChanged().RemoveAll(this); IntelList->RemoveFromParent(); }
    IntelList = WidgetTree->ConstructWidget<UIntelReportList>(UIntelReportList::StaticClass(), TEXT("NativeIntelList"));
    IntelList->OnItemSelectionChanged().AddUObject(this, &UCmdIntelDlg::OnNewsSelectionChanged);
    IntelList->SetSelectionMode(ESelectionMode::Single);
    LeftScroll->RemoveChild(LeftContents);
    LeftCard->SetContent(LeftContents);
	IntelList->RemoveFromParent();
	IntelList->SetVisibility(ESlateVisibility::Visible);
	if (UVerticalBoxSlot* ListSlot = LeftContents->AddChildToVerticalBox(IntelList))
	{
		FSlateChildSize Fill;
		Fill.SizeRule = ESlateSizeRule::Fill;
		Fill.Value = 1.f;
		ListSlot->SetSize(Fill);
	}

	UBorder* RightOutline = WidgetTree->ConstructWidget<UBorder>();
	RightOutline->SetPadding(FMargin(2.f));
	RightOutline->SetBrushColor(FlightPanelStyle::DividerColor());
	UBorder* RightCard = WidgetTree->ConstructWidget<UBorder>();
	RightCard->SetPadding(FMargin(16.f));
	RightCard->SetBrushColor(FLinearColor(0.08f, 0.08f, 0.08, 0.96f));
	UScrollBox* RightScroll = WidgetTree->ConstructWidget<UScrollBox>();
	UVerticalBox* RightContents = WidgetTree->ConstructWidget<UVerticalBox>();
	RightScroll->AddChild(RightContents);
	RightCard->SetContent(RightScroll);
	RightOutline->SetContent(RightCard);
	if (UHorizontalBoxSlot* RightSlot = Columns->AddChildToHorizontalBox(RightOutline))
	{
		RightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RightSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}
	RightContents->AddChildToVerticalBox(MakeText(TEXT("REPORT"), 18, FLinearColor(0.95f, 0.97f, 1.f)))->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	if (!IntelNameText) IntelNameText = MakeText(TEXT("SELECT A REPORT"), 22, FLinearColor(0.95f, 0.97f, 1.f));
	if (!IntelSourceText) IntelSourceText = MakeText(TEXT(""), 13, FLinearColor(0.71f, 0.81f, 0.88f));
	if (!IntelLocationText) IntelLocationText = MakeText(TEXT(""), 13, FLinearColor(0.71f, 0.81f, 0.88f));
	if (!IntelDateText) IntelDateText = MakeText(TEXT(""), 13, FLinearColor(0.71f, 0.81f, 0.88f));
	if (!IntelMessageText) IntelMessageText = MakeText(TEXT(""), 13, FLinearColor(0.95f, 0.97f, 1.f));
	if (!IntelImage) IntelImage = WidgetTree->ConstructWidget<UImage>();
	IntelImage->SetVisibility(ESlateVisibility::Collapsed);

	UHorizontalBox* TopRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	USizeBox* InfoColumn = WidgetTree->ConstructWidget<USizeBox>();
	InfoColumn->SetWidthOverride(180.f);
	UVerticalBox* Info = WidgetTree->ConstructWidget<UVerticalBox>();
	Info->AddChildToVerticalBox(IntelNameText)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	Info->AddChildToVerticalBox(IntelSourceText);
	Info->AddChildToVerticalBox(IntelLocationText);
	Info->AddChildToVerticalBox(IntelDateText);
	InfoColumn->SetContent(Info);
	if (UHorizontalBoxSlot* InfoSlot = TopRow->AddChildToHorizontalBox(InfoColumn))
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		InfoSlot->SetVerticalAlignment(VAlign_Top);
		InfoSlot->SetPadding(FMargin(0.f, 4.f, 4.f, 4.f));
	}

	USizeBox* ImageBox = WidgetTree->ConstructWidget<USizeBox>();
	ImageBox->SetWidthOverride(160.f);
	ImageBox->SetHeightOverride(160.f);
	IntelImage->SetDesiredSizeOverride(FVector2D(160.f, 160.f));
	ImageBox->SetContent(IntelImage);
	if (UHorizontalBoxSlot* ImageSlot = TopRow->AddChildToHorizontalBox(ImageBox))
	{
		ImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ImageSlot->SetHorizontalAlignment(HAlign_Right);
		ImageSlot->SetVerticalAlignment(VAlign_Top);
		ImageSlot->SetPadding(FMargin(4.f));
	}
	if (UVerticalBoxSlot* TopSlot = RightContents->AddChildToVerticalBox(TopRow))
	{
		TopSlot->SetHorizontalAlignment(HAlign_Fill);
		TopSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}
    RightContents->AddChildToVerticalBox(IntelMessageText);
    if (!AudioButton)
    {
        AudioButton = WidgetTree->ConstructWidget<UButton>();
        auto* Icon = WidgetTree->ConstructWidget<UNativeWidgetHost>();
        Icon->SetContent(SNew(SShipSystemIcon).Icon(EShipSystemIcon::Audio));
        AudioButton->SetContent(Icon);
        AudioButton->SetToolTipText(INVTEXT("Play report audio"));
        AudioButton->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (AudioButton)
    {
        AudioButton->RemoveFromParent();
        MissionUIStyle::ApplyActionButtonStyle(AudioButton, TEXT("Blue"));
        RightContents->AddChildToVerticalBox(AudioButton)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
    }
    if (bReportSubpanels)
    {
        ReplayButton=WidgetTree->ConstructWidget<UButton>();
        auto* ReplayLabel=WidgetTree->ConstructWidget<UTextBlock>();ReplayLabel->SetText(INVTEXT("\u25B6"));ReplayLabel->SetFont(MissionUIStyle::GetHeaderFont(16));
        ReplayButton->SetContent(ReplayLabel);ReplayButton->SetToolTipText(INVTEXT("Replay this cutscene in a window"));
        ReplayButton->OnClicked.AddUniqueDynamic(this,&UCmdIntelDlg::OnReplayClicked);
        ReplayButton->SetVisibility(ESlateVisibility::Collapsed);
        RightContents->AddChildToVerticalBox(ReplayButton);
        LeftOutline->RemoveFromParent();
        RightOutline->RemoveFromParent();
        Columns->SetVisibility(ESlateVisibility::Collapsed);
        LeftCard->SetBrush(FlightPanelStyle::Window()); LeftCard->SetBrushColor(FLinearColor::White);
        RightCard->SetBrush(FlightPanelStyle::Window()); RightCard->SetBrushColor(FLinearColor::White);
        ReportPages = WidgetTree->ConstructWidget<UWidgetSwitcher>();
        auto* PageSlot = Root->AddChildToCanvas(ReportPages);
        PageSlot->SetAnchors(FAnchors(0,0,1,1)); PageSlot->SetOffsets(FMargin(8)); PageSlot->SetZOrder(10);
        ReportPages->AddChild(LeftOutline);
        auto* DetailPage = WidgetTree->ConstructWidget<UVerticalBox>();
        auto* DetailHeader = WidgetTree->ConstructWidget<UHorizontalBox>();
        auto* Back = WidgetTree->ConstructWidget<UButton>();
        auto* BackIcon = WidgetTree->ConstructWidget<UNativeWidgetHost>();
        BackIcon->SetContent(SNew(SShipSystemIcon).Icon(EShipSystemIcon::Back));
        Back->SetContent(BackIcon); Back->SetToolTipText(INVTEXT("Back to intel reports"));
        Back->OnClicked.AddUniqueDynamic(this,&UCmdIntelDlg::BackToIntelList);
        DetailHeader->AddChildToHorizontalBox(Back)->SetPadding(FMargin(0,0,10,0));
        DetailHeader->AddChildToHorizontalBox(MakeText(TEXT("INTEL DETAILS"),16,FLinearColor(.88f,.96f,1.f)));
        DetailPage->AddChildToVerticalBox(DetailHeader)->SetPadding(FMargin(0,0,0,8));
        DetailPage->AddChildToVerticalBox(RightOutline)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ReportPages->AddChild(DetailPage);
        ReplayPage=WidgetTree->ConstructWidget<UVerticalBox>();
        auto* Close=WidgetTree->ConstructWidget<UButton>();auto* CloseLabel=WidgetTree->ConstructWidget<UTextBlock>();
        CloseLabel->SetText(INVTEXT("\u00D7"));CloseLabel->SetFont(MissionUIStyle::GetHeaderFont(18));Close->SetContent(CloseLabel);
        MissionUIStyle::ApplyActionButtonStyle(Close,TEXT("Red"));Close->SetToolTipText(INVTEXT("Close cutscene replay"));
        Close->OnClicked.AddUniqueDynamic(this,&UCmdIntelDlg::CloseReplay);
        ReplayPage->AddChildToVerticalBox(Close)->SetHorizontalAlignment(HAlign_Right);
        ReportPages->AddChild(ReplayPage);

        ReportPages->SetActiveWidgetIndex(0);
    }
}

void UCmdIntelDlg::RefreshIntelRows()
{
	if (IntelCountText && IntelList)
		IntelCountText->SetText(FText::FromString(FString::Printf(TEXT("%d REPORTS"), IntelList->GetNumItems())));
}

void UCmdIntelDlg::BindFormWidgets()
{
	if (!IntelList)
	{
		IntelList = Cast<UListView>(GetWidgetFromName(TEXT("IntelList")));
	}

	if (!IntelNameText)     IntelNameText = Cast<UTextBlock>(GetWidgetFromName(TEXT("IntelNameText")));
	if (!IntelSourceText)   IntelSourceText = Cast<UTextBlock>(GetWidgetFromName(TEXT("IntelSourceText")));
	if (!IntelLocationText) IntelLocationText = Cast<UTextBlock>(GetWidgetFromName(TEXT("IntelLocationText")));
	if (!IntelDateText)     IntelDateText = Cast<UTextBlock>(GetWidgetFromName(TEXT("IntelDateText")));
	if (!IntelMessageText)  IntelMessageText = Cast<UTextBlock>(GetWidgetFromName(TEXT("IntelMessageText")));
	if (!IntelImage)        IntelImage = Cast<UImage>(GetWidgetFromName(TEXT("IntelImage")));
	if (!AudioButton)       AudioButton = Cast<UButton>(GetWidgetFromName(TEXT("AudioButton")));

	if (!mov_news)
	{
		mov_news = GetWidgetFromName(TEXT("mov_news"));
	}
}

// +----------------------------------------------------------------------+
// | ShowIntelDlg                                                         |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::ShowIntelDlg()
{
	SetVisibility(ESlateVisibility::Visible);
	SetClipping(EWidgetClipping::ClipToBounds);

	if (UWidgetSwitcherSlot* SwitcherSlot = Cast<UWidgetSwitcherSlot>(Slot))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
		SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
		SwitcherSlot->SetPadding(FMargin(0.f));
	}
	else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(0.f));
		CanvasSlot->SetAutoSize(false);
	}

	if (WidgetTree)
	{
		if (UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			Root->SetClipping(EWidgetClipping::ClipToBounds);
			for (UWidget* Child : Root->GetAllChildren())
			{
				if (UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(Child ? Child->Slot : nullptr))
				{
					ChildSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
					ChildSlot->SetOffsets(Child->GetName() == TEXT("IntelTwoPanel") ? FMargin(12.f) : FMargin(0.f));
					ChildSlot->SetAutoSize(false);
				}
			}
		}
	}

	RefreshIntelData();
}

// +----------------------------------------------------------------------+
// | ExecFrame                                                            |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::ExecFrame()
{
	RebuildNewsListIfCampaignChanged();

	if (StartSceneCountdown > 0)
	{
		--StartSceneCountdown;

		if (StartSceneCountdown == 0 && !EventScene.IsEmpty())
		{
			// scene playback hook if/when needed
		}
	}
}

// +----------------------------------------------------------------------+
// | RebuildNewsListIfCampaignChanged                                     |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::EnsureTrainingIntroduction(Campaign* Story)
{
    // Training is agent-driven now; it no longer visits the Operations screen
    // that used to publish the campaign-start event. Keep its introduction in Intel.
    if (Story && Story->GetCampaignId() == 1)
    {
        CombatEvent* Introduction = nullptr;
        for (int32 Index=0; Index<Story->GetEvents().size(); ++Index)
        {
            auto* Entry=Story->GetEvents()[Index];
            if (Entry && FString(UTF8_TO_TCHAR(Entry->SceneFile())).Equals(TEXT("Intro"),ESearchCase::IgnoreCase))
            { Introduction=Entry; break; }
        }
        if (!Introduction)
        {
            Introduction = new CombatEvent(Story,ECombatEventType::NONE,0,1,ECombatEventSource::NONE,"Janus");
            Introduction->SetSceneFile("Intro");
            Story->GetEvents().append(Introduction);
        }
        Introduction->SetTitle("Operation Live Fire");
        Introduction->SetInformation("Third Fleet is conducting live-fire exercises in the Janus system. "
            "Report to Lieutenant Mara Venn for your training assignments. Initial lessons cover space flight, "
            "combat, atmospheric operations and recovery; later assignments introduce capital ships as your rank and qualifications advance. "
            "The original Training Introduction is retained with this report.");
    }
}

void UCmdIntelDlg::RebuildNewsListIfCampaignChanged()
{
	Campaign* Current = Campaign::GetCampaign();
    if(auto* Game=Cast<USSWGameInstance>(GetGameInstance());Game && Game->IsTrainingTestSession())Current=Game->GetAgentTrainingCampaign();
	if (!Current)
    {
        CampaignPtr = nullptr;
        if (IntelList) IntelList->ClearListItems();
        ClearNewsDetails();
        RefreshIntelRows();
        return;
    }

    EnsureTrainingIntroduction(Current);
	const double CurrentUpdateTime = Current->GetUpdateTime();

	if (CampaignPtr != Current || UpdateTime != CurrentUpdateTime ||
        (IntelList && IntelList->GetNumItems() != Current->GetEvents().size()))
	{
        if (CampaignPtr != Current)
        {
            if (IntelList) IntelList->ClearListItems();
            ClearNewsDetails();
        }
        CampaignPtr = Current;
        UpdateTime = CurrentUpdateTime;
		AppendNewEventsIfAny();
	}
}

// +----------------------------------------------------------------------+
// | AppendNewEventsIfAny                                                 |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::AppendNewEventsIfAny()
{
    TGuardValue<bool> RefreshGuard(bRefreshingIntelList,true);
	if (!CampaignPtr || !IntelList)
		return;

	CombatEvent* PreviousEvent = nullptr;
	if (UIntelListObject* Previous = Cast<UIntelListObject>(IntelList->GetSelectedItem())) PreviousEvent = Previous->EventPtr;
	IntelList->ClearSelection();
	ClearNewsDetails();
	IntelList->ClearListItems();

	List<CombatEvent>& Events = CampaignPtr->GetEvents();

	for (int32 i = 0; i < Events.size(); ++i)
	{
		CombatEvent* Info = Events[i];
		if (!Info)
			continue;

		UIntelListObject* Item = NewObject<UIntelListObject>(this);

		char Dateline[32] = { 0 };
		UFormattingUtils::FormatDayTime(Dateline, Info->GetTime());

		Item->NewsTitle = UTF8_TO_TCHAR(Info->GetTitle());
		Item->NewsLocation = UTF8_TO_TCHAR(Info->GetRegion());
		Item->NewsSource = Info->GetEventSourceName();
		Item->NewsDate = UTF8_TO_TCHAR(Dateline);
		Item->NewsInfoText = UTF8_TO_TCHAR(Info->GetInformation());
		Item->NewsVisited = Info->Visited();
		Item->EventPtr = Info;

		Item->NewsImage = Info->ImageFile() ? UTF8_TO_TCHAR(Info->ImageFile()) : TEXT("");
		Item->NewsAudio = Info->AudioFile() ? UTF8_TO_TCHAR(Info->AudioFile()) : TEXT("");

		IntelList->AddItem(Item);
		if (Info == PreviousEvent) IntelList->SetSelectedItem(Item);
	}

	AutoScrollToFirstUnreadIfNeeded();
	RefreshIntelRows();
	if (GetVisibility() == ESlateVisibility::Visible) AutoSelectFirstItemIfNeeded();
}

// +----------------------------------------------------------------------+
// | AutoScrollToFirstUnreadIfNeeded                                      |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::AutoScrollToFirstUnreadIfNeeded()
{
	if (!IntelList)
		return;

	const int32 Num = IntelList->GetNumItems();
	for (int32 i = 0; i < Num; ++i)
	{
		UObject* Obj = IntelList->GetItemAt(i);
		UIntelListObject* Item = Cast<UIntelListObject>(Obj);
		if (Item && !Item->NewsVisited)
		{
			IntelList->ScrollIndexIntoView(i);
			break;
		}
	}
}

// +----------------------------------------------------------------------+
// | ClearNewsDetails                                                     |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::ClearNewsDetails()
{
	if (IntelNameText)     IntelNameText->SetText(FText::GetEmpty());
	if (IntelSourceText)   IntelSourceText->SetText(FText::GetEmpty());
	if (IntelLocationText) IntelLocationText->SetText(FText::GetEmpty());
	if (IntelDateText)     IntelDateText->SetText(FText::GetEmpty());
	if (IntelMessageText)  IntelMessageText->SetText(FText::GetEmpty());

	if (IntelImage)
	{
		IntelImage->SetBrush(FSlateBrush());
		IntelImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
		IntelImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UWidget* ImageField = GetWidgetFromName(TEXT("ImageField")))
		ImageField->SetVisibility(ESlateVisibility::Collapsed);
	if (UWidget* ImageBorder = GetWidgetFromName(TEXT("IntelImageBorder")))
		ImageBorder->SetVisibility(ESlateVisibility::Collapsed);
	if (AudioButton) AudioButton->SetVisibility(ESlateVisibility::Collapsed);
	if (UWidget* Detail = GetWidgetFromName(TEXT("NewsDetail")))
		Detail->SetVisibility(ESlateVisibility::Hidden);
	if (UWidget* DetailPanel = GetWidgetFromName(TEXT("IntelDetail")))
		DetailPanel->SetVisibility(ESlateVisibility::Hidden);
}

// +----------------------------------------------------------------------+
// | GetSelectedEvent                                                     |
// +----------------------------------------------------------------------+

CombatEvent* UCmdIntelDlg::GetSelectedEvent(int32& OutSelectedIndex) const
{
	OutSelectedIndex = -1;

	if (!IntelList)
		return nullptr;

	UObject* Selected = IntelList->GetSelectedItem();
	if (!Selected)
		return nullptr;

	OutSelectedIndex = IntelList->GetIndexForItem(Selected);

	UIntelListObject* Item = Cast<UIntelListObject>(Selected);
	if (!Item)
		return nullptr;

	return Item->EventPtr;
}

// +----------------------------------------------------------------------+
// | OnNewsSelectionChanged                                               |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::OnNewsSelectionChanged(UObject* Item)
{
    if (bReportSubpanels && bRefreshingIntelList) return;
	UE_LOG(LogTemp, Warning, TEXT("[CmdIntelDlg] SelectionChanged fired"));

	if (!Item)
	{
		ClearNewsDetails();
		return;
	}

	UIntelListObject* IntelItem = Cast<UIntelListObject>(Item);
	if (!IntelItem)
	{
		UE_LOG(LogTemp, Error, TEXT("[CmdIntelDlg] SelectionChanged: Cast failed. Class=%s"),
			*Item->GetClass()->GetName());
		return;
	}

	SetSelectedIntelData(IntelItem);
    if (ReportPages) ReportPages->SetActiveWidgetIndex(1);

	IntelItem->NewsVisited = true;
	if (IntelItem->EventPtr)
	{
		IntelItem->EventPtr->SetVisited(true);
	}

	if (IntelList)
	{
		IntelList->RequestRefresh();
        for (UUserWidget* Entry : IntelList->GetDisplayedEntryWidgets())
            if (UIntelLVElement* Report = Cast<UIntelLVElement>(Entry))
                Report->NativeOnListItemObjectSet(Report->GetListItem<UIntelListObject>());
    }
}

// +----------------------------------------------------------------------+
// | SetSelectedIntelData                                                 |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::SetSelectedIntelData(UIntelListObject* Item)
{
    if (ReplayButton)
    {
        bool bSeen=false;
        CombatEvent* Entry=Item?Item->EventPtr:nullptr;
        if (Entry && Entry->SceneFile() && *Entry->SceneFile())
        {
            const auto* GI=Cast<USSWGameInstance>(GetGameInstance());
            const auto* Save=GI?GI->CampaignSave:nullptr;
            bSeen=(Save && CampaignPtr && Save->CampaignIndex==CampaignPtr->GetCampaignId())
                ? Save->HasPlayedScene(UTF8_TO_TCHAR(Entry->SceneFile())) : false;
            // The training introduction is available as the training archive's first entry.
            bSeen |= CampaignPtr && CampaignPtr->GetCampaignId()==1;
        }
        ReplayButton->SetVisibility(Entry && Entry->SceneFile() && *Entry->SceneFile()?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
        ReplayButton->SetIsEnabled(bSeen);
        ReplayButton->SetToolTipText(bSeen?INVTEXT("Replay this cutscene in a window"):INVTEXT("Available after the first viewing"));
    }

	if (!Item)
		return;

	if (IntelNameText)
		IntelNameText->SetText(FText::FromString(Item->NewsTitle));

	if (IntelLocationText)
		IntelLocationText->SetText(FText::FromString(Item->NewsLocation));

	if (IntelSourceText)
		IntelSourceText->SetText(FText::FromString(Item->NewsSource));

	if (IntelDateText)
		IntelDateText->SetText(FText::FromString(Item->NewsDate));

	if (IntelMessageText)
	{
		FString Message = Item->NewsInfoText;
		Message = Message.Replace(TEXT("\\n"), TEXT("\n"));
		IntelMessageText->SetText(FText::FromString(Message));
	}

	const FString IntelImageAssetPath = GetIntelImageAssetPath(Item->NewsImage);
	GetIntelAudioFile(Item->NewsAudio);

	UTexture2D* LoadedTexture = LoadTextureFromAssetPath(IntelImageAssetPath);
	if (LoadedTexture && IntelImage)
	{
		FSlateBrush Brush = CreateBrushFromTexture(
			LoadedTexture,
			FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
		IntelImage->SetBrush(Brush);
		IntelImage->SetColorAndOpacity(FLinearColor::White);
		IntelImage->SetVisibility(ESlateVisibility::Visible);
		if (UWidget* ImageField = GetWidgetFromName(TEXT("ImageField")))
			ImageField->SetVisibility(ESlateVisibility::Visible);
		if (UWidget* ImageBorder = GetWidgetFromName(TEXT("IntelImageBorder")))
			ImageBorder->SetVisibility(ESlateVisibility::Visible);
	}
	else if (IntelImage)
	{
		IntelImage->SetBrush(FSlateBrush());
		IntelImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.f));
		IntelImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	const bool bHasAudio = !Item->NewsAudio.IsEmpty();
	if (AudioButton)
	{
		AudioButton->SetVisibility(bHasAudio ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

// +----------------------------------------------------------------------+
// | OnPlayClicked                                                        |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::OnPlayClicked()
{
	int32 Index = -1;
	CombatEvent* EventPtr = GetSelectedEvent(Index);
	if (!EventPtr)
		return;

	USoundBase* AudioAsset = EventPtr->GetAudio();
	if (!AudioAsset)
	{
		const FString EventAudioName = EventPtr->AudioFile() ? UTF8_TO_TCHAR(EventPtr->AudioFile()) : TEXT("");
		if (EventAudioName.IsEmpty())
			return;

		GetIntelAudioFile(EventAudioName);

		AudioAsset = Cast<USoundBase>(
			StaticLoadObject(USoundBase::StaticClass(), nullptr, *AudioPath)
		);

		EventPtr->SetAudio(AudioAsset);
	}

	if (AudioAsset)
	{
		UGameplayStatics::PlaySound2D(this, AudioAsset);
	}
}

// +----------------------------------------------------------------------+
// | ShowMovie                                                            |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::ShowMovie()
{
	if (mov_news)
	{
		mov_news->SetVisibility(ESlateVisibility::Visible);
	}

	if (IntelImage)       IntelImage->SetVisibility(ESlateVisibility::Collapsed);
	if (IntelMessageText) IntelMessageText->SetVisibility(ESlateVisibility::Collapsed);
	if (AudioButton)      AudioButton->SetVisibility(ESlateVisibility::Collapsed);
}

// +----------------------------------------------------------------------+
// | HideMovie                                                            |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::HideMovie()
{
	int32 Index = -1;
	CombatEvent* EventPtr = GetSelectedEvent(Index);
	const bool bPlay = (EventPtr && EventPtr->AudioFile() && *EventPtr->AudioFile());

	if (mov_news)
	{
		mov_news->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IntelImage)       IntelImage->SetVisibility(ESlateVisibility::Visible);
	if (IntelMessageText) IntelMessageText->SetVisibility(ESlateVisibility::Visible);

	if (AudioButton)
	{
		AudioButton->SetVisibility(bPlay ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

// +----------------------------------------------------------------------+
// | SetModeAndRoute                                                      |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::SetModeAndRoute(ECOMMAND_MODE InMode)
{
	Mode = InMode;

	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CmdIntelDlg] Manager is null (SetModeAndRoute)."));
		return;
	}

	// expand when needed
}

// +----------------------------------------------------------------------+
// | OnSaveClicked                                                        |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::OnSaveClicked()
{
	if (Manager)
	{
		Manager->ShowCmpFileDlg();
	}
}

// +----------------------------------------------------------------------+
// | GetIntelImageAssetPath                                               |
// +----------------------------------------------------------------------+

FString UCmdIntelDlg::GetIntelImageAssetPath(const FString& IntelImageName) const
{
	if (IntelImageName.IsEmpty() ||
		IntelImageName.Equals(TEXT("Empty"), ESearchCase::IgnoreCase))
	{
		return FString();
	}

	Campaign* CurrentCampaign = Campaign::GetCampaign();
	if (!CurrentCampaign)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CmdIntelDlg] No Campaign when resolving intel image"));
		return FString();
	}

	int32 CampaignIndex = CurrentCampaign->GetCampaignId();
	CampaignIndex = FMath::Max(1, CampaignIndex);

	const FString CampaignFolder = FString::Printf(TEXT("%02d"), CampaignIndex);
    FString ImageToken = IntelImageName;
    ImageToken.ReplaceInline(TEXT("\\"), TEXT("/"));
    const FString ImageName = FPaths::GetBaseFilename(ImageToken);

	const FString AssetPath = FString::Printf(
		TEXT("/Game/UI/Campaigns/%s/%s.%s"),
		*CampaignFolder,
		*ImageName,
		*ImageName);

	UE_LOG(LogTemp, Log,
		TEXT("[CmdIntelDlg] Intel Image Asset: %s"),
		*AssetPath);

	return AssetPath;
}

// +----------------------------------------------------------------------+
// | GetIntelAudioFile                                                    |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::GetIntelAudioFile(const FString& IntelAudioName)
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
	if (!SSWInstance)
		return;

    FString Token = IntelAudioName;
    Token.ReplaceInline(TEXT("\\"), TEXT("/"));
    Token = FPaths::GetBaseFilename(Token);
    const Campaign* CurrentCampaign = Campaign::GetCampaign();
    const int32 Number = CurrentCampaign ? CurrentCampaign->GetCampaignId() : SSWInstance->GetActiveCampaign().Index + 1;
    AudioPath = Token.IsEmpty() ? FString() : FString::Printf(
        TEXT("/Game/Audio/Vox/Scenes/%02d/%s.%s"), Number, *Token, *Token);

	UE_LOG(LogTemp, Log, TEXT("Action Audio: %s"), *AudioPath);
}

// +----------------------------------------------------------------------+
// | LoadTextureFromAssetPath                                             |
// +----------------------------------------------------------------------+

UTexture2D* UCmdIntelDlg::LoadTextureFromAssetPath(const FString& AssetPath) const
{
	if (AssetPath.IsEmpty())
	{
		return nullptr;
	}

	UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, *AssetPath);

	if (!LoadedTexture)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[CmdIntelDlg] Failed to load intel image asset: %s"),
			*AssetPath);
	}

	return LoadedTexture;
}

// +----------------------------------------------------------------------+
// | CreateBrushFromTexture                                               |
// +----------------------------------------------------------------------+

FSlateBrush UCmdIntelDlg::CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize)
{
	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.ImageSize = ImageSize;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	return Brush;
}

void UCmdIntelDlg::AutoSelectFirstItemIfNeeded()
{
    if (bReportSubpanels) return;
	if (!IntelList || IntelList->GetNumItems() <= 0)
	{
		return;
	}

	UObject* Selected = IntelList->GetSelectedItem();
	if (Selected)
	{
		return;
	}

	// Prefer first unread item
	for (int32 i = 0; i < IntelList->GetNumItems(); ++i)
	{
		UObject* Obj = IntelList->GetItemAt(i);
		UIntelListObject* Item = Cast<UIntelListObject>(Obj);
		if (Item && !Item->NewsVisited)
		{
			IntelList->SetSelectedItem(Item);
			SetSelectedIntelData(Item);

			Item->NewsVisited = true;
			if (Item->EventPtr)
			{
				Item->EventPtr->SetVisited(true);
			}

			IntelList->RequestRefresh();
			return;
		}
	}

	// Fallback to first item
	if (UObject* FirstObj = IntelList->GetItemAt(0))
	{
		if (UIntelListObject* FirstItem = Cast<UIntelListObject>(FirstObj))
		{
			IntelList->SetSelectedItem(FirstItem);
			SetSelectedIntelData(FirstItem);

			FirstItem->NewsVisited = true;
			if (FirstItem->EventPtr)
			{
				FirstItem->EventPtr->SetVisited(true);
			}

			IntelList->RequestRefresh();
		}
	}
}

void UCmdIntelDlg::RefreshIntelData()
{
	RebuildNewsListIfCampaignChanged();
	AppendNewEventsIfAny();
}
void UCmdIntelDlg::BackToIntelList()
{
    CloseReplay();
    if (ReportPages) ReportPages->SetActiveWidgetIndex(0);
}

void UCmdIntelDlg::OnReplayClicked()
{
    int32 Index=INDEX_NONE;CombatEvent* Entry=GetSelectedEvent(Index);
    if(!Entry || !ReplayPage || !ReplayButton || !ReplayButton->GetIsEnabled())return;
    auto* Data=GetGameInstance()?GetGameInstance()->GetSubsystem<UStarshatterGameDataSubsystem>():nullptr;
    const int32 Number=CampaignPtr?CampaignPtr->GetCampaignId():1;
    const FS_Campaign* Story=Data?Data->GetCampaignByIndex1Based(Number):nullptr;
    if(!Story)return;
    for(const auto& Scene:Story->Missions)
        if(Scene.Scene.Equals(UTF8_TO_TCHAR(Entry->SceneFile()),ESearchCase::IgnoreCase))
        {
            CloseReplay();
            ReplayPanel=CreateWidget<URecordedCutsceneDlg>(GetOwningPlayer());
            if(!ReplayPanel)return;
            ReplayPage->AddChildToVerticalBox(ReplayPanel)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            ReplayPanel->TakeWidget();ReplayPanel->LoadMovie(URecordedCutsceneDlg::MoviePath(Number,Scene.Scene));
            ReportPages->SetActiveWidgetIndex(2);return;
        }
}
void UCmdIntelDlg::CloseReplay()
{
    if(ReplayPanel){ReplayPanel->RemoveFromParent();ReplayPanel=nullptr;}
    if(ReportPages && ReportPages->GetActiveWidgetIndex()==2)ReportPages->SetActiveWidgetIndex(1);
}
