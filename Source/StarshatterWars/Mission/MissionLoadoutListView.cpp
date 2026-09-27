#include "MissionLoadoutListView.h"

void UMissionLoadoutListView::SetEntryWidgetClassPublic(TSubclassOf<UUserWidget> InEntryWidgetClass)
{
    EntryWidgetClass = InEntryWidgetClass;
}