// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CentralSunActor.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInterface.h"
#include "CentralSunWidget.generated.h"

/**
 * Widget that displays the 3D-rendered central star
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSunClickedDelegate);

UCLASS()
class STARSHATTERWARS_API UCentralSunWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnSunClickedDelegate OnSunClicked;
	
	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanelSlot* SunSlot;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* SunNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* SunImage;

	// Material reference for black-transparent UI material
	UPROPERTY(EditDefaultsOnly, Category = "Render")
	UMaterialInterface* SunWidgetMaterial;
	
	UFUNCTION()
	void InitializeFromSunActor(ACentralSunActor* SunActor);
	UFUNCTION()
	void SetWidgetRenderTarget(UTextureRenderTarget2D* InRT, EBodyUISizeClass SizeClass);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	ACentralSunActor* CentralStar; 
	
	UPROPERTY()
	float StarSize;
};
