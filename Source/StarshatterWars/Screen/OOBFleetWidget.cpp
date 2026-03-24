// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBFleetWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBFleetItem.h"
#include "OOBBattleItem.h"
#include "OOBMinefieldItem.h"
#include "OOBDestroyerItem.h"
#include "OOBCarrierGroupItem.h"
#include "OperationsScreen.h"
#include "Components/ListView.h"

void UOOBFleetWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ExpandIcon)
    {
        ExpandIcon->SetVisibility(ESlateVisibility::Visible);
        ExpandIcon->SetBrushFromTexture(CollapsedIconTexture);
    }

    SetVisible(bIsExpanded);
}

void UOOBFleetWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBFleetWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBFleetItem* FleetData = Cast<UOOBFleetItem>(ListItemObject))
    {
        if (NameText)
        {
           NameText->SetText(FText::FromString(FleetData->Data.Name));
        }

        bIsExpanded = false;
        
        Data = FleetData->Data;

        if (BattleListView) BattleListView->ClearListItems();
        if (CarrierListView) CarrierListView->ClearListItems();
        if (DestroyerListView) DestroyerListView->ClearListItems();
        if (MinefieldListView) MinefieldListView->ClearListItems();

        BuildChildren(FleetData->Data);
    }
}

FReply UOOBFleetWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ShowUnitData();
    ToggleExpansion();
    return FReply::Handled();
}

void UOOBFleetWidget::ToggleExpansion()
{
    bIsExpanded = !bIsExpanded;

    if (ExpandIcon)
    {
        ExpandIcon->SetBrushFromTexture(bIsExpanded ? ExpandedIconTexture : CollapsedIconTexture);
    }

    SetVisible(bIsExpanded);
}

void UOOBFleetWidget::BuildChildren(const FS_OOBFleet& FleetDataStruct)
{
    if (!BattleListView || !CarrierListView || !DestroyerListView || !MinefieldListView) {
        UE_LOG(LogTemp, Error, TEXT("Fleet ListViews are notxvalid!"));
            return;
    }

    // Fill Battles
    for (const FS_OOBBattle& Battle : FleetDataStruct.Battle)
    {
        UE_LOG(LogTemp, Error, TEXT("Battle Group Found: %s"), *Battle.Name);
        UOOBBattleItem* BattleData = NewObject<UOOBBattleItem>(this);
        if (BattleData)
        {
            BattleData->Data = Battle;
            BattleListView->AddItem(BattleData);
        }
    }

    // Fill Carriers
    for (const FS_OOBCarrier& Carrier : FleetDataStruct.Carrier)
    {
        UE_LOG(LogTemp, Error, TEXT("Carrier Found: %s"), *Carrier.Name);
        UOOBCarrierGroupItem* CarrierData = NewObject<UOOBCarrierGroupItem>(this);
        if (CarrierData)
        {
            CarrierData->Data = Carrier;
            CarrierListView->AddItem(CarrierData);
        }
    }

    // Fill DESRONs
    for (const FS_OOBDestroyer& Desron : FleetDataStruct.Destroyer)
    {
        UE_LOG(LogTemp, Error, TEXT("Desron Found: %s"), *Desron.Name);
        UOOBDestroyerItem* DestroyerData = NewObject<UOOBDestroyerItem>(this);
        if (DestroyerData)
        {
            DestroyerData->Data = Desron;
            DestroyerListView->AddItem(DestroyerData);
        }    
    }

    // Fill Minefields
    for (const FS_OOBMinefield& Minefield : FleetDataStruct.Minefield)
    {
        UE_LOG(LogTemp, Error, TEXT("Minefield Found: %s"), *Minefield.Name);
        UOOBMinefieldItem* MinefieldData = NewObject<UOOBMinefieldItem>(this);
        if (MinefieldData)
        {
            MinefieldData->Data = Minefield;
            MinefieldListView->AddItem(MinefieldData);
        }
    }
}

void UOOBFleetWidget::SetVisible(bool bIsVisible)
{
    if (BattleListView) BattleListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (CarrierListView) CarrierListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (DestroyerListView) DestroyerListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (MinefieldListView) MinefieldListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBFleetWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBFleetWidget::SetHighlight(bool bHighlighted)
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
