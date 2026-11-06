// Fill out your copyright notice in the Description page of Project Settings.


#include "WS_Grid_InvalidSphere.h"

// -------------------------------------------------------------
AWS_Grid_InvalidSphere::AWS_Grid_InvalidSphere()
{
	PrimaryActorTick.bCanEverTick = false;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetComponentTickEnabled(false);
	SetRootComponent(SphereComponent);
}


