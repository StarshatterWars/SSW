// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_GridSpawnerVolume.h"
#include "WorldScapeRoot.h"

// --------------------------------------
AWorldScape_GridSpawnerVolume::AWorldScape_GridSpawnerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	RootComponent = SphereComponent;
	SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SphereComponent->SetComponentTickEnabled(false);
}
// --------------------------------------
void AWorldScape_GridSpawnerVolume::BeginPlay()
{
	Super::BeginPlay();
	WS_Root = Cast<AWorldScapeRoot>(GetOwner());
	if ( IsValid(WS_Root) )
	{
		WS_Root->Grid_RegisterSpawnerVolume(this,CoordBase);
	}
	else
	{
		if ( IsValid(Linked_WS_Root) )
		{
			WS_Root =  Linked_WS_Root; // Set Root if owner not valid
			WS_Root->Grid_RegisterSpawnerVolume(this,CoordBase);
		}
	}
}
// --------------------------------------
void AWorldScape_GridSpawnerVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if ( IsValid(WS_Root) )
	{
		WS_Root->Grid_UnRegisterSpawnerVolume(this,CoordBase);
	}
}
// --------------------------------------
void AWorldScape_GridSpawnerVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
#if WITH_EDITOR
	WS_Root = Cast<AWorldScapeRoot>(GetOwner());
	if ( IsValid(WS_Root) )
	{
		WS_Root->Grid_RegisterSpawnerVolume(this,CoordBase);
	}
	else
	{
		if ( IsValid(Linked_WS_Root) )
		{
			WS_Root =  Linked_WS_Root; // Set Root if owner not valid
			WS_Root->Grid_RegisterSpawnerVolume(this,CoordBase);
		}
	}
	#endif
}
// --------------------------------------
void AWorldScape_GridSpawnerVolume::Destroyed()
{
#if WITH_EDITOR
	
#endif
	if ( IsValid(WS_Root) )
	{
		WS_Root->Grid_UnRegisterSpawnerVolume(this,CoordBase);
	}
	Super::Destroyed();
}