// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Checkbox.h"
#include "IntelListObject.h"
#include "Blueprint/IUserObjectListEntry.h"

#include "SSWGameInstance.h"
#include "IntelLVElement.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UIntelLVElement : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NewsTitleText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NewsLocationText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NewsDateText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NewsSourceText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* NewsInfoText;
	UPROPERTY(meta = (BindWidgetOptional))
	class UCheckBox* NewsVisited;
	UPROPERTY(meta = (BindWidgetOptional))
	class UButton* NewsFeedButton;
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission LV Variables")
	int32 NewsfeedId;

protected:
	void NativeConstruct() override;
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	
	UFUNCTION()
	void OnNewsFeedButtonClicked();

	UPROPERTY()
	UIntelListObject* IntelList;
private:
	void SetNewsfeedInfo();
};
