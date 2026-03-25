// +----------------------------------------------------------------------+
// | UCmdIntelDlg                                                         |
// +----------------------------------------------------------------------+
// | PURPOSE:                                                             |
// |   Displays campaign intel events (news feed).                        |
// |                                                                      |
// | RESPONSIBILITIES:                                                    |
// |   - Populate intel list from Campaign events                         |
// |   - Track unread/visited events                                      |
// |   - Display selected event details                                   |
// |   - Handle scene playback (audio/video)                              |
// |   - Route command screen navigation                                  |
// +----------------------------------------------------------------------+

#include "CmdIntelDlg.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "IntelListObject.h"
#include "Blueprint/WidgetTree.h"

#include "CmpnScreen.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "Game.h"
#include "Starshatter.h"
#include "PlayerCharacter.h"
#include "FormattingUtils.h"
#include "Mouse.h"

UCmdIntelDlg::UCmdIntelDlg(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UCmdIntelDlg::SetManager(UCmpnScreen* InManager)
{
	Manager = InManager;
}

void UCmdIntelDlg::SetParentCmdDlg(UCmdDlg* InParentCmdDlg)
{
	ParentCmdDlg = InParentCmdDlg;
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
		IntelList->OnItemClicked().RemoveAll(this);
		IntelList->OnItemClicked().AddUObject(this, &UCmdIntelDlg::OnNewsItemClicked);
	}

	ClearNewsDetails();
	AppendNewEventsIfAny();
}

void UCmdIntelDlg::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	ExecFrame();
}

void UCmdIntelDlg::BindFormWidgets()
{
	// If lst_news is not directly bound, resolve the actual BP widget name.
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

	// Optional movie widget if present in the BP
	if (!mov_news)
	{
		mov_news = GetWidgetFromName(TEXT("mov_news"));
	}
}

void UCmdIntelDlg::ShowIntelDlg()
{
	SetVisibility(ESlateVisibility::Visible);
	AppendNewEventsIfAny();
}

void UCmdIntelDlg::ExecFrame()
{
	RebuildNewsListIfCampaignChanged();
	AppendNewEventsIfAny();

	if (StartSceneCountdown > 0)
	{
		--StartSceneCountdown;

		if (StartSceneCountdown == 0 && !EventScene.IsEmpty())
		{
			// Hook your scene playback here if needed.
		}
	}
}

void UCmdIntelDlg::RebuildNewsListIfCampaignChanged()
{
	Campaign* Current = Campaign::GetCampaign();
	if (!Current)
		return;

	const double CurrentUpdateTime = Current->GetUpdateTime();

	if (CampaignPtr != Current || UpdateTime != CurrentUpdateTime)
	{
		CampaignPtr = Current;
		UpdateTime = CurrentUpdateTime;

		if (IntelList)
		{
			IntelList->ClearListItems();
		}

		ClearNewsDetails();
	}
}

// +----------------------------------------------------------------------+
// | AppendNewEventsIfAny                                                 |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::AppendNewEventsIfAny()
{
	if (!CampaignPtr || !IntelList)
		return;

	IntelList->ClearListItems();

	List<CombatEvent>& Events = CampaignPtr->GetEvents();

	for (int32 i = 0; i < Events.size(); ++i)
	{
		CombatEvent* Info = Events[i];
		if (!Info)
			continue;

		UIntelListObject* Item = NewObject<UIntelListObject>(this);

		char Dateline[32] = { 0 };
		UFormattingUtils::FormatDayTime(Dateline, Info->Time());

		Item->NewsTitle = UTF8_TO_TCHAR(Info->Title());
		Item->NewsLocation = UTF8_TO_TCHAR(Info->Region());
		Item->NewsSource = Info->GetEventSourceName();
		Item->NewsDate = UTF8_TO_TCHAR(Dateline);
		Item->NewsInfoText = UTF8_TO_TCHAR(Info->Information());
		Item->NewsVisited = Info->Visited();
		Item->EventPtr = Info;

		Item->NewsImage = TEXT("");
		Item->NewsAudio = TEXT("");

		IntelList->AddItem(Item);
	}
}

void UCmdIntelDlg::AutoScrollToFirstUnreadIfNeeded()
{
	if (!IntelList)
		return;

	const int32 Num = IntelList->GetNumItems();
	for (int32 i = 0; i < Num; ++i)
	{
		UObject* Obj = IntelList->GetItemAt(i);
		UCmdIntelNewsItem* Item = Cast<UCmdIntelNewsItem>(Obj);
		if (Item && Item->UnreadMark == TEXT("*"))
		{
			IntelList->ScrollIndexIntoView(i);
			break;
		}
	}
}

void UCmdIntelDlg::ClearNewsDetails()
{
	if (IntelNameText)     IntelNameText->SetText(FText::GetEmpty());
	if (IntelSourceText)   IntelSourceText->SetText(FText::GetEmpty());
	if (IntelLocationText) IntelLocationText->SetText(FText::GetEmpty());
	if (IntelDateText)     IntelDateText->SetText(FText::GetEmpty());
	if (IntelMessageText)  IntelMessageText->SetText(FText::GetEmpty());

	if (IntelImage)
	{
		if (DefaultNewsTexture)
		{
			IntelImage->SetBrushFromTexture(DefaultNewsTexture, true);
		}
		else
		{
			IntelImage->SetBrush(FSlateBrush());
		}
	}

	if (AudioButton)
	{
		AudioButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}

CombatEvent* UCmdIntelDlg::GetSelectedEvent(int32& OutSelectedIndex) const
{
	OutSelectedIndex = -1;

	if (!IntelList)
		return nullptr;

	UObject* Selected = IntelList->GetSelectedItem();
	if (!Selected)
		return nullptr;

	OutSelectedIndex = IntelList->GetIndexForItem(Selected);

	UCmdIntelNewsItem* Item = Cast<UCmdIntelNewsItem>(Selected);
	if (!Item)
		return nullptr;

	return Item->EventPtr;
}

// +----------------------------------------------------------------------+
// | OnNewsItemClicked                                                    |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::OnNewsItemClicked(UObject* Item)
{
	UIntelListObject* IntelItem = Cast<UIntelListObject>(Item);
	if (!IntelItem || !IntelItem->EventPtr)
		return;

	// Populate detail panel
	SetSelectedIntelData(IntelItem);

	// Mark visited
	IntelItem->NewsVisited = true;
	IntelItem->EventPtr->SetVisited(true);

	IntelList->RequestRefresh();
}

void UCmdIntelDlg::ShowSelectedEvent(CombatEvent* EventPtr, int32 SelectedIndex)
{
	if (!EventPtr)
	{
		ClearNewsDetails();
		return;
	}

	if (IntelNameText)
	{
		IntelNameText->SetText(FText::FromString(UTF8_TO_TCHAR(EventPtr->Title())));
	}

	if (IntelLocationText)
	{
		IntelLocationText->SetText(FText::FromString(UTF8_TO_TCHAR(EventPtr->Region())));
	}

	if (IntelSourceText)
	{
		IntelSourceText->SetText(FText::FromString(EventPtr->GetEventSourceName()));
	}

	if (IntelDateText)
	{
		char Dateline[32] = { 0 };
		UFormattingUtils::FormatDayTime(Dateline, EventPtr->Time());
		IntelDateText->SetText(FText::FromString(UTF8_TO_TCHAR(Dateline)));
	}

	if (IntelMessageText)
	{
		FString Info = UTF8_TO_TCHAR(EventPtr->Information());
		Info = Info.Replace(TEXT("\\n"), TEXT("\n"));
		IntelMessageText->SetText(FText::FromString(Info));
	}

	if (SelectedIndex >= 0 && IntelList)
	{
		UObject* Obj = IntelList->GetItemAt(SelectedIndex);
		UCmdIntelNewsItem* NewsItem = Cast<UCmdIntelNewsItem>(Obj);
		if (NewsItem)
		{
			NewsItem->UnreadMark = TEXT(" ");
			IntelList->RequestRefresh();
		}
	}

	if (IntelImage)
	{
		if (DefaultNewsTexture)
		{
			IntelImage->SetBrushFromTexture(DefaultNewsTexture, true);
		}
		else
		{
			IntelImage->SetBrush(FSlateBrush());
		}
	}

	const bool bHasScene = (EventPtr->SceneFile() && *EventPtr->SceneFile());

	if (AudioButton)
	{
		AudioButton->SetVisibility(bHasScene ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (!EventPtr->Visited() && bHasScene && AudioButton && AudioButton->GetIsEnabled())
	{
		OnPlayClicked();
	}

	EventPtr->SetVisited(true);
}

void UCmdIntelDlg::OnPlayClicked()
{
	int32 Index = -1;
	CombatEvent* EventPtr = GetSelectedEvent(Index);
	if (!EventPtr)
		return;

	if (!EventPtr->SceneFile() || !*EventPtr->SceneFile())
		return;

	EventScene = UTF8_TO_TCHAR(EventPtr->SceneFile());
	StartSceneCountdown = 2;

	ShowMovie();
}

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

void UCmdIntelDlg::HideMovie()
{
	bool bPlay = false;

	int32 Index = -1;
	CombatEvent* EventPtr = GetSelectedEvent(Index);
	if (EventPtr && EventPtr->SceneFile() && *EventPtr->SceneFile())
	{
		bPlay = true;
	}

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

void UCmdIntelDlg::SetModeAndRoute(ECOMMAND_MODE InMode)
{
	Mode = InMode;

	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("CmdIntelDlg: Manager is null (SetModeAndRoute)."));
		return;
	}
}

void UCmdIntelDlg::OnSaveClicked()
{
	if (Manager)
	{
		Manager->ShowCmpFileDlg();
	}
}

// +----------------------------------------------------------------------+
// | SetSelectedIntelData                                                 |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::SetSelectedIntelData(UIntelListObject* Item)
{
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

	GetIntelImageFile(Item->NewsImage);
	GetIntelAudioFile(Item->NewsAudio);

	UTexture2D* LoadedTexture = LoadTextureFromFile();
	if (LoadedTexture && IntelImage)
	{
		FSlateBrush Brush = CreateBrushFromTexture(
			LoadedTexture,
			FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
		IntelImage->SetBrush(Brush);
	}
}

// +----------------------------------------------------------------------+
// | GetIntelImageFile                                                    |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::GetIntelImageFile(const FString& IntelImageName)
{
	if (!CampaignPtr)
		return;

	ImagePath = FPaths::ProjectContentDir() + TEXT("UI/Campaigns/0");
	ImagePath.Append(FString::FromInt(CampaignPtr->GetCampaignId() + 1));
	ImagePath.Append(TEXT("/"));
	ImagePath.Append(IntelImageName);
	ImagePath.Append(TEXT(".png"));

	UE_LOG(LogTemp, Log, TEXT("Action Image: %s"), *ImagePath);
}

// +----------------------------------------------------------------------+
// | GetIntelAudioFile                                                    |
// +----------------------------------------------------------------------+

void UCmdIntelDlg::GetIntelAudioFile(const FString& IntelAudioName)
{
	if (!CampaignPtr)
		return;

	AudioPath = TEXT("/Game/Audio/Vox/Scenes/0");
	AudioPath.Append(FString::FromInt(CampaignPtr->GetCampaignId() + 1));
	AudioPath.Append(TEXT("/"));
	AudioPath.Append(IntelAudioName);
	AudioPath.Append(TEXT("."));
	AudioPath.Append(IntelAudioName);

	UE_LOG(LogTemp, Log, TEXT("Action Audio: %s"), *AudioPath);
}

// +----------------------------------------------------------------------+
// | LoadTextureFromFile                                                  |
// +----------------------------------------------------------------------+

UTexture2D* UCmdIntelDlg::LoadTextureFromFile()
{
	USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
	if (!SSWInstance)
		return nullptr;

	return SSWInstance->LoadPNGTextureFromFile(ImagePath);
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
