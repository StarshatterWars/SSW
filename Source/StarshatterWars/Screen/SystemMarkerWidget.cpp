// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/


#include "SystemMarkerWidget.h"

void USystemMarkerWidget::SetSelected(bool isSelected)
{
	if (HighlightBorder)
	{
		bSelected = isSelected;
		HighlightBorder->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void USystemMarkerWidget::InitCommon(const FString& DisplayName, float Radius, EBodyUISizeClass SizeClass)
{
	SetVisibility(ESlateVisibility::Visible);

	bSelected = false;
	CachedName = DisplayName;
	CachedRadius = Radius;

	MarkerSize = SystemMapUtils::GetUISizeFromRadius(Radius / 10, SizeClass);
	SetToolTipText(FText::FromString(DisplayName));

	if (ObjectNameText)
	{
		ObjectNameText->SetText(FText::FromString(DisplayName));
		ObjectNameText->SetColorAndOpacity(FLinearColor::White);
	}
}

FReply USystemMarkerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnObjectClicked.Broadcast(CachedName);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USystemMarkerWidget::SetWidgetRenderTarget(UTextureRenderTarget2D* InRT, UMaterialInterface* WidgetMaterial)
{
	if (InRT && ObjectImage && WidgetMaterial)
	{
		SystemMapUtils::ApplyRenderTargetToImage(
			this,
			ObjectImage,
			WidgetMaterial,
			InRT,
			FVector2D(MarkerSize, MarkerSize)
		);
	}
}








