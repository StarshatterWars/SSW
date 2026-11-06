// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_FCP_CollisionActor.h"
#include "Engine/StaticMesh.h"

// ---------------------------------------------------
AWorldScape_FCP_CollisionActor::AWorldScape_FCP_CollisionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	if (RootComponent)
	{
		RootComponent->SetMobility(EComponentMobility::Static);
	}
	else
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
		RootComponent->SetMobility(EComponentMobility::Static);
	}
}
// ---------------------------------------------------
UWorldScape_FCP_CollisionComponent* AWorldScape_FCP_CollisionActor::FCP_CollisionComponent_Create( UStaticMesh* StaticMesh , const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale )
{
	if ( IsValid(this) && IsValid(StaticMesh) )
	{
		FString MeshName = StaticMesh->GetName();
		FName CompName = *FString::Printf(TEXT("FCP_Collision_%s"), *MeshName);
		
		UWorldScape_FCP_CollisionComponent* NewCollisionComponent = NewObject<UWorldScape_FCP_CollisionComponent>(this, CompName);
		if ( NewCollisionComponent )
		{
			//NewCollisionComponent->SetMobility(EComponentMobility::Static);
			NewCollisionComponent->SetMobility(EComponentMobility::Movable);
			NewCollisionComponent->SetupAttachment(RootComponent);
			NewCollisionComponent->RegisterComponent();
			NewCollisionComponent->FCP_MakeCollisionPooling(StaticMesh,CollisionSettings,PoolScale);
			Components.Add( NewCollisionComponent );
			return NewCollisionComponent;
		}
	}
	return nullptr;
}
// ---------------------------------------------------
void AWorldScape_FCP_CollisionActor::FCP_CollisionComponent_Destroy()
{
	for ( auto& Component : Components )
	{
		if ( IsValid(Component) )
		{
			Component->FCP_RemovePool();
		}
	}
}

