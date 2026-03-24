// Project nGenEx
// Fractal Dev Games
// Copyright (C) 2024. All Rights Reserved.
// 
// SUBSYSTEM:    SSW
// FILE:         PlanetMarkerWidget.h
// AUTHOR:       Carlos Bott

#pragma once

#include "CoreMinimal.h"
#include "SystemMarkerWidget.h"
#include "PlanetPanelActor.h"
#include "PlanetMarkerWidget.generated.h"


UCLASS()
class STARSHATTERWARS_API UPlanetMarkerWidget : public USystemMarkerWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FS_PlanetMap PlanetData;

	UFUNCTION()
	void InitFromPlanetActor(const FS_PlanetMap& Planet, APlanetPanelActor* PlanetActor);
};
