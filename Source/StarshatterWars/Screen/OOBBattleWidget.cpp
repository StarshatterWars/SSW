// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBBattleWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBBattleItem.h"
#include "OOBUnitItem.h"
#include "OperationsScreen.h"
#include "Components/ListView.h"  

void UOOBBattleWidget::NativeConstruct()
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

void UOOBBattleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBBattleWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBBattleItem* BattleData = Cast<UOOBBattleItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(BattleData->Data.Name));
        }

        Data = BattleData->Data;

        // Expand/collapse setup
        bIsExpanded = false;

        if (ElementListView) ElementListView->ClearListItems();

        BuildChildren(BattleData->Data);
    }
}

FReply UOOBBattleWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ToggleExpansion(); // << Expand or collapse when clicked
    ShowUnitData();
    return FReply::Handled();
}

void UOOBBattleWidget::ToggleExpansion()
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

void UOOBBattleWidget::BuildChildren(const FS_OOBBattle& BattleDataStruct)
{
    if (!ElementListView) return;

    // Fill Ship Elements
    for (const FS_OOBUnit& Ship : BattleDataStruct.Unit)
    {
        UE_LOG(LogTemp, Error, TEXT("Ship Found: %s"), *Ship.Name);
        UOOBUnitItem* UnitData = NewObject<UOOBUnitItem>(this);

        if (UnitData)
        {
            UnitData->Data = Ship;
            ElementListView->AddItem(UnitData);
        }
    }
}

void UOOBBattleWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBBattleWidget::SetHighlight(bool bHighlighted)
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