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

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "Blueprint/UserWidget.h"
#include "CmdDlg.h"
#include "GameStructs.h"
#include "CmdIntelDlg.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UListView;
class UWidget;
class UIntelListObject;

class Starshatter;
class Campaign;
class CombatEvent;
class UCmpnScreen;

UENUM(BlueprintType)
enum class ECmdIntelRowType : uint8
{
	Event,
	Blank
};

UCLASS()
class STARSHATTERWARS_API UCmdIntelNewsItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY() FString UnreadMark;
	UPROPERTY() FString Date;
	UPROPERTY() FString Title;
	UPROPERTY() FString Loc;
	UPROPERTY() FString Source;

	CombatEvent* EventPtr = nullptr;

	UPROPERTY() ECmdIntelRowType RowType = ECmdIntelRowType::Event;
};

UCLASS()
class STARSHATTERWARS_API UCmdIntelDlg : public UBaseScreen
{
	GENERATED_BODY()

public:
	UCmdIntelDlg(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	void SetManager(UCmpnScreen* InManager);
	void SetParentCmdDlg(UCmdDlg* InParentCmdDlg);
	void ShowIntelDlg();

private:
	void BindFormWidgets();

	void ExecFrame();

	void RebuildNewsListIfCampaignChanged();
	void AppendNewEventsIfAny();
	void AutoScrollToFirstUnreadIfNeeded();

	void ShowSelectedEvent(CombatEvent* EventPtr, int32 SelectedIndex);
	CombatEvent* GetSelectedEvent(int32& OutSelectedIndex) const;

	void ShowMovie();
	void HideMovie();

	void ClearNewsDetails();

	void SetModeAndRoute(ECOMMAND_MODE InMode);

private:
	UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_save = nullptr;

	// Existing list binding if your BP still exposes lst_news directly:
	UPROPERTY(meta = (BindWidgetOptional)) UListView* IntelList = nullptr;

	// New CmdIntelPanel detail widgets:
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelNameText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelSourceText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelLocationText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelDateText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelMessageText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* IntelImage = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UButton* AudioButton = nullptr;

	// Optional movie surface container if present in BP
	UPROPERTY(meta = (BindWidgetOptional)) UWidget* mov_news = nullptr;

private:
	UCmpnScreen* Manager = nullptr;
	Starshatter* Stars = nullptr;
	Campaign* CampaignPtr = nullptr;

	double UpdateTime = 0.0;
	int32 StartSceneCountdown = 0;
	FString EventScene;

	ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_INTEL;

	UPROPERTY() UTexture2D* DefaultNewsTexture = nullptr;

private:
	UFUNCTION() void OnSaveClicked();

	void SetSelectedIntelData(UIntelListObject* Item);

	UFUNCTION() void OnPlayClicked();
	
	UFUNCTION()
	void OnNewsItemClicked(UObject* Item);

protected:
	FString ImagePath;
	FString AudioPath;

	void GetIntelImageFile(const FString& IntelImageName);
	void GetIntelAudioFile(const FString& IntelAudioName);

	UTexture2D* LoadTextureFromFile();
	FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);

protected:
	UPROPERTY()
	UCmdDlg* ParentCmdDlg = nullptr;
};