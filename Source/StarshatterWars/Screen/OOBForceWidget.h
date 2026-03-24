// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBForce definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBForceWidget.generated.h"

class UTextBlock;
class UImage;
class UOOBForceItem; // The Data Object class holding FS_OOBForce
class UListView;
struct FS_OOBForce;

/**
 * Force UI Widget - represents a Force at the top of the Order of Battle
 * Expands into a list of Fleets
 */

 // Force Clicked Event
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FForceClickedDelegate, UOOBForceWidget*, ClickedForce);

UCLASS()
class STARSHATTERWARS_API UOOBForceWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
 public:
    // Bound UI elements
    
    UPROPERTY()
    FS_OOBForce Data;
    
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* ExpandIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    UListView* FleetListView; // List of Fleets inside Force

     UPROPERTY(meta = (BindWidgetOptional))
    UListView* BattalionListView; // List of Battalions inside Force

    // Expand/Collapse textures
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* ExpandedIconTexture;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* CollapsedIconTexture;

    // Expand/collapse state
    UPROPERTY()
    bool bIsExpanded = false;

    // Expand/collapse state
    UPROPERTY()
    bool bClickBound = false;

    // Delegate to broadcast when clicked
    UPROPERTY(BlueprintAssignable)
    FForceClickedDelegate OnForceClicked;

    // Tree depth level
    UPROPERTY()
    int32 IndentLevel = 0;

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ListView binding override
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
    
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Toggle expansion of child ListView
    void ToggleExpansion();
    void SetVisible(bool bIsVisible);
    // Build Fleets under this Force
    void BuildChildren(const FS_OOBForce& ForceDataStruct);
    void ShowUnitData();
    void SetHighlight(bool bHighlighted);
};