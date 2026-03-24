// Project nGenEx
// Fractal Dev Games
// Copyright (C) 2024. All Rights Reserved.
// 
// SUBSYSTEM:    SSW
// FILE:         MoonMarkerWidget.cpp
// AUTHOR:       Carlos Bott


#include "MoonMarkerWidget.h"

void UMoonMarkerWidget::InitFromMoonActor(const FS_MoonMap& Moon, AMoonPanelActor* MoonActor)
{
	MoonData = Moon;
	bSelected = false;

	InitCommon(MoonData.Name, MoonData.Radius, EBodyUISizeClass::Moon);
	SetWidgetRenderTarget(MoonActor->GetRenderTarget(), ObjectWidgetMaterial);
}

