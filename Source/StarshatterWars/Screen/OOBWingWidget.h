// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBWing definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBWingWidget.generated.h"

class UTextBlock;
class UImage;
class UListView;
struct FS_OOBWing;

/**
 * Wing UI Widget - represents a Wing under a Carrier, expandable into Squadrons
 */

UCLASS()
class STARSHATTERWARS_API UOOBWingWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
    UPROPERTY()
    FS_OOBWing Data;

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ExpandIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* AttackListView; // Squadrons
    
    UPROPERTY(meta = (BindWidgetOptional))
    UListView* InterceptorListView; // Interceptor Squadron

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* FighterListView; // Fighter Squadron

     UPROPERTY(meta = (BindWidgetOptional))
    UListView* LandingListView; // Landing Craft Squadron

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
    void BuildChildren(const FS_OOBWing& WingDataStruct);
 
    void SetVisible(bool bIsVisible);
    void ShowUnitData();
    void SetHighlight(bool bHighlighted);
};