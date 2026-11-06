// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "WorldScapeFoliageFCP_Struct.h"
#include "WorldScape_FCP_CollisionComponent.generated.h"

/**
 * 
 */
UCLASS()
class WORLDSCAPECORE_API UWorldScape_FCP_CollisionComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	
	UWorldScape_FCP_CollisionComponent(const FObjectInitializer& ObjectInitializer);
	// =============================================================================
	// Delegate
	// =============================================================================
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCompReady, UWorldScape_FCP_CollisionComponent* , Comp );
	UPROPERTY(BlueprintAssignable , Category="FCP")
	FCompReady CompReady;
	
	// =============================================================================
	// Function
	// =============================================================================
	UFUNCTION()
	bool FCP_IsReady();
	
	UFUNCTION()
	void FCP_MakeCollisionPooling( UStaticMesh* inStaticMesh , const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale );
	UFUNCTION()
	void FCP_RemovePool();

	UPROPERTY()
	//TSoftObjectPtr<UStaticMesh> StaticMeshBase;
	UStaticMesh* StaticMeshBase = nullptr;
	UPROPERTY()
	TArray<FTransform> PendingTransforms;
	UFUNCTION()
	int32 CountWorkAvailable();

	// ----------------------------------------
	// Update Function
	// ----------------------------------------
	UFUNCTION()
	bool Update_InstanceTransform( const FTransform& NewTransform );
	UFUNCTION()
	void Reset_Work_Index();
	UFUNCTION()
	bool Get_WorkTO();
	UPROPERTY()
	int32 Work_Index = 0;
	UPROPERTY()
	bool OnAsync = false;
	// Setup Body
	UPROPERTY()
	UBodySetup* BodySetup = nullptr;
	UPROPERTY()
	FBodyInstance QDC_BodyInstance;
	UPROPERTY()
	int32 BasePoolSize = 100;
	UPROPERTY()
	float StaticMeshBoundsRadius = 1.0;
	UPROPERTY()
	double MinimalScale = 0.1;
	
private:
	
	UFUNCTION()
	void Compute_Internal_Data( UStaticMesh*& inStaticMesh ,const FWS_FCP_PoolingData& CollisionSettings , const double PoolScale);
	UFUNCTION()
	void Internal_Make_Pool();
	// State
	UPROPERTY()
	bool IsReady = false;
	// Internal Data
	// Storage Instances
	TArray<FBodyInstance*> BodyInstances;
	
};
