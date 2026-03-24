// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Blueprint/UserWidget.h"
#include "SSWGameInstance.h"
#include "TimerSubsystem.h"
#include "CampaignLoading.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UCampaignLoading : public UUserWidget
{
	GENERATED_BODY()
	
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* PlayerNameText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* CampaignNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* LoadButton;

protected:
	void NativeConstruct() override;
	//UFUNCTION()
	//void OnLoadButtonClicked();
	
private:

};
