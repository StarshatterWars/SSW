// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_Galaxy struct
#include "SSWGameInstance.h"
#include "JumpLinksWidget.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UJumpLinksWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    void SetJumpLinks(const TArray<FJumpLink>& InLinks);

protected:
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
    UPROPERTY()
    TArray<FJumpLink> JumpLinks;	
	
};
