// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "WorldScape_ISMComponent.generated.h"


USTRUCT()
struct FWS_ISM_TransformsCell
{
	GENERATED_BODY()
	UPROPERTY()
	FVector CellLocation = FVector(0, 0, 0);
	UPROPERTY()
	bool OnBillboard = false;
	UPROPERTY()
	TArray<FTransform> Transforms;
	UPROPERTY()
	TArray<int32> InstanceIndex;
	FWS_ISM_TransformsCell() = default;
};
/**
 * 
 */
UCLASS()
class WORLDSCAPECORE_API UWorldScape_ISMComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;

	UPROPERTY(EditAnywhere,Category = "Billboard")
	UStaticMesh* BillboardMesh = nullptr;
	UPROPERTY(EditAnywhere,Category = "Billboard")
	int32 BillboardDistance = 44000;



private:

	UPROPERTY(EditAnywhere,Category = "Billboard")
	UInstancedStaticMeshComponent* BillboardComponent = nullptr;
	UPROPERTY(EditAnywhere,Category = "Instance")
	TArray<FWS_ISM_TransformsCell> TransformsInstances;

	UFUNCTION()
	void MakeBillboardComponent();
	UFUNCTION()
	void DestroyAll();

	UFUNCTION()
	void MakeGridVisibility();
	UFUNCTION()
	void UpdateVisibility();
	
};
