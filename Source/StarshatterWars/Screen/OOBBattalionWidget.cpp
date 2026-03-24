// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBBattalionWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBBattalion.h"
#include "OOBStationItem.h"
#include "OOBStarbaseItem.h"
#include "OOBBatteryItem.h"
#include "OperationsScreen.h"
#include "Components/ListView.h"

void UOOBBattalionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ExpandIcon)
    {
        ExpandIcon->SetVisibility(ESlateVisibility::Visible);
        ExpandIcon->SetBrushFromTexture(CollapsedIconTexture);
    }
    
    SetVisible(bIsExpanded);
}

void UOOBBattalionWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBBattalionWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBBattalion* BattalionData = Cast<UOOBBattalion>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(BattalionData->Data.Name));
        }
        Data = BattalionData->Data;

        bIsExpanded = false;

        if (BatteryListView) BatteryListView->ClearListItems();
        if (StarbaseListView) StarbaseListView->ClearListItems();
        if (StationListView) StationListView->ClearListItems();

        BuildChildren(BattalionData->Data);
    }
}

FReply UOOBBattalionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ToggleExpansion();
    ShowUnitData();
    return FReply::Handled();
}

void UOOBBattalionWidget::ToggleExpansion()
{
    bIsExpanded = !bIsExpanded;

    if (ExpandIcon)
    {
        ExpandIcon->SetBrushFromTexture(bIsExpanded ? ExpandedIconTexture : CollapsedIconTexture);
    }

    SetVisible(bIsExpanded);
}

void UOOBBattalionWidget::BuildChildren(const FS_OOBBattalion& BattalionDataStruct)
{
    if (!BatteryListView || !StarbaseListView || !StationListView) {
        UE_LOG(LogTemp, Error, TEXT("Battalion ListViews are notxvalid!"));
        return;
    }

    // Fill Batteries
    for (const FS_OOBBattery& Battery : BattalionDataStruct.Battery)
    {
        UE_LOG(LogTemp, Error, TEXT("Battery Found: %s"), *Battery.Name);
        UOOBBatteryItem* BatteryData = NewObject<UOOBBatteryItem>(this);
        if (BatteryData)
        {
            BatteryData->Data = Battery;
            BatteryListView->AddItem(BatteryData);
        }
    }

    // Fill Starbase
    for (const FS_OOBStarbase& Starbase : BattalionDataStruct.Starbase)
    {
        UE_LOG(LogTemp, Error, TEXT("Starbase Found: %s"), *Starbase.Name);
        UOOBStarbaseItem* StarbaseData = NewObject<UOOBStarbaseItem>(this);
        if (StarbaseData)
        {
            StarbaseData->Data = Starbase;
            StarbaseListView->AddItem(StarbaseData);
        }
    }

    // Fill Station
    for (const FS_OOBStation& Station : BattalionDataStruct.Station)
    {
        UE_LOG(LogTemp, Error, TEXT("Desron Found: %s"), *Station.Name);
        UOOBStationItem* StationData = NewObject<UOOBStationItem>(this);
        if (StationData)
        {
            StationData->Data = Station;
            StationListView->AddItem(StationData);
        }
    }
}

void UOOBBattalionWidget::SetVisible(bool bIsVisible) 
{
    if (BatteryListView) BatteryListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (StarbaseListView) StarbaseListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (StationListView) StationListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBBattalionWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBBattalionWidget::SetHighlight(bool bHighlighted)
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


