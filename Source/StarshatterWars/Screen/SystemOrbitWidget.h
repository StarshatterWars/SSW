// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h" // FS_Galaxy struct
#include "SSWGameInstance.h"
#include "SystemOrbitWidget.generated.h"

/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API USystemOrbitWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Set the orbit radius in screen units (pixels) */
	void SetOrbitRadius(float InRadius)
	{
		OrbitRadius = InRadius;
		Invalidate(EInvalidateWidget::Paint);
	}

	/** Set the orbital inclination in degrees */
	void SetOrbitInclination(float InDegrees)
	{
		UE_LOG(LogTemp, Warning, TEXT("SetOrbitInclination called with: %.2f"), InDegrees);
		OrbitInclinationDeg = InDegrees;
		Invalidate(EInvalidateWidget::Paint);
	}

protected:

	/** Paint the orbit ellipse */
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled
	) const override;

protected:

	UPROPERTY()
	float OrbitRadius = 100.0f;

	UPROPERTY()
	float OrbitInclinationDeg = 0.0f;
};