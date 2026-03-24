// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInterface.h"
#include "SystemMapUtils.h"
#include "GameStructs.h"
#include "SSWGameInstance.h"
#include "SystemMarkerWidget.generated.h"

class UImage;
class UBorder;
/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlanetClicked, const FString&, ObjectName);

UCLASS()
class STARSHATTERWARS_API USystemMarkerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION()
	void SetSelected(bool bSelected);
	// Call from derived InitFromXActor after it sets its data struct
	
	void InitCommon(const FString& DisplayName, float Radius, EBodyUISizeClass SizeClass);

	FOnPlanetClicked OnObjectClicked;
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void SetWidgetRenderTarget(UTextureRenderTarget2D* InRT, UMaterialInterface* WidgetMaterial);

	UPROPERTY(meta = (BindWidgetOptional)) 
	class UTextBlock* ObjectNameText;

	UPROPERTY(meta = (BindWidgetOptional)) 
	class UImage* ObjectImage;

	UPROPERTY(meta = (BindWidgetOptional))
	class UBorder* HighlightBorder;

	UPROPERTY()
	FString CachedName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UMaterialInterface* ObjectWidgetMaterial;

	UPROPERTY()
	float CachedRadius = 0.f;

	UPROPERTY()
	float MarkerSize = 0.f;

	UPROPERTY()
	bool bSelected = false;
};
