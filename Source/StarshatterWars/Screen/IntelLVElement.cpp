// +----------------------------------------------------------------------+
// | UIntelLVElement                                                      |
// +----------------------------------------------------------------------+

#include "IntelLVElement.h"
#include "Components/TextBlock.h"
#include "Components/CheckBox.h"
#include "Components/Border.h"

#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "MissionUIStyle.h"


void UIntelLVElement::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IntelList = Cast<UIntelListObject>(ListItemObject);
	if (!IntelList)
		return;

	if (NewsTitleText)
		NewsTitleText->SetText(FText::FromString((IntelList->NewsVisited ? TEXT("") : TEXT("NEW  |  ")) + IntelList->NewsTitle));

	if (NewsLocationText)
		NewsLocationText->SetText(FText::FromString(IntelList->NewsLocation));

	if (NewsDateText)
		NewsDateText->SetText(FText::FromString(IntelList->NewsDate));

	if (NewsSourceText)
		NewsSourceText->SetText(FText::FromString(IntelList->NewsSource));

	if (NewsVisited)
		NewsVisited->SetIsChecked(IntelList->NewsVisited);
	ApplySelectionVisual();
}

void UIntelLVElement::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	bRowSelected = bIsSelected;
	if (IntelList && NewsTitleText)
		NewsTitleText->SetText(FText::FromString((IntelList->NewsVisited ? TEXT("") : TEXT("NEW  |  ")) + IntelList->NewsTitle));
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
		SelectionBorder->SetBrushColor(FLinearColor(0.025f, 0.07f, 0.11f, 0.95f));
	}
}
UIntelReportList::UIntelReportList(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Retain the existing asset as the entry-class shell required by UListView in editor.
    // Its visual widget tree is built entirely by UIntelLVElement below.
    EntryWidgetClass = LoadClass<UIntelLVElement>(nullptr,
        TEXT("/Game/Screens/Operations/IntelLVElement.IntelLVElement_C"));
}

TSharedRef<SWidget> UIntelLVElement::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget || WidgetTree->RootWidget->GetFName() != TEXT("NativeIntelRow"))
    {
        USizeBox* Row = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NativeIntelRow"));
        Row->SetMinDesiredHeight(112.f);
        WidgetTree->RootWidget = Row;
        SelectionBorder = WidgetTree->ConstructWidget<UBorder>();
        SelectionBorder->SetPadding(FMargin(16.f, 12.f));
        Row->SetContent(SelectionBorder);
        UVerticalBox* Contents = WidgetTree->ConstructWidget<UVerticalBox>();
        SelectionBorder->SetContent(Contents);
        auto AddText = [this, Contents](int32 FontSize)->UTextBlock*
        {
            UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
            Text->SetFont(MissionUIStyle::GetRowFont(FontSize));
            Text->SetAutoWrapText(true);
            Text->SetColorAndOpacity(FLinearColor(0.85f, 0.93f, 1.f));
            Contents->AddChildToVerticalBox(Text)->SetPadding(FMargin(0.f, 2.f));
            return Text;
        };
        NewsVisited = nullptr;
        NewsTitleText = AddText(20);
        NewsSourceText = AddText(15);
        NewsLocationText = AddText(15);
        NewsDateText = AddText(14);
    }
    ApplySelectionVisual();
    return Super::RebuildWidget();
}
