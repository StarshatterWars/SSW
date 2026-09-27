// +----------------------------------------------------------------------+
// | UIntelLVElement                                                      |
// +----------------------------------------------------------------------+

#include "IntelLVElement.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/Border.h"

void UIntelLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IntelList = Cast<UIntelListObject>(ListItemObject);
	if (!IntelList)
		return;

	if (NewsTitleText)
		NewsTitleText->SetText(FText::FromString(IntelList->NewsTitle));

	if (NewsLocationText)
		NewsLocationText->SetText(FText::FromString(IntelList->NewsLocation));

	if (NewsDateText)
		NewsDateText->SetText(FText::FromString(IntelList->NewsDate));

	if (NewsSourceText)
		NewsSourceText->SetText(FText::FromString(IntelList->NewsSource));

	if (NewsVisited)
		NewsVisited->SetIsChecked(IntelList->NewsVisited);
}

void UIntelLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	bRowSelected = bIsSelected;
	ApplySelectionVisual();
}

void UIntelLVElement::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	IntelList = nullptr;
	bRowSelected = false;
	ApplySelectionVisual();
}
void UIntelLVElement::ApplySelectionVisual()
{
	if (!SelectionBorder)
	{
		return;
	}

	if (bRowSelected)
	{
		SelectionBorder->SetBrushColor(FLinearColor(0.0f, 0.7f, 1.0f, 0.35f));
	}
	else
	{
		SelectionBorder->SetBrushColor(FLinearColor(0, 0, 0, 0));
	}
}