// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "CentralSunWidget.h"
#include "Components/Image.h"
#include "Components/Widget.h"
#include "Components/CanvasPanelSlot.h"
#include "SSWGameInstance.h"
#include "StarUtils.h"
#include "SystemMapUtils.h"

void UCentralSunWidget::InitializeFromSunActor(ACentralSunActor* SunActor)
{
	CentralStar = SunActor;

	UTextureRenderTarget2D* RT = CentralStar->GetRenderTarget();

	if (!SunImage || !SunActor || !SunWidgetMaterial || !RT)
	{
		UE_LOG(LogTemp, Warning, TEXT("InitFromSunActor: missing setup"));
		return;
	}

	if (SunNameText) {
		SunNameText->SetText(FText::FromString(CentralStar->StarName));
	}

	StarSize = StarUtils::GetUISizeFromRadius(CentralStar->GetRadius()) / 2;
	SetWidgetRenderTarget(RT, EBodyUISizeClass::Star);
	
	float Scale = StarSize / 64.f; // assuming 64 is base size
	FWidgetTransform Transform;
	Transform.Scale = FVector2D(Scale, Scale);
	SunImage->SetRenderTransform(Transform);
}

FReply UCentralSunWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	UE_LOG(LogTemp, Log, TEXT("Sun clicked in CentralSunWidget"));
	OnSunClicked.Broadcast();
	return FReply::Handled();
}

void UCentralSunWidget::SetWidgetRenderTarget(UTextureRenderTarget2D* InRT, EBodyUISizeClass SizeClass)
{
	if (InRT && SunImage && SunWidgetMaterial)
	{
		SystemMapUtils::ApplyRenderTargetToImage(
			this,
			SunImage,
			SunWidgetMaterial,
			InRT,
			FVector2D(StarSize, StarSize)
		);
	}
}
