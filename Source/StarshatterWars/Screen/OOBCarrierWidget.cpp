// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBCarrierWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBCarrierGroupItem.h"
#include "OOBWingItem.h"
#include "OOBUnitItem.h"
#include "OOBAttackItem.h"
#include "OOBFighterSquadronItem.h"
#include "OOBInterceptorItem.h"
#include "OOBLandingItem.h"
#include "OperationsScreen.h"
#include "Components/ListView.h" 

void UOOBCarrierWidget::NativeConstruct()
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

    SetVisible(bIsExpanded);
}

void UOOBCarrierWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBCarrierWidget::SetVisible(bool bIsVisible) {
    if (ElementListView) ElementListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (WingListView) WingListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (AttackListView) AttackListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (FighterListView) FighterListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (InterceptorListView) InterceptorListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (LandingListView) LandingListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBCarrierWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBCarrierGroupItem* CarrierData = Cast<UOOBCarrierGroupItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(CarrierData->Data.Name));
        }

        Data = CarrierData->Data;

        // Expand/collapse setup
        bIsExpanded = false;

        if (ElementListView) ElementListView->ClearListItems();
        if (WingListView) WingListView->ClearListItems();

        BuildChildren(CarrierData->Data);
    }
}


FReply UOOBCarrierWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ToggleExpansion(); // << Expand or collapse when clicked
    ShowUnitData();
    return FReply::Handled();
}

void UOOBCarrierWidget::ToggleExpansion()
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

    SetVisible(bIsExpanded);
}

void UOOBCarrierWidget::BuildChildren(const FS_OOBCarrier& CarrierDataStruct)
{
    if (!ElementListView || !WingListView) {
        UE_LOG(LogTemp, Error, TEXT("Carrier ListViews are notxvalid!"));
        return;
    }

    if (ElementListView) { ElementListView->ClearListItems(); }
    if (WingListView) { WingListView->ClearListItems(); }
    if (AttackListView) { AttackListView->ClearListItems(); }
    if (FighterListView) { FighterListView->ClearListItems(); }
    if (InterceptorListView) { InterceptorListView->ClearListItems(); }
    if (LandingListView) { LandingListView->ClearListItems(); }

    // Fill Wings
    for (const FS_OOBWing& Wing : CarrierDataStruct.Wing)
    {
        UE_LOG(LogTemp, Error, TEXT("Wing Found: %s"), *Wing.Name);
        UOOBWingItem* WingData = NewObject<UOOBWingItem>(this);
        if (WingData)
        {
            WingData->Data = Wing;
            WingListView->AddItem(WingData);
        }
    }

    // Fill Ship Elements
    for (const FS_OOBUnit& Ship : CarrierDataStruct.Unit)
    {
        UE_LOG(LogTemp, Error, TEXT("Ship Found: %s"), *Ship.Name);
        UOOBUnitItem* UnitData = NewObject<UOOBUnitItem>(this);
        
        if (UnitData)
        {
            UnitData->Data = Ship;
            ElementListView->AddItem(UnitData);
        }
    }

    // Fill Attack Squadrons
    for (const FS_OOBAttack& Attack : CarrierDataStruct.Attack)
    {
        UE_LOG(LogTemp, Error, TEXT("Attack Squadron Group Found: %s"), *Attack.Name);

        UOOBAttackItem* AttackData = NewObject<UOOBAttackItem>(this);
        if (AttackData)
        {
            AttackData->Data = Attack;
            AttackListView->AddItem(AttackData);
        }
    }

    // Fill Fighters
    for (const FS_OOBFighter& Fighter : CarrierDataStruct.Fighter)
    {
        UE_LOG(LogTemp, Error, TEXT("Fighter Squadron Group Found: %s"), *Fighter.Name);

        UOOBFighterSquadronItem* FighterData = NewObject<UOOBFighterSquadronItem>(this);
        if (FighterData)
        {
            FighterData->Data = Fighter;
            FighterListView->AddItem(FighterData);
        }
    }

    // Fill Interceptors
    for (const FS_OOBIntercept& Interceptor : CarrierDataStruct.Intercept)
    {
        UE_LOG(LogTemp, Error, TEXT("Inteceptor Squadron Group Found: %s"), *Interceptor.Name);

        UOOBInterceptorItem* InterceptorData = NewObject<UOOBInterceptorItem>(this);
        if (InterceptorData)
        {
            InterceptorData->Data = Interceptor;
            InterceptorListView->AddItem(InterceptorData);
        }
    }

    // Fill Interceptors
    for (const FS_OOBLanding& Landing : CarrierDataStruct.Landing)
    {
        UE_LOG(LogTemp, Error, TEXT("Landing Squadron Group Found: %s"), *Landing.Name);

        UOOBLandingItem* LandingData = NewObject<UOOBLandingItem>(this);
        if (LandingData)
        {
            LandingData->Data = Landing;
            LandingListView->AddItem(LandingData);
        }
    }
}

void UOOBCarrierWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBCarrierWidget::SetHighlight(bool bHighlighted)
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


