/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    SSW
    FILE:         SystemMarker.h
    AUTHOR:       Carlos Bott

    OVERVIEW
    ========
    Galaxy system marker widget.
*/

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameStructs.h"
#include "SystemMarker.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UTexture2D;

DECLARE_DELEGATE_OneParam(FOnSystemMarkerClicked, const FString&);

UCLASS()
class STARSHATTERWARS_API USystemMarker : public UUserWidget
{
    GENERATED_BODY()

public:
    void Init(const FS_Galaxy& System, UTexture2D* InStarTexture);
    void SetSelected(bool bIsSelected);

    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent) override;

    FSlateBrush CreateBrushFromTexture(UTexture2D* Texture, FVector2D ImageSize) const;

public:
    FOnSystemMarkerClicked OnClicked;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    UTextBlock* SystemNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* StarImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UImage* IffImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    UBorder* HighlightBorder = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "System Marker")
    FS_Galaxy SystemData;

    UPROPERTY(BlueprintReadOnly, Category = "System Marker")
    FString SystemName;

    UPROPERTY(BlueprintReadOnly, Category = "System Marker")
    FLinearColor Tint = FLinearColor::Gray;

protected:
    UFUNCTION(BlueprintImplementableEvent)
    void PlayGlow();

    UFUNCTION(BlueprintImplementableEvent)
    void StopGlow();
};