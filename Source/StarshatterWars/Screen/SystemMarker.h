// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Engine/Texture2D.h"
#include "SSWGameInstance.h"
#include "SystemMarker.generated.h"

/**
 * 
 */

DECLARE_DELEGATE_OneParam(FOnMarkerClicked, const FString&);

UCLASS()
class STARSHATTERWARS_API USystemMarker : public UUserWidget
{
	GENERATED_BODY()
	
public:
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* StarImage;
    UPROPERTY(meta = (BindWidgetOptional))
    UImage* IffImage;
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock* SystemNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* HighlightBorder;

    FS_Galaxy SystemData;

    FOnMarkerClicked OnClicked;

    // Initialize with system data and available textures
    UFUNCTION()
    void Init(const FS_Galaxy& System);
    UTexture2D* LoadTextureFromFile(FString Path);
    FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize);
    
    UFUNCTION()
    void SetSelected(bool bIsSelected);

    FString GetSystemName() const { return SystemData.Name; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Galaxy")
    void PlayGlow();

    UFUNCTION(BlueprintImplementableEvent, Category = "Galaxy")
    void StopGlow();

    UFUNCTION(BlueprintImplementableEvent, Category = "Galaxy")
    void PlayIFFPulse();

    UFUNCTION(BlueprintImplementableEvent, Category = "Galaxy")
    void StopIFFPulse();

protected:
    void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

 private:
    FLinearColor Tint;
    FString SystemName;
};
