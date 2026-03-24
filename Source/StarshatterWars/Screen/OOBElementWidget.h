// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBForce definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBElementWidget.generated.h"

class UTextBlock;
class UImage;
class UOOBUnitItem; // The Data Object class holding FS_OOBForce
class UListView;
struct FS_OOBUnit;
/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UOOBElementWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
    UPROPERTY()
    FS_OOBUnit Data;// Bound UI elements

    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;
	
public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ListView binding override
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    void ShowElementData();
    void SetHighlight(bool bHighlighted);
};
