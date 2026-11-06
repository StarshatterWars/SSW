// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_FCP_CollisionComponent.h"
#include "WorldScapeFoliageFCP_Struct.h"
#include "GameFramework/Actor.h"
#include "WorldScape_FCP_CollisionActor.generated.h"

UCLASS()
class WORLDSCAPECORE_API AWorldScape_FCP_CollisionActor : public AActor
{
	GENERATED_BODY()

public:
	
	AWorldScape_FCP_CollisionActor();
	
	UFUNCTION()
	UWorldScape_FCP_CollisionComponent* FCP_CollisionComponent_Create( UStaticMesh* StaticMesh , const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale );
	UFUNCTION()
	void FCP_CollisionComponent_Destroy();

private:
	
	UPROPERTY()
	TArray<UWorldScape_FCP_CollisionComponent*> Components;
	
};
