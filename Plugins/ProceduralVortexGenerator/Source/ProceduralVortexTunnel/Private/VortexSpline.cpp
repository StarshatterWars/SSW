// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "VortexSpline.h"



// Sets default values
AVortexSpline::AVortexSpline()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootBillboard = CreateDefaultSubobject<UBillboardComponent>("RootBillboard");
	RootComponent = RootBillboard;
	
	SplineComponent = CreateDefaultSubobject<USplineComponent>("VortexSpline");
	SplineComponent->SetRelativeRotation(FRotator(0, 0, 0));
	SplineComponent->AttachToComponent(RootBillboard, FAttachmentTransformRules::KeepRelativeTransform);

#if WITH_EDITOR

	SplineComponent->EditorTangentColor = FLinearColor(1,0,0,1);

#endif
	
}

// Called when the game starts or when spawned
void AVortexSpline::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVortexSpline::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

