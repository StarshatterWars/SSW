// /*  Project nGenEx	Fractal Dev Games	Copyright (C) 2024. All Rights Reserved.	SUBSYSTEM:    SSW	FILE:         Game.cpp	AUTHOR:       Carlos Bott*/

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameStructs.h" // FS_Galaxy struct
#include "SSWGameInstance.h"
#include "GalaxyManager.generated.h"

class UTextureRenderTarget2D;
class UUserWidget;
class FWidgetRenderer;
/**
 * 
 */
UCLASS()
class STARSHATTERWARS_API UGalaxyManager : public UObject
{
	GENERATED_BODY()
	
public:
		
	// GalaxyManager.h
	UPROPERTY()
	UTextureRenderTarget2D* SystemOverviewRenderTarget = nullptr;

	UFUNCTION()
	UTextureRenderTarget2D* GetOrCreateSystemOverviewRenderTarget(
		UWorld* World,
		const FBox2D& ContentBounds,
		float Padding = 512.f
	);

	static UGalaxyManager* Get(UObject* WorldContext);

	void LoadGalaxy(const TArray<FS_Galaxy>& ParsedSystems);
	const FS_Galaxy* FindSystemByName(const FString& Name) const;

	UFUNCTION()
	UTextureRenderTarget2D* GetOrCreateRenderTarget(const FString& Name, int32 Resolution, UObject* Object = nullptr);

	UFUNCTION()
	void ClearAllRenderTargets();

	// Capture any widget to texture
	UFUNCTION()
	UTextureRenderTarget2D* RenderWidgetToTarget(UUserWidget* Widget, int32 Width, int32 Height, float Scale = 1.0f);

	UFUNCTION()
	bool RenderWidgetToImage(UUserWidget* Widget, UImage* TargetImage, UMaterialInterface* OverlayMaterial, FVector2D RenderSize, float Scale);
	UFUNCTION()
	const TArray<FS_Galaxy>& GetAllSystems() const { return Systems; }

	UPROPERTY()
	TMap<FString, UTextureRenderTarget2D*> RenderTargets;

private:
	UPROPERTY()
	TArray<FS_Galaxy> Systems;

	UPROPERTY()
	TArray<FS_PlanetMap> Sectors;

	TSharedPtr<FWidgetRenderer> WidgetRenderer;
};

