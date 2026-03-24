// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_OOBBattery definition
#include "Blueprint/IUserObjectListEntry.h"
#include "OOBStarbaseWidget.generated.h"

class UTextBlock;
class UImage;
class UListView;
struct FS_OOBStarbase;

/**
 * Starbase UI Widget - represents one Starbase under a Battalion
 */

UCLASS()
class STARSHATTERWARS_API UOOBStarbaseWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
    UPROPERTY()
    FS_OOBStarbase Data;

    // Bound UI elements
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* NameText;
	
public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // ListView binding override
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    void SetHighlight(bool bHighlighted);
    void ShowElementData();
};

