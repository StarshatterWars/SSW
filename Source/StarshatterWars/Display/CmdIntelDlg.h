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
// |   - Handle audio playback                                            |
// |   - Route command screen navigation                                  |
// +----------------------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "BaseScreen.h"
#include "CmdDlg.h"
#include "GameStructs.h"
#include "CmdIntelDlg.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UListView;
class UWidget;
class UIntelListObject;
class UTexture2D;
class USoundBase;

class Starshatter;
class Campaign;
class CombatEvent;
class UCmpnScreen;

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
	void RefreshIntelData();

private:
	void BindFormWidgets();
	void ExecFrame();
	void RebuildNewsListIfCampaignChanged();
	void AppendNewEventsIfAny();
	void AutoScrollToFirstUnreadIfNeeded();

	void ClearNewsDetails();
	void SetSelectedIntelData(UIntelListObject* Item);

	void ShowMovie();
	void HideMovie();

	void SetModeAndRoute(ECOMMAND_MODE InMode);

	CombatEvent* GetSelectedEvent(int32& OutSelectedIndex) const;

	FString GetIntelImageAssetPath(const FString& IntelImageName) const;
	void GetIntelAudioFile(const FString& IntelAudioName);

	UTexture2D* LoadTextureFromAssetPath(const FString& AssetPath) const;
	FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);
	void AutoSelectFirstItemIfNeeded();

private:
	UPROPERTY(meta = (BindWidgetOptional)) UButton* btn_save = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UButton* AudioButton = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UListView* IntelList = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelNameText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelSourceText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelLocationText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelDateText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UTextBlock* IntelMessageText = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) UImage* IntelImage = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) UWidget* mov_news = nullptr;

private:
	UCmpnScreen* Manager = nullptr;
	Starshatter* Stars = nullptr;
	Campaign* CampaignPtr = nullptr;

	double UpdateTime = 0.0;
	int32 StartSceneCountdown = 0;
	FString EventScene;

	ECOMMAND_MODE Mode = ECOMMAND_MODE::MODE_INTEL;

	UPROPERTY()
	UTexture2D* DefaultNewsTexture = nullptr;

protected:
	FString AudioPath;

protected:
	UPROPERTY()
	UCmdDlg* ParentCmdDlg = nullptr;

private:
	UFUNCTION() void OnSaveClicked();
	UFUNCTION() void OnPlayClicked();

	void OnNewsSelectionChanged(UObject* Item);
};