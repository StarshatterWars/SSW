// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBFighterElementWidget.h"
#include "OOBFighterUnit.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OperationsScreen.h"
#include "GameStructs.h" // FS_OOBFleet definition

void UOOBFighterElementWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UOOBFighterElementWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBFighterElementWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBFighterUnit* UnitData = Cast<UOOBFighterUnit>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(FString::Printf(TEXT("%i x %s"), UnitData->Data.Count, *UnitData->Data.Design)));
        }
        Data = UnitData->Data;
    }
}

FReply UOOBFighterElementWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowElementData(); // << Expand or collapse when clicked
    return FReply::Handled();
}

void UOOBFighterElementWidget::ShowElementData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this); 
    SSWInstance->SetActiveElement(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayElementChanged = true;
}

void UOOBFighterElementWidget::SetHighlight(bool bHighlighted)
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





