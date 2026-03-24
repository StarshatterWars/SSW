/*  Project nGenEx
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.
	SUBSYSTEM:    SSW	
	FILE:         PlanetMarkerWidget.cpp
	AUTHOR:       Carlos Bott
*/


#include "PlanetMarkerWidget.h"
#include "PlanetPanelActor.h"


void UPlanetMarkerWidget::InitFromPlanetActor(const FS_PlanetMap& Planet, APlanetPanelActor* PlanetActor)
{
	PlanetData = Planet;
	InitCommon(PlanetData.Name, PlanetData.Radius, EBodyUISizeClass::Planet);
	SetWidgetRenderTarget(PlanetActor->GetRenderTarget(), ObjectWidgetMaterial);
}

