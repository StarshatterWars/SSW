// +----------------------------------------------------------------------+
// | UIntelLVElement                                                      |
// +----------------------------------------------------------------------+

#include "IntelLVElement.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"

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