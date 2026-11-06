// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"
#include <WorldScapeCommon/Public/DoublePrecisionUtils.h>
#include <WorldScapeNoise/Public/NoiseData.h>
#include <WorldScapeNoise/Public/WorldScapeNoiseClass.h>
#include "Components/BoxComponent.h"
#include "NoiseVolumeData.generated.h"


USTRUCT(BlueprintType)
struct WORLDSCAPEVOLUME_API FNoiseVolumeData {
	GENERATED_BODY()
public:
	bool IsEnabled;
	float EdgeFalloff;
	float NoiseScale;
	float NoiseIntensity;
	float NoiseHeightOffset;
	int Seed;
	CustomNoise CustomVolumeNoise;
	UWorldScapeNoiseClass* WorldScapeNoise;
	FVector NoiseOffset;
	FTransform VolumeLocation;

	bool IsValidNoise();
};