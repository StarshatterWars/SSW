#include "FormListBox.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"

UFormListBox::UFormListBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsVariable = true;
}

void UFormListBox::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);

    ListView.Reset();
}

void UFormListBox::Clear()
{
    ItemsText.Reset();
    Items.Reset();
    SelectedIndex = INDEX_NONE;

    RefreshList();
}

void UFormListBox::AddItem(const FText& InText)
{
    ItemsText.Add(InText);
    Items.Add(MakeShared<FString>(InText.ToString()));

    RefreshList();
}

FText UFormListBox::GetItemText(int32 InIndex) const
{
    if (ItemsText.IsValidIndex(InIndex))
    {
        return ItemsText[InIndex];
    }
    return FText::GetEmpty();
}

void UFormListBox::SetSelectedIndex(int32 InIndex)
{
    if (InIndex < 0 || InIndex >= Items.Num())
    {
        SelectedIndex = INDEX_NONE;
        if (ListView.IsValid())
        {
            ListView->ClearSelection();
        }
        return;
    }

    SelectedIndex = InIndex;

    if (ListView.IsValid())
    {
        // Prevents selection callback recursion from becoming noisy:
        ListView->SetSelection(Items[InIndex], ESelectInfo::Direct);
        ListView->RequestScrollIntoView(Items[InIndex]);
    }

    OnSelectionChanged.Broadcast(SelectedIndex);
}

void UFormListBox::RebuildItemsFromText()
{
    Items.Reset(ItemsText.Num());
    for (const FText& T : ItemsText)
    {
        Items.Add(MakeShared<FString>(T.ToString()));
    }
}

void UFormListBox::RefreshList()
{
    if (ListView.IsValid())
    {
        ListView->RequestListRefresh();

        // Re-apply selection if possible:
        if (SelectedIndex != INDEX_NONE && Items.IsValidIndex(SelectedIndex))
        {
            ListView->SetSelection(Items[SelectedIndex], ESelectInfo::Direct);
        }
    }
}

TSharedRef<SWidget> UFormListBox::RebuildWidget()
{
    // Ensure Items exists if editor/serialization filled ItemsText:
    if (Items.Num() != ItemsText.Num())
    {
        RebuildItemsFromText();
    }

    SAssignNew(ListView, SListView<FItemPtr>)
        .ListItemsSource(&Items)
        .SelectionMode(ESelectionMode::Single)

        // IMPORTANT: Bind as UObject (no AsShared required)
        .OnGenerateRow(SListView<FItemPtr>::FOnGenerateRow::CreateUObject(
            this, &UFormListBox::GenerateRow))

        .OnSelectionChanged(SListView<FItemPtr>::FOnSelectionChanged::CreateUObject(
            this, &UFormListBox::HandleSelectionChanged));

    return ListView.ToSharedRef();
}

void UFormListBox::SynchronizeProperties()
{
    Super::SynchronizeProperties();

    // If selection was set before Slate existed, apply it now:
    if (ListView.IsValid())
    {
        RefreshList();
    }
}

TSharedRef<ITableRow> UFormListBox::GenerateRow(FItemPtr InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
    // Resolve display text:
    const FText DisplayText = InItem.IsValid()
        ? FText::FromString(*InItem)
        : FText::GetEmpty();

    // Keep it simple: Starshatter styling can be layered later.
    return SNew(STableRow<FItemPtr>, OwnerTable)
        .Padding(FMargin(RowPadding))
        [
            SNew(STextBlock)
                .Text(DisplayText)
        ];
}

void UFormListBox::HandleSelectionChanged(FItemPtr InItem, ESelectInfo::Type SelectInfo)
{
    // Find index of selected item:
    SelectedIndex = INDEX_NONE;

    if (InItem.IsValid())
    {
        for (int32 i = 0; i < Items.Num(); ++i)
        {
            if (Items[i] == InItem)
            {
                SelectedIndex = i;
                break;
            }
        }
    }

    OnSelectionChanged.Broadcast(SelectedIndex);
}
