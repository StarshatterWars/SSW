// +----------------------------------------------------------------------+
// | UCmdIntelDlg                                                         |
// +----------------------------------------------------------------------+

#include "CmdIntelDlg.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Texture2D.h"

#include "IntelListObject.h"
#include "CmpnScreen.h"
#include "Campaign.h"
#include "CombatEvent.h"
#include "CombatGroup.h"
#include "Game.h"
#include "Starshatter.h"
#include "PlayerCharacter.h"
#include "FormattingUtils.h"
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
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CmdIntelDlg] IntelList is NULL"));
	}

	ClearNewsDetails();
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
	}

	AutoScrollToFirstUnreadIfNeeded();
	AutoSelectFirstItemIfNeeded();
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
	UE_LOG(LogTemp, Warning, TEXT("[CmdIntelDlg] SelectionChanged fired"));

	if (!Item)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CmdIntelDlg] SelectionChanged: Item is NULL"));
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

	IntelItem->NewsVisited = true;
	if (IntelItem->EventPtr)
	{
		IntelItem->EventPtr->SetVisited(true);
	}

	if (IntelList)
	{
		IntelList->RequestRefresh();
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

	const FString IntelImageAssetPath = GetIntelImageAssetPath(Item->NewsImage);
	GetIntelAudioFile(Item->NewsAudio);

	UTexture2D* LoadedTexture = LoadTextureFromAssetPath(IntelImageAssetPath);
	if (LoadedTexture && IntelImage)
	{
		FSlateBrush Brush = CreateBrushFromTexture(
			LoadedTexture,
			FVector2D(LoadedTexture->GetSizeX(), LoadedTexture->GetSizeY()));
		IntelImage->SetBrush(Brush);
	}
	else if (IntelImage)
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

	const FString AssetPath = FString::Printf(
		TEXT("/Game/UI/Campaigns/%s/%s.%s"),
		*CampaignFolder,
		*IntelImageName,
		*IntelImageName);

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

	AudioPath = TEXT("/Game/Audio/Vox/Scenes/0");
	AudioPath.Append(FString::FromInt(SSWInstance->GetActiveCampaign().Index + 1));
	AudioPath.Append(TEXT("/"));
	AudioPath.Append(IntelAudioName);
	AudioPath.Append(TEXT("."));
	AudioPath.Append(IntelAudioName);

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