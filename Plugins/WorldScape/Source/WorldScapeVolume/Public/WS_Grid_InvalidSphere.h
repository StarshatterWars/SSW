// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "WS_Grid_InvalidSphere.generated.h"

UCLASS()
class WORLDSCAPEVOLUME_API AWS_Grid_InvalidSphere : public AActor
{
	GENERATED_BODY()

public:
	
	AWS_Grid_InvalidSphere();
	UPROPERTY()
	USphereComponent* SphereComponent;

	UFUNCTION()
	double WS_ZoneGetSize() const
	{
		return SphereComponent->GetScaledSphereRadius();
	}
};
