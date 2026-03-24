// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBBattery definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBBatteryWidget.generated.h"

class UTextBlock;
class UImage;
struct FS_OOBBattery;

/**
 * Battery UI Widget - represents one Battery under a Battalion
 */

UCLASS()
class STARSHATTERWARS_API UOOBBatteryWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
    // The BattleGroup data this widget represents
    UPROPERTY()
    FS_OOBBattery Data;
    
    // Bound UI elements
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;
	
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ListView binding override
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    void ShowUnitData();
    void SetHighlight(bool bHighlighted);
};