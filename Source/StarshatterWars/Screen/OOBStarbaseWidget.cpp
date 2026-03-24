// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBStarbaseWidget.h"
#include "OOBStarbaseItem.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBUnitItem.h"
#include "OperationsScreen.h"
#include "GameStructs.h" // FS_OOBFleet definition

void UOOBStarbaseWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UOOBStarbaseWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBStarbaseWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBStarbaseItem* StarbaseData = Cast<UOOBStarbaseItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(StarbaseData->Data.Name));
        }
        Data = StarbaseData->Data;
    }
}

FReply UOOBStarbaseWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowElementData(); // << Expand or collapse when clicked
    return FReply::Handled();
}

void UOOBStarbaseWidget::ShowElementData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBStarbaseWidget::SetHighlight(bool bHighlighted)
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

