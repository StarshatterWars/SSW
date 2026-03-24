// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RosterViewObject.h"

#include "Blueprint/IUserObjectListEntry.h"
#include "SSWGameInstance.h"
#include "Components/ListView.h"
#include "CombatGroupObject.h"

#include "RosterTVElement.generated.h"

class UOperationsScreen;
class UTextBlock;
class UButton;
class UImage;

class UObject;

class UOOBForceItem;
class UOOBFleetItem;
class UOOBCarrierGroupItem;
class UOOBBattleItem;
class UOOBDestroyerItem;
class UOOBWingItem;
class UOOBUnitItem;
class UOOBSquadronItem;
class UOOBFighterSquadronItem;
class UOOBFighterUnit;
class UOOBBattalion;
class UOOBCivilianItem;
class UOOBBatteryItem;

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API URosterTVElement : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* NameText;  // TextBlock for displaying the name of the item

    UOperationsScreen* OpsScreen;

    // Whether this Fleet is expanded (showing Battles, Carriers, DesRons)
    UPROPERTY()
    bool bIsExpanded = false;

    // Tree depth level (0 = Force, 1 = Fleet, etc.)
    UPROPERTY()
    int32 IndentLevel = 0;
	
    // UI: Expand/collapse icon (plus/minus)
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ExpandIcon;

	 // UI: ListView holding child Fleets
    UPROPERTY(meta = (BindWidgetOptional))
    UListView* FleetListView;

    // Icons for expand/collapse states
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* ExpandedIconTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CollapsedIconTexture;
	
protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual void NativeConstruct() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Force Widget Variables")
	FString Name;

};
