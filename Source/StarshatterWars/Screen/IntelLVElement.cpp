// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "IntelLVElement.h"
#include "OperationsScreen.h"
#include "SSWGameInstance.h"



void UIntelLVElement::NativeConstruct()
{
	Super::NativeConstruct(); 
	if (NewsFeedButton) {
		NewsFeedButton->OnClicked.AddDynamic(this, &UIntelLVElement::OnNewsFeedButtonClicked);
	}
}

void UIntelLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IntelList = Cast<UIntelListObject>(ListItemObject);

	if (!IntelList) {
		return;
	}

	if (UListView* OwningListView = Cast<UListView>(GetOwningListView())) {
		NewsfeedId = OwningListView->GetIndexForItem(IntelList);
	}

	if (NewsTitleText)
	{
		NewsTitleText->SetText(FText::FromString(IntelList->NewsTitle));
	}
	if (NewsLocationText)
	{
		NewsLocationText->SetText(FText::FromString(IntelList->NewsLocation));
	}
	if (NewsDateText)
	{
		NewsDateText->SetText(FText::FromString(IntelList->NewsDate));
	}
	if (NewsSourceText)
	{
		NewsSourceText->SetText(FText::FromString(IntelList->NewsSource));
	}
}

void UIntelLVElement::OnNewsFeedButtonClicked()
{
	UE_LOG(LogTemp, Log, TEXT("Selected Newsfeed Nr: %i"), NewsfeedId);
	SetNewsfeedInfo();
}

void UIntelLVElement::SetNewsfeedInfo()
{
	USSWGameInstance* SSWInstance = (USSWGameInstance*)GetGameInstance();
	SSWInstance->SetSelectedActionNr(NewsfeedId);
	SSWInstance->ActionSelectionChanged = true;
}
