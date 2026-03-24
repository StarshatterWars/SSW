// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GalaxyLink.generated.h"

class UImage;
/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UGalaxyLink : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* LineImage;

    UFUNCTION(BlueprintCallable, Category = "Galaxy")
    void ConfigureLine(float Length, float AngleDegrees, FLinearColor Tint = FLinearColor::White);
};
