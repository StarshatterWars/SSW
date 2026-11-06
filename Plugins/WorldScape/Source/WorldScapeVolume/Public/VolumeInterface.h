// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Components/BoxComponent.h"
#include "VolumeInterface.generated.h"

UCLASS(hideCategories = (Code))
class WORLDSCAPEVOLUME_API AVolumeInterface : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AVolumeInterface();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	bool IsEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
	int32 Priority = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults")
		bool AlignToPlanetSurface = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defaults", meta = (EditCondition = "AlignToPlanetSurface == true"))
		FRotator SurfaceAlignmentRotationOffset = FRotator(0, 0, 0);

	bool PREVIOUS_ENABLED;
	int32 PREVIOUS_Priority;
	FVector PREVIOUS_Location;
	FVector PREVIOUS_Scale;
	FRotator PREVIOUS_Rotation;

	// Cy _ Add Force Refresh for pool methode
	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_ForceRefresh()
	{
		NeedRefresh = true;
	};

	virtual void OnConstruction(const FTransform& Transform) override;

	//Used to know if planet need Foliage reload 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Code")
		bool NeedRefresh;

	UBoxComponent* VisualBorder;

	
};