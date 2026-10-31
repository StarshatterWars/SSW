// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"
#include "Components/BillboardComponent.h"
#include "GameFramework/Actor.h"

#include "VortexSpline.generated.h"

UCLASS()
class PROCEDURALVORTEXTUNNEL_API AVortexSpline : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AVortexSpline();

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Billboard")
	UBillboardComponent* RootBillboard;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Spline")
	USplineComponent* SplineComponent;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
