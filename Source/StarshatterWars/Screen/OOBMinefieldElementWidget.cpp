// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBMinefieldElementWidget.h"
#include "Components/TextBlock.h"
#include "OOBMinefieldElement.h"
#include "Components/Image.h"
#include "OperationsScreen.h"
#include "GameStructs.h" // FS_OOB Minefield Element definition

void UOOBMinefieldElementWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UOOBMinefieldElementWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}
void UOOBMinefieldElementWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBMinefieldElement* UnitData = Cast<UOOBMinefieldElement>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(FString::Printf(TEXT("%i x %s"), UnitData->Data.Count, *UnitData->Data.Design)));
        }
        Data = UnitData->Data;
    }
}

FReply UOOBMinefieldElementWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowElementData(); // << Expand or collapse when clicked
    return FReply::Handled();
}

void UOOBMinefieldElementWidget::ShowElementData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveElement(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayElementChanged = true;
}

void UOOBMinefieldElementWidget::SetHighlight(bool bHighlighted)
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



