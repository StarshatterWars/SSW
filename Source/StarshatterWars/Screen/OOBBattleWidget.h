// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBFleet definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBBattleWidget.generated.h"

class UTextBlock;
class UImage;
class UListView;
struct FS_OOBBattle;

/**
 * BattleGroup UI Widget - represents one BattleGroup inside a Fleet
 */

UCLASS()
class STARSHATTERWARS_API UOOBBattleWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
    // The BattleGroup data this widget represents
    UPROPERTY()
    FS_OOBBattle Data;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ExpandIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* ElementListView; // Units

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* ExpandedIconTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CollapsedIconTexture;

    UPROPERTY()
    bool bIsExpanded = false;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    void ToggleExpansion();
    void BuildChildren(const FS_OOBBattle& BattleDataStruct);
    void ShowUnitData();
    void SetHighlight(bool bHighlighted);
};