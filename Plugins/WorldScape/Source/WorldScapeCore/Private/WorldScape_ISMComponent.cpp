// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldScape_ISMComponent.h"
#include "Engine/World.h"


// --------------------------------------------------
void UWorldScape_ISMComponent::BeginPlay()
{

	Super::BeginPlay();
}
// --------------------------------------------------
void UWorldScape_ISMComponent::BeginDestroy()
{

	Super::BeginDestroy();
}
// --------------------------------------------------
void UWorldScape_ISMComponent::MakeBillboardComponent()
{
	
}
// --------------------------------------------------
void UWorldScape_ISMComponent::DestroyAll()
{
	
}
// --------------------------------------------------
void UWorldScape_ISMComponent::MakeGridVisibility()
{
	
}
// --------------------------------------------------
void UWorldScape_ISMComponent::UpdateVisibility()
{
	if ( APlayerCameraManager *CameraManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager )
	{
		const FVector CameraLocation = CameraManager->GetCameraLocation();
		if ( (CameraLocation-GetComponentLocation()).SquaredLength() < FMath::Square(Bounds.SphereRadius+BillboardDistance) )
		{
			
		}
	}
	
}