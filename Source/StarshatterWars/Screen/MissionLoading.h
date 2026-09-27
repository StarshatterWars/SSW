// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"

#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ListView.h"

#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "MissionLoading.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UMissionLoading : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CampaignNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SystemLocationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* RegionLocationText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionStartTimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* MissionNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CancelButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SelectButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* CancelButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* SituationButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* PackageButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* MapButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* WeaponsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GameTimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* CampaignTPlusText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWidgetSwitcher* MissionScreenSwitcher;

public:
	UFUNCTION()
	void GetSelectedMissionData();

protected:
	void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnSelectButtonClicked();
	UFUNCTION()
	void OnSelectButtonHovered();
	UFUNCTION()
	void OnSelectButtonUnHovered();
	
	UFUNCTION()
	void OnSituationButtonClicked();
	UFUNCTION()
	void OnSituationButtonHovered();
	UFUNCTION()
	void OnSituationButtonUnHovered();
	UFUNCTION()
	void OnPackageButtonClicked();
	UFUNCTION()
	void OnPackageButtonHovered();
	UFUNCTION()
	void OnPackageButtonUnHovered();

	UFUNCTION()
	void OnMapButtonClicked();
	UFUNCTION()
	void OnMapButtonHovered();
	UFUNCTION()
	void OnMapButtonUnHovered();

	UFUNCTION()
	void OnWeaponsButtonClicked();
	UFUNCTION()
	void OnWeaponsButtonHovered();
	UFUNCTION()
	void OnWeaponsButtonUnHovered();

	//UFUNCTION()
	//void OnCancelButtonClicked();
	UFUNCTION()
	void OnCancelButtonHovered();
	UFUNCTION()
	void OnCancelButtonUnHovered();

	UPROPERTY()
	int SelectedMission;
	UPROPERTY()
	int SelectedCampaign;
private:
	void HandleUniverseSecondTick(uint64 UniverseSecondsNow);
	void HandleUniverseMinuteTick(uint64 UniverseSecondsNow);
	void HandleCampaignTPlusChanged(uint64 UniverseSecondsNow, uint64 TPlusSeconds);
};
