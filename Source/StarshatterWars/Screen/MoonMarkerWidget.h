// Project nGenEx
// Fractal Dev Games
// Copyright (C) 2024. All Rights Reserved.
// 
// SUBSYSTEM:    SSW
// FILE:         MoonMarkerWidget.cpp
// AUTHOR:       Carlos Bott

#pragma once

#include "CoreMinimal.h"
#include "SystemMarkerWidget.h"
#include "MoonPanelActor.h"
#include "MoonMarkerWidget.generated.h"

UCLASS()
class STARSHATTERWARS_API UMoonMarkerWidget : public USystemMarkerWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FS_MoonMap MoonData;

	UFUNCTION()
	void InitFromMoonActor(const FS_MoonMap& Planet, AMoonPanelActor* MoonActor);
};