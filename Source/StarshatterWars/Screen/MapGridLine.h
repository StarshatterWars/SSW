// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_Galaxy struct
#include "SSWGameInstance.h"
#include "MapGridLine.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UMapGridLine : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	// Provide systems and scale
	void SetGalaxyData(const TArray<FS_Galaxy>& Systems, float InMapScale); 

protected:
	void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
private:
	UPROPERTY()
    TArray<FS_Galaxy> GalaxySystems; // Store this to access system bounds	
	UPROPERTY()
    float GridStep = 2.0f;
	UPROPERTY()
    float MapScale = 70.0f;
};
