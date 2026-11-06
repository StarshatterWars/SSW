// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_GridStruct.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "WorldScape_GridSpawnerVolume.generated.h"

class AWorldScapeRoot;
class UWorldScape_GridDataAsset;

UCLASS(Blueprintable)
class WORLDSCAPECORE_API AWorldScape_GridSpawnerVolume : public AActor
{
	GENERATED_BODY()

public:
	
	AWorldScape_GridSpawnerVolume();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TArray<TSoftObjectPtr<UWorldScape_GridDataAsset>> SpawnDataAsset;
	// Static Data ?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	float SpawnScale = 1.0f;

	// Direct Linked Root , if spawned by WS , auto get owner
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	AWorldScapeRoot* Linked_WS_Root = nullptr;

	UFUNCTION()
	double WS_ZoneGetSize() const
	{
		return SphereComponent->GetScaledSphereRadius();
	}

protected:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WS")
	USphereComponent* SphereComponent;

private:
	
	UPROPERTY()
	AWorldScapeRoot* WS_Root = nullptr;
	UPROPERTY()
	FWS_Grid CoordBase;
	
};
