// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WorldScape_GridStruct.h"
#include "WorldScape_SnapStruct.h"
#include "GameFramework/Actor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TimerManager.h"
#include "WorldScape_SnapSpawnerActor.generated.h"

class AWorldScapeRoot;
class UWorldScape_SnapDataAsset;
class UWorldScape_GridDataAsset;

UCLASS(Blueprintable,hideCategories = (Replication,Collision,HLOD,Physics,Networking,Input,Rendering,WorldPartition,LevelInstance,Cooking,DataLayers))
class WORLDSCAPECORE_API AWorldScape_SnapSpawnerActor : public AActor
{
	GENERATED_BODY()

public:
	
	AWorldScape_SnapSpawnerActor();

protected:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickActor(float DeltaTime, enum ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;

public:
	
	virtual void BeginDestroy() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	UFUNCTION(BlueprintCallable,CallInEditor, Category = "OnEditor" )
	void ViewFlat();
	UFUNCTION(BlueprintCallable,CallInEditor, Category = "OnEditor" )
	void Simulate();
	UFUNCTION(BlueprintCallable,CallInEditor, Category = "OnEditor" )
	void Clear();
#endif	

	UFUNCTION(BlueprintCallable, Category = "WS")
	void BuildFromWS();
	UFUNCTION(BlueprintCallable, Category = "WS")
	void AutoGetWS();

	
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	bool AutoSimulateOnconstruct = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	bool AutoUseFlatviewIfWSFail = true;
#endif
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	bool SpawnOnDedicated = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	bool AutoGet_NearWS = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	bool AutoStartOnBegin = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Auto")
	float AutoStart_Delay = 2.0f;
	// Data Asset
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	EWSSnapDataType DataType = EWSSnapDataType::SnapData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	UWorldScape_SnapDataAsset* SnapDataAsset;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	UWorldScape_GridDataAsset* GridDataAsset;
	// Manual Data
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_Actor_Data> Collection_ActorsDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<FWS_Grid_StaticMesh_Data> Collection_StaticMeshDatas;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	FVector Extent = FVector(1000,1000,1000);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Culling")
	bool ComputeCulling = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Culling")
	float CullingAngleBase = 0.3f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TSubclassOf<UInstancedStaticMeshComponent> InstancedClass = UHierarchicalInstancedStaticMeshComponent::StaticClass();
	// reference
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	TArray<UObject*> ReferencedObject;
	// Data
	UFUNCTION()
	FWS_SnapData Get_SnapData();
	UFUNCTION()
	bool Get_DataValid();
	UFUNCTION()
	void SetSpawnData( const FWS_SnapSpawnData& Data , const bool PendingManager = false );
	// Auto get WS
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "WorldScape Get")
	AWorldScapeRoot* WorldScapeRoot = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "WorldScape Get")
	FName WS_ValidTag = TEXT("WS_Planetary");
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "WorldScape Get")
	FName WS_ExcludeTag = TEXT("WS_Exclude");
	// Pending
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Pending")
	bool Use_PendingSpawn = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Pending")
	int32 PendingSpawnActor_PerTick = 10;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Pending")
	int32 PendingSpawnStaticMesh_PerTick = 2;
	
	// Referenced Actor
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Transient , SkipSerialization ,Category = "InternalData")
	TArray<AActor*> ReferencedActor;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Transient , SkipSerialization ,Category = "InternalData")
	TMap<UStaticMesh*,UInstancedStaticMeshComponent*> StaticMeshComponent;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Transient , SkipSerialization ,category = "InternalData" )
	TMap<UStaticMesh*,double> StaticMeshComponentSizing;

	// ExternalPending
	UFUNCTION()
	bool Next_Pending();
	
private:
	
	UFUNCTION()
	FWS_SnapSpawnData ConvertDataToSpawnDirect( const FWS_SnapData& Data );
	// State
	UPROPERTY(Transient , SkipSerialization)
	bool IsDedicated = false;
	UPROPERTY(Transient , SkipSerialization)
	bool IsSpawner = false;
	UPROPERTY(Transient , SkipSerialization)
	bool OnPendingSpawn = false;

	UPROPERTY()
	FTimerHandle DeferredStartTimer;
	// Make Actor
	UFUNCTION()
	void Make_ActorList( const TArray<FWS_Grid_Actor_SpawnData>& ActorData );
	UFUNCTION()
	void Make_Actor( FWS_Grid_Actor_SpawnData& ActorData );
	UFUNCTION()
	void Clear_ActorList();
	// Make StaticMesh
	UFUNCTION()
	void Make_StaticMeshList( const TArray<FWS_Grid_Static_SpawnData>& StaticMeshData );
	UFUNCTION()
	void Make_StaticMesh( FWS_Grid_Static_SpawnData& StaticMeshData );
	UFUNCTION()
	void Clear_StaticMeshList();
	// Pending
	UPROPERTY(Transient,SkipSerialization)
	TArray<FWS_Grid_Actor_SpawnData> PendingSpawnActorData;
	UPROPERTY(Transient,SkipSerialization)
	TArray<FWS_Grid_Static_SpawnData> PendingSpawnStaticMeshData;
	UPROPERTY(Transient,SkipSerialization)
	bool IsExternalPending = false;
	UFUNCTION()
	void Update_Pending();
	UFUNCTION()
	void ResetPending();

	// Post Compute
	UFUNCTION()
	double PostComputeCulling( const double Size );
	UFUNCTION()
	void PostCompute();

#if WITH_EDITORONLY_DATA
	// Billboard
	UPROPERTY(Transient , SkipSerialization)
	TObjectPtr<UBillboardComponent> Billboard_Component = nullptr;
#endif
	
};
