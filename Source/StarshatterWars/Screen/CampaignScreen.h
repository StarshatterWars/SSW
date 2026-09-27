// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "GameStructs.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Texture2D.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "CampaignScreen.generated.h"

/**
 *
 */
UCLASS()
class STARSHATTERWARS_API UCampaignScreen : public UUserWidget
{
	GENERATED_BODY()
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayerNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GameTimeText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CampaignNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DescriptionText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* SituationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders1Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders2Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders3Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Orders4Text;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LocationSystemText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* LocationRegionText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CampaignStartTimeText;


	UPROPERTY(meta = (BindWidgetOptional))
	class UImage* CampaignImage;

	UPROPERTY(meta = (BindWidgetOptional))
	class UComboBoxString* CampaignSelectDD;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* RestartButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CancelButtonText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* PlayButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* RestartButton;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* CancelButton;
	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* HoverSound;

	UPROPERTY(EditAnywhere, Category = "UI Sound")
	USoundBase* AcceptSound;


protected:
	void NativeConstruct() override;

	UTexture2D* LoadTextureFromFile();
	FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);

	// UI selection state
	int32 Selected = 0;
	FName PickedRowName = NAME_None;
	TArray<FName> CampaignRowNamesByOptionIndex;
	TArray<int32> CampaignIndexByOptionIndex;

	UFUNCTION()
	void OnPlayButtonClicked();
	UFUNCTION()
	void OnPlayButtonHovered();
	UFUNCTION()
	void OnPlayButtonUnHovered();
	UFUNCTION()
	void OnRestartButtonClicked();
	UFUNCTION()
	void OnRestartButtonHovered();
	UFUNCTION()
	void OnRestartButtonUnHovered();
	//UFUNCTION()
	//void OnCancelButtonClicked();
	UFUNCTION()
	void OnCancelButtonHovered();
	UFUNCTION()
	void OnCancelButtonUnHovered();

	UFUNCTION()
	void SetCampaignDDList();

	UFUNCTION()
	void SetSelectedData(int selected);

	UFUNCTION()
	void OnSetSelected(FString dropDownInt, ESelectInfo::Type type);

	UFUNCTION()
	void GetCampaignImageFile(int selected);
	UFUNCTION()
	void PlayUISound(UObject* WorldContext, USoundBase* UISound);
	UFUNCTION()
	bool DoesSelectedCampaignSaveExist() const;

	UFUNCTION()
	void UpdateCampaignButtons();

	UPROPERTY()
	FString ImagePath;
};