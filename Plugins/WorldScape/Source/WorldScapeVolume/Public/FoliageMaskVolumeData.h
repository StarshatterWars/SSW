// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include "Components/BoxComponent.h"
#include "FoliageMaskVolumeData.generated.h"

USTRUCT(BlueprintType)
struct WORLDSCAPEVOLUME_API FFoliageMaskVolumeData {
	GENERATED_BODY()
public :
	bool IsEnabled;
	float EdgeFalloff;
	FTransform VolumeLocation;
	TArray<int> FoliageLayerMask;
	bool SpawnVolume;
	
	bool UseBlending;
	float BlendFalloff;

	bool MaskAll;
};
