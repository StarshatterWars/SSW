// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "OOBWingWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "OOBWingItem.h"
#include "OOBAttackItem.h"
#include "OOBFighterSquadronItem.h"
#include "OOBInterceptorItem.h"
#include "OOBLandingItem.h"
#include "OperationsScreen.h"
#include "Components/ListView.h"

void UOOBWingWidget::NativeConstruct()
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

void UOOBWingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    if (SSWInstance->GetActiveWidget() == this) {
        SetHighlight(true);
    }
    else {
        SetHighlight(false);
    }
}

void UOOBWingWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    if (UOOBWingItem* WingData = Cast<UOOBWingItem>(ListItemObject))
    {
        if (NameText)
        {
            NameText->SetText(FText::FromString(WingData->Data.Name));
        }

        Data = WingData->Data;
        // Expand/collapse setup
        bIsExpanded = false;

        if (AttackListView) { AttackListView->ClearListItems(); }
        if (FighterListView) { FighterListView->ClearListItems(); }
        if (InterceptorListView) { InterceptorListView->ClearListItems(); }
        if (LandingListView) { LandingListView->ClearListItems(); }

        BuildChildren(WingData->Data);
    }
}


FReply UOOBWingWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    ToggleExpansion(); // << Expand or collapse when clicked
    ShowUnitData();
    return FReply::Handled();
}

void UOOBWingWidget::ToggleExpansion()
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

void UOOBWingWidget::BuildChildren(const FS_OOBWing& WingDataStruct)
{
    if (!AttackListView || !FighterListView || !InterceptorListView || !LandingListView) {
        UE_LOG(LogTemp, Error, TEXT("Wing ListViews are notxvalid!"));
        return;
    }

    // Fill Attack Squadrons
    for (const FS_OOBAttack& Attack : WingDataStruct.Attack)
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
    for (const FS_OOBFighter& Fighter : WingDataStruct.Fighter)
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
    for (const FS_OOBIntercept& Interceptor : WingDataStruct.Intercept)
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
    for (const FS_OOBLanding& Landing : WingDataStruct.Landing)
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
void UOOBWingWidget::SetVisible(bool bIsVisible) 
{
    if (AttackListView) AttackListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (FighterListView) FighterListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (InterceptorListView) InterceptorListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (LandingListView) LandingListView->SetVisibility(bIsVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UOOBWingWidget::ShowUnitData()
{
    USSWGameInstance* SSWInstance = Cast<USSWGameInstance>(GetGameInstance());
    SSWInstance->SetActiveWidget(this);
    SSWInstance->SetActiveUnit(true, Data.Name, Data.Empire, Data.Type, Data.Location);
    SSWInstance->bIsDisplayUnitChanged = true;
}

void UOOBWingWidget::SetHighlight(bool bHighlighted)
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



