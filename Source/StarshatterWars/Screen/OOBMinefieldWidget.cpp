// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBMinefieldWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBMinefieldItem.h"
#include "OOBMinefieldElement.h"
#include "OperationsScreen.h"
#include "Components/ListView.h"

void UOOBMinefieldWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ExpandIcon)
    {
        ExpandIcon->SetVisibility(ESlateVisibility::Visible);

        if (bIsExpanded)
        {
            ExpandIcon->SetBrushFromTexture(ExpandedIconTexture); // Expanded (-)
        }
        else
        {
            ExpandIcon->SetBrushFromTexture(CollapsedIconTexture); // Collapsed (+)
        }
    }

    if (ElementListView) ElementListView->SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBMinefieldWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBMinefieldWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBMinefieldItem* MinefieldData = Cast<UOOBMinefieldItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(MinefieldData->Data.Name));
        }

        Data = MinefieldData->Data;
        // Expand/collapse setup
        bIsExpanded = false;

        if (ElementListView) ElementListView->ClearListItems();

        BuildChildren(MinefieldData->Data);
    }
}


FReply UOOBMinefieldWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ToggleExpansion(); // << Expand or collapse when clicked
    ShowUnitData();
    return FReply::Handled();
}

void UOOBMinefieldWidget::ToggleExpansion()
{
    bIsExpanded = !bIsExpanded;

    if (ExpandIcon)
    {
        if (bIsExpanded)
        {
            ExpandIcon->SetBrushFromTexture(ExpandedIconTexture);
        }
        else
        {
            ExpandIcon->SetBrushFromTexture(CollapsedIconTexture);
        }
    }

    if (ElementListView) ElementListView->SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBMinefieldWidget::BuildChildren(const FS_OOBMinefield& MinefieldDataStruct)
{
    if (!ElementListView) {
        UE_LOG(LogTemp, Error, TEXT("Minefield Unit ListView is notxvalid!"));
        return;
    }

    ElementListView->ClearListItems();

    // Fill Units
    for (const FS_OOBMinefieldUnit& Element : MinefieldDataStruct.Unit)
    {
        UE_LOG(LogTemp, Error, TEXT("Mines Found: %s"), *Element.Name);
        UOOBMinefieldElement* MinefieldData = NewObject<UOOBMinefieldElement>(this);
        if (MinefieldData)
        {
            MinefieldData->Data = Element;
            ElementListView->AddItem(MinefieldData);
        }
    }
}

void UOOBMinefieldWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBMinefieldWidget::SetHighlight(bool bHighlighted)
{
    if (NameText)
    {
        NameText->SetColorAndOpacity(
            bHighlighted
            ? FSlateColor(FLinearColor(0.2f, 0.8f, 1.0f))  // Cyan or highlight color
            : FSlateColor(FLinearColor::White)            // Default color
        );
    }
}
