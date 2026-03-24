// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBBatteryWidget.h"
#include "OOBBatteryItem.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OperationsScreen.h"
#include "GameStructs.h" // FS_OOBFleet definition

void UOOBBatteryWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UOOBBatteryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBBatteryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBBatteryItem* BatteryData = Cast<UOOBBatteryItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(BatteryData->Data.Name));
        }

        Data = BatteryData->Data;
    }
}

FReply UOOBBatteryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowUnitData();
    return FReply::Handled();
}

void UOOBBatteryWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBBatteryWidget::SetHighlight(bool bHighlighted)
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

