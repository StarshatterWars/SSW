// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBStationWidget.h"
#include "OOBStationItem.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBUnitItem.h"
#include "OOBStationItem.h"
#include "OperationsScreen.h"
#include "GameStructs.h" // FS_OOBFleet definition 

void UOOBStationWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UOOBStationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBStationWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBStationItem* StationData = Cast<UOOBStationItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(StationData->Data.Name));
        }
        Data = StationData->Data;
    }
}

FReply UOOBStationWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowElementData(); // << Expand or collapse when clicked
    return FReply::Handled();
}

void UOOBStationWidget::ShowElementData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBStationWidget::SetHighlight(bool bHighlighted)
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


