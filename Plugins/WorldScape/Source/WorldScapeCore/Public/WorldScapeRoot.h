// Copyright 2021 IOLACORP STUDIO. All Rights Reserved
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/CoreDelegates.h"
#include "UObject/Object.h"
#include "UObject/UObjectBase.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include <algorithm>
#include <map>
#include "WorldScapeMeshComponent.h"
#include <WorldScapeCommon/Public/DoublePrecisionUtils.h>
#include <WorldScapeCommon/Public/StopWatch.h>
#include <WorldScapeNoise/Public/NoiseData.h>
#include <WorldScapeNoise/Public/WorldScapeNoiseClass.h>
#include "WorldScapeVolume/Public/NoiseVolume.h"
#include "WorldScapeVolume/Public/HeightMapVolume.h"
#include "WorldScapeVolume/Public/FoliageMaskVolume.h"
#include "WorldScapeVolume/Public/TerrainHoleVolume.h"
#include "WorldScapeVolume/Public/NoiseVolumeData.h"
#include "WorldScapeVolume/Public/HeightMapVolumeDataCopy.h"
#include "WorldScapeVolume/Public/FoliageMaskVolumeData.h"
#include "WorldScapeVolume/Public/WS_Grid_InvalidSphere.h"
#include "WorldScape_SnapStruct.h"

#include "WorldScapeCommon/Public/WorldScapeHelper.h"
#include "WorldScapeCommon/Public/NoiseMathUtils.h"
#include "WorldScapeLod.h"
#include "Engine/World.h"
#include "LodData.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesAsset.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesBlueprint.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCluster.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesClusterUnit.h"
#include "WorldScapeFoliages/Public/FoliagesSector.h"
#include "WorldScapeFoliages/Public/WorldScapeFoliagesCollection.h"
#include "MaterialLod.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "WorldScapeWorldType.h"
#include "WorldscapeSubsystem.h"
#include "WorldscapeCollisionInvoker.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include <iostream>
// FCP
#include "WorldScapeFoliageFCP_Struct.h"
#include "WorldScape_FCP_CollisionComponent.h"
#include "WorldScape_FCP_CollisionActor.h"
#include "WorldScape_FCP_Struct.h"
// Grid Snap
#include "WorldScape_GridSpawnerVolume.h"
#include "WorldScape_GridStruct.h"
#include "WorldScape_SnapSpawnerActor.h"

#include "HAL/RunnableThread.h"
class UWorldScape_GridCellData;
class UWorldScape_GridSerializedData;
class AWorldScape_GridSpawnerVolume;
class UWorldScape_GridDataAsset;

#if WITH_EDITOR
#include <WorldScapeEditor/Public/PCUtils.h>
#endif

#include "WorldScapeRoot.generated.h"


UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EWorldScapeType : uint8
{
	Flat 	UMETA(DisplayName = "Flat World"),
	Planet 	UMETA(DisplayName = "Planet")
};

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EPlanetaryHeightMapBlendType : uint8
{
	//Replace the default noise data with the one from the heightmap
	Replace 	UMETA(DisplayName = "Normal"),
	Addition	UMETA(DisplayName = "Additional"),
    Subtract 	UMETA(DisplayName = "Subtract")
};


static void ProcessVertices(LodData& Data, int32 MinY, int32 MinX, int32 MaxY, int32 MaxX, DVector Tangent, DVector BiTangent, double StepSize, double HalfSize, double uvSpacing,
	FVector2D SubPosition, int32 MinBorderY, int32 MinBorderX, int32 MaxBorderY, int32 MaxBorderX, double OffsetX, double OffsetY, bool InvertOrder, double PlanetScaleCode,
	DVector PPos, bool FlatWorld, FVector2D pWorldSizeLimit = FVector2D(1000000, 1000000), bool pLimitWorldSize = false, bool DealSewing = true)
{
	DVector X, Y;
	DVector PositionNormal;
	bool doSewing = false;
	

	if (InvertOrder)
	{
		for (int32 y = MinY; y < MaxY; y++)
		{
			for (int32 x = MaxX - 1; x >= MinX; x--)
			{
				
				if (DealSewing)
				{

					doSewing = (bool)(y % 2);
					if (SubPosition.Y > 0.5) doSewing = !doSewing;

					if (x == MinBorderX && doSewing)
					{
						Y = BiTangent * ((y + 1) * StepSize - HalfSize + OffsetY);
					}
					else if (x == MaxBorderX && doSewing)
					{
						Y = BiTangent * ((y - 1) * StepSize - HalfSize + OffsetY);
					}
					else
					{
						Y = BiTangent * (y * StepSize - HalfSize + OffsetY);
					}

					doSewing = (bool)(x % 2);
					if (SubPosition.X > 0.5) doSewing = !doSewing;

					if (y == MinBorderY && doSewing)
					{
						X = Tangent * ((x + 1) * StepSize - HalfSize + OffsetX);
					}
					else if (y == MaxBorderY && doSewing)
					{
						X = Tangent * ((x - 1) * StepSize - HalfSize + OffsetX);
					}
					else
					{
						X = Tangent * (x * StepSize - HalfSize + OffsetX);
					}
				}
				else
				{
					Y = BiTangent * (y * StepSize - HalfSize + OffsetY);
					X = Tangent * (x * StepSize - HalfSize + OffsetX);
				}

				if (FlatWorld)
				{
					if (pLimitWorldSize)
					{
						if (abs(X.X) > pWorldSizeLimit.X) X = (pWorldSizeLimit.X * FMath::Sign(X.X));
						if (abs(Y.Y) > pWorldSizeLimit.Y) Y = pWorldSizeLimit.Y * FMath::Sign(Y.Y);
					}
					
					Data.Vertices.Add(DVector(X + Y).ToFVector());
				}
				else
				{
					PositionNormal = DVector(X + Y); // Plane Position
					PositionNormal = PositionNormal + PPos; // ECEF position
					PositionNormal.Normalize();
					
					Data.Vertices.Add(((PositionNormal * PlanetScaleCode) - PPos).ToFVector());
				}
				
				Data.Normals.Add(FVector(0.0f, 0.0f, 1.0f));
				
				Data.UV.Add(FVector2D((x + (int32)SubPosition.X) * uvSpacing, (y + (int32)SubPosition.Y) * uvSpacing));
				
				Data.Tangents.Add(FWorldScapeMeshTangent(Tangent.X, Tangent.Y, Tangent.Z));
				
			}
		}
	}
	else
	{
		for (int32 y = MinY; y < MaxY; y++)
		{
			for (int32 x = MinX; x < MaxX; x++)
			{
				if (DealSewing) {

					doSewing = (bool)(y % 2);
					if (SubPosition.Y > 0.5) doSewing = !doSewing;

					if (x == MinBorderX && doSewing)
					{
						Y = BiTangent * ((y + 1) * StepSize - HalfSize + OffsetY);
					}
					else if (x == MaxBorderX && doSewing)
					{
						Y = BiTangent * ((y - 1) * StepSize - HalfSize + OffsetY);
					}
					else
					{
						Y = BiTangent * (y * StepSize - HalfSize + OffsetY);
					}

					doSewing = (bool)(x % 2);
					if (SubPosition.X > 0.5) doSewing = !doSewing;

					if (y == MinBorderY && doSewing)
					{
						X = Tangent * ((x + 1) * StepSize - HalfSize + OffsetX);
					}
					else if (y == MaxBorderY && doSewing)
					{
						X = Tangent * ((x - 1) * StepSize - HalfSize + OffsetX);
					}
					else
					{
						X = Tangent * (x * StepSize - HalfSize + OffsetX);
					}
				}
				else
				{
					Y = BiTangent * (y * StepSize - HalfSize + OffsetY);
					X = Tangent * (x * StepSize - HalfSize + OffsetX);
				}

				if (FlatWorld)
				{
					if (pLimitWorldSize)
					{
						if (abs(X.X + PPos.X) > pWorldSizeLimit.X) X.X = pWorldSizeLimit.X * FMath::Sign(X.X) - PPos.X;
						if (abs(Y.Y + PPos.Y) > pWorldSizeLimit.Y) Y.Y = pWorldSizeLimit.Y * FMath::Sign(Y.Y) - PPos.Y;
					}
					
					Data.Vertices.Add(DVector(X + Y).ToFVector());
				}
				else
				{
					PositionNormal = DVector(X + Y); // Plane Position
					PositionNormal = PositionNormal + PPos; // ECEF position
					PositionNormal.Normalize();
					
					Data.Vertices.Add(((PositionNormal * PlanetScaleCode) - PPos).ToFVector());
				}
				
				Data.Normals.Add(FVector(0.0f, 0.0f, 1.0f));
				
				Data.UV.Add(FVector2D((x + (int32)SubPosition.X) * uvSpacing, (y + (int32)SubPosition.Y) * uvSpacing));
				
				Data.Tangents.Add(FWorldScapeMeshTangent(Tangent.X, Tangent.Y, Tangent.Z));
				
			}
		}
	}

}


//Forward Declaration

class LodGenerationThread;
class FoliageGenerationThread;
class UWorldscapeCollisionInvoker;
class GridGenerationThread;

//DECLARE_LOG_CATEGORY_EXTERN(LogWorldScapeCore, Log, All);

UCLASS(hideCategories = ( Code,Debug,MeshInfo,Rebase))
//UCLASS(hideCategories = (MeshInfo, Rebase))
class WORLDSCAPECORE_API AWorldScapeRoot : public AActor
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCollisionLodsUpdate, UWorldScapeLod*, WorldscapeLod);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "WS")
	FOnCollisionLodsUpdate OnCollisionLodsUpdate;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInitializedEnd);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "WS")
	FInitializedEnd InitializedEnd;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWS_Pooling_OnInitialized);
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "WS")
	FWS_Pooling_OnInitialized WS_Pooling_OnInitialized;
	
	AWorldScapeRoot();

private:
	UFUNCTION()
	void AddWorldscapeActor(AWorldScapeRoot* WorldScapeActor)
	{
		if (IsValid(WorldScapeActor))
		{
			if ( UWorldscapeSubsystem* SubSystem = GetWorld()->GetSubsystem<UWorldscapeSubsystem>() )
			{
				SubSystem->WS_Registry_WorldScapeRoot(WorldScapeActor);
			}
			//GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeActors.Add(WorldScapeActor);
		}
		for (auto& Elem : GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeCollisionInvokers) {
			if (IsValid(Elem->GetOwner())) {
				CollisionDependantActor.Add(Elem->GetOwner());
			}
		}
		UpdatePosition();
	}

	UFUNCTION()
	void RemoveWorldscapeActor(AWorldScapeRoot* WorldScapeActor)
	{
		if (IsValid(WorldScapeActor))
		{
			if ( UWorldscapeSubsystem* SubSystem = GetWorld()->GetSubsystem<UWorldscapeSubsystem>() )
			{
				SubSystem->WS_UnRegistry_WorldScapeRoot(WorldScapeActor);
			}
			//GetWorld()->GetSubsystem<UWorldscapeSubsystem>()->WorldScapeActors.Add(WorldScapeActor);
		}
	}

	UPROPERTY()
	bool IsInitialized = false;

public:

	UFUNCTION(BlueprintCallable, Category = "WS")
	bool CheckIsInitialized()
	{
		return IsInitialized;
	}
	
	
	void InitializeValues();
	
	virtual void Tick(float DeltaTime) override;
	void CleanFoliage();
	void UpdateFoliageData();
	/** Allows Tick To happen in the editor viewport*/
	virtual bool ShouldTickIfViewportsOnly() const override;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void PostLoad() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void ClearCrossLevelReferences() override;

	virtual bool IsReadyForFinishDestroy() override;
	virtual void BeginDestroy() override;



	//void UpdateWorldOrigin(FIntVector NewPosition);


		//Number of tick per seconds
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Actor")
		float TickPerSecond;



	//Default Properties : 

	//WorldScape Generation Type
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	EWorldScapeType GenerationType;
	bool bFlatWorld = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default")
	bool IsAllowAutoGetNearest = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Code")
		TArray<UMaterialInstanceDynamic*> MaterialTerrainInstances;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Code")
		TArray<UMaterialInstanceDynamic*> MaterialOceanInstances;
	
	
	// -------------------------------------------------------
	// WS_Terrain
	// -------------------------------------------------------
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainAffectDynamicIndirectLighting = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainAffectDistanceFieldLighting = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainContactShadow = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainCastStaticShadow = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainCastDynamicShadow = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainFarShadow = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainTowSideShadow = false;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainMeshTreatAsBackGroundForOcclusion = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainMeshRenderCustomDepthPass = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool TerrainMeshIsOccluder = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	EShadowCacheInvalidationBehavior TerrainVSMShadow_Invalidation = EShadowCacheInvalidationBehavior::Auto;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool Terrain_MakeMaterialInstance = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Terrain_Gen")
	bool Terrain_UpdatePlanetLocation = false;

	UPROPERTY()
	FBodyInstance Terrain_CollisionSettings;
	// -------------------------------------------------------
	// WS_Ocean
	// -------------------------------------------------------
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	bool OceanMeshTreatAsBackGroundForOcclusion = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	bool OceanMeshRenderCustomDepthPass = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	bool OceanMeshIsOccluder = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	EShadowCacheInvalidationBehavior OceanVSMShadow_Invalidation = EShadowCacheInvalidationBehavior::Auto;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	bool Ocean_UseMaterialInstance = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Ocean_Gen")
	bool Ocean_UpdatePlanetLocation = false;

	// -------------------------------------------------------
	// WS_Foliage
	// -------------------------------------------------------
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	TEnumAsByte<EComponentMobility::Type> Foliage_MobilityType = EComponentMobility::Movable;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	bool FoliageMeshIsOccluder = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	bool FoliageRenderTreatAsBackGroundForOcclusion = false;
	UPROPERTY(BlueprintReadWrite, Category = "Foliage_Gen") // OLD
	bool FoliageParallelForCompute = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	EShadowCacheInvalidationBehavior FoliageVSMShadow_Invalidation = EShadowCacheInvalidationBehavior::Auto;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	int32 WPO_FoliageDisabledDistance = 22000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Gen")
	bool Foliage_ForceDisabledCollision = false;
	// -------------------------------------------------------
	// UtilsFunction
	// -------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "WS UTils")
	void WS_CleanFoliage()
	{
		CleanFoliage();
	}
	UFUNCTION(BlueprintCallable, Category = "WS UTils")
	void WS_UpdateFoliageData()
	{
		UpdateFoliageData();
	}
	UFUNCTION(BlueprintCallable, Category = "WS UTils")
	void WS_ForceRegenerate()
	{
		CheckForRegenerate();
		GenerateBaseMesh();
	}
	// -------------------------------------------------------
	// WS_GRID
	// -------------------------------------------------------
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool EnabledGrid = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_SimulateDedicated = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_CheckSpawnBySeed = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_Debug = false;
	// SpawnCell Data
	UPROPERTY(Transient)
	TMap<int64,FWS_Grid_Container> Loaded_Cell;
	UPROPERTY(Transient)
	TArray<int64> Pending_Unload;
	
	UPROPERTY(Transient)
	TArray<FWS_Grid_Container> Pending_DestroyCell;
	UPROPERTY(Transient)
	TArray<FWS_Grid_SpawnPending> Pending_SpawnCell;

	// Volume register
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	TArray<AWS_Grid_InvalidSphere*> Grid_InvalidVolume;

	// Pending Params
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_UsePendingSpawn = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_MaxSpawnPerSecond = 512;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_MaxSpawnPerSecond_OnDedicated = 1024;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_UsePendingDestroy = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_MaxDestroyPerSecond = 1024;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_MaxDestroyPerSecond_OnDedicated = 2048;

	UPROPERTY()
	bool Grid_AutoScalingPending = false;
	UPROPERTY()
	float Grid_BaseInterval = 0.0166f;//60fps
	UPROPERTY()
	bool Grid_EnabledSkipLimite = false;
	UPROPERTY()
	float Grid_SkipLimiteInterval = 0.0833f;//12fps
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_DestroyAllOnOutDistance = false;

	// Spawn Grid Params
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_Size = 64;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_Data_Size = 8;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	double Grid_DistanceToLoad = 5000000;
	// Not Used
	UPROPERTY()
	float Grid_UnloadTimeOut = 10.0;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere,Category = "Grid")
	bool Grid_HaveStaticData = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_StaticData_IsValid = false;


	UPROPERTY(BlueprintReadWrite,EditAnywhere,AdvancedDisplay, Category = "Grid")
	UWorldScape_GridCellData* Grid_CellData = nullptr;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	TArray<UWorldScape_GridDataAsset*> Grid_BaseSpawnData;
	// Storage
	UPROPERTY(Transient)
	TMap<int64,FWS_Grid_StorageData> Grid_LoadedStorage;
	// Scale
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	float Grid_BaseSpawnScale = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	float Grid_GlobalSpawnScale = 1.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_RNDCircle_TestLimite = 600;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	int32 Grid_FoliageBlockSend = 16;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	float Grid_Size_Scale = 1.1f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	bool Grid_SkipStaticMeshOnDedicated = false;
	UPROPERTY()
	bool Grid_SkipAdjacentCellOnDedicated = false;
	
	UFUNCTION()
	void Grid_UpdateInfo();
	
	// Info
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 NumberOfCellLoaded = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 NumberOfThread = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 OnPendingAsyncCompute = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 TempQueueLength = 0;

	
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	bool OnWaitingSpawn = false;
	
	
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	bool FoliageThread = false;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 LeftPendingToSpawn = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 LeftPendingUnLoad = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 LeftPendingToDestroy = 0;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	int32 NumberOfRegisterdVolume = 0;
	
	// StaticData 
	UFUNCTION(BlueprintCallable,CallInEditor, Category = "Grid")
	void Grid_MakeBaseStaticData();
	UFUNCTION(BlueprintCallable,CallInEditor, Category = "Grid")
	void Grid_CheckStaticData();
	
	// Load Unload
	UFUNCTION()
	void Grid_Update( const TArray<FVector>& Locations , const float& DeltaTime );
	UFUNCTION()
	void WS_Grid_LoadCell( const FWS_Grid& Cell );
	UFUNCTION()
	void WS_Grid_UnLoadCell( const int64& CellHash );

	// Update Pending
	UFUNCTION()
	void Grid_Pending_Update( const float DeltaTime );
	UFUNCTION()
	void Grid_PendingSpawn_Update( const float DeltaTime );
	UFUNCTION()
	void Grid_PendingDestroy_Update( const float DeltaTime );
	UFUNCTION()
	void Grid_PendingSpawn_RemoveInvalid();
	
	// Destroy
	UFUNCTION()
	void Grid_DestroyAllCell();
	UFUNCTION()
	void Grid_DestroyCellInstance( const FWS_Grid_Instance& Instance );
	UFUNCTION()
	void WS_Grid_DestroyCell( const int64& CellHash , bool AutoDestroy = true );
	// Spawn
	UFUNCTION()
	void WS_Grid_Spawn_Multi( const FWS_Grid& GridCoord );
	UFUNCTION()
	void WS_Grid_DirectSpawnDataAsset( const FWS_Grid_SpawnConf& SpawnConf , const UWorldScape_GridDataAsset* DataAsset  , const FWS_VolumeData& WSVolume);

	// Sampling
	UFUNCTION()
	bool WS_Grid_SearchPointOnDisk( const TArray<FWS_Grid_InvalidZone>& ZoneData ,FVector& Out , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const TArray<FVector>& Spawned , const FVector& Center , const FVector& UpVector , const float& Radius , const FWS_VolumeData& WS_Volume , const float& TestScale = 1.0f );
	UFUNCTION()
	bool WS_Grid_SnapPoint( FTransform& Transform , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FVector& Location , const FVector& UpVector , const double& Offset , const FWS_VolumeData& WS_Volume );
	UFUNCTION()
	bool Grid_CheckBySpawnParams( const FVector& Location , const FNoiseData& Data , const FWS_Grid_SpawnParams& SpawnParams , const FWS_VolumeData& WS_Volume );
	// Other Sampler
	UFUNCTION()
	TArray<FVector> WS_Grid_SearchPoissonDisk( const TArray<FWS_Grid_InvalidZone>& ZoneData , const int32& MaxSpawn , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FVector& Center , const float& Radius ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	TArray<FVector> WS_Grid_SearchGridSampling( const TArray<FWS_Grid_InvalidZone>& ZoneData , const int32& MaxSpawn , FRandomStream& Random , const FWS_Grid_SpawnParams& SpawnParams , const FTransform& BaseTransform , const double& BaseSize ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	TArray<FVector> WS_Grid_GenerateZoneGrid( const FTransform& BaseTransform , const double& Size , const int32 Step );
	// Compute
	UFUNCTION()
	TArray<FWS_Grid_Actor_SpawnData> Grid_Compute_CollectionActor( FRandomStream& Random , const FTransform& BaseTransform , const TArray<FWS_Grid_Actor_Data>& Collection ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	TArray<FWS_Grid_Static_SpawnData> Grid_Compute_CollectionStaticMesh( FRandomStream& Random , const FTransform& BaseTransform , const TArray<FWS_Grid_StaticMesh_Data>& Collection ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	FTransform WS_Grid_ComputePlacement( FRandomStream& Random , const FTransform& BaseTransform , /*const FVector& Extend*/ const FBoxSphereBounds& Bound, const FWS_Grid_Placement_Data& Placement ,const FWS_VolumeData& WS_Volume);

	UFUNCTION()
	int32 Grid_Compute_ActorMeshIndex( FRandomStream& Random , const TArray<FWS_Grid_Actor_Data>& Actors );
	UFUNCTION()
	int32 Grid_Compute_StaticMeshIndex( FRandomStream& Random , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh );
	// Make Instance Data
	UFUNCTION()
	void Grid_Make_Instance( const FWS_Grid& GridCoord , const FTransform& InstanceTransform , const FWS_Grid_Proxy& VizData , const TArray<FWS_Grid_Actor_SpawnData>& Actors , const TArray<FWS_Grid_Static_SpawnData>& StaticMesh , const bool IsFoliage = false );
	
	// Compute Async
	UFUNCTION()
	void Grid_ComputeSpawn_Actor( const FWS_Grid_SpawnConf& SpawnConf , const TArray<FWS_Grid_Spawner_Actor>& Actors ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	void Grid_ComputeSpawn_StaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	void Grid_ComputeSpawn_Collection( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	void Grid_ComputeSpawn_CollectionStaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh ,const FWS_VolumeData& WS_Volume);
	UFUNCTION()
	void Grid_ComputeSpawn_Foliage( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize ,const FWS_VolumeData& WS_Volume);
	// Proxy
	UFUNCTION()
	void Grid_Update_Proxy( const TArray<FVector>& Locations );
	UFUNCTION()
	bool Grid_Check_Proxy( const FWS_Grid_Visualisation_Data& Data , const TArray<FVector>& Locations );
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Grid")
	float Grid_ProxyUpdateTime = 1.0f;
	UPROPERTY()
	float Grid_UpdateProxyCounter = 0.0f;
	// Invalid Zone
	UFUNCTION()
	TArray<FWS_Grid_InvalidZone> Grid_MakeInvalidZoneData( const FVector& CellLocation , const double& CellSize , const TArray<AWS_Grid_InvalidSphere*>& Zone );
	UFUNCTION()
	bool Grid_CheckInValidZone( const TArray<FWS_Grid_InvalidZone>& Zone , const FVector& Location );

	// Utils
	FTransform Grid_Math_GetTransformNormalAt( const FVector& Location , const double Size , const double Yaw , const double Z_Offset , const FVector& Forward , const bool& RND_Yaw , const FWS_VolumeData& WS_Volume);
	void Grid_ShuffleFromStream(int32 ArraySize, TArray<int32>& ShuffledIndexes, FRandomStream& RandomStream);
	// SpawnerVolume
	UPROPERTY(Transient)
	TMap<int64,FWS_Grid_SpawnerArray> Grid_SpawnerVolumes;
	UFUNCTION()
	void Grid_RegisterSpawnerVolume( AWorldScape_GridSpawnerVolume* Spawner , FWS_Grid& Coord );
	UFUNCTION()
	void Grid_UnRegisterSpawnerVolume( AWorldScape_GridSpawnerVolume* Spawner , const FWS_Grid& Coord );
	UFUNCTION()
	void Grid_SpawnByVolume( const FWS_Grid& GridCoord , const FWS_Grid_SpawnerVolumeData& Data );
	UFUNCTION()
	void Grid_CheckVolumeDataToLoad( const FWS_Grid& GridCoord );

	// Serialized
	UFUNCTION()
	bool Grid_SerializeActor( AActor* Actor , TArray<uint8>& Serialized );
	UFUNCTION()
	bool Grid_UnSerializeActor( AActor*& Actor , const TArray<uint8>& Serialized );
	UFUNCTION(BlueprintCallable, Category = "WS_Grid")
	bool Grid_StorageActors( TArray<AActor*>& Actors , TMap<int64,FWS_Grid_SerializedData>& StorageData );
	// Load Starage
	UFUNCTION()
	void Grid_UpdateStorage( const TArray<FWS_Grid>& Coords );

	// ------------------------------------
	// Utils
	// ------------------------------------
	UFUNCTION()
	FWS_VolumeData Grid_GetWSVolume( const FWS_Grid& GridCoord );
	UFUNCTION(BlueprintCallable, CallInEditor , Category = "Grid")
	void Clear_Spawned();
	// ------------------------------------
	// Delegate
	// ------------------------------------
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWS_Grid_LoadCell_Delegate , const FWS_Grid& , Cell );
	UPROPERTY(BlueprintAssignable , Category="WS_Grid")
	FWS_Grid_LoadCell_Delegate WS_Grid_LoadCell_Delegate;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWS_Grid_UnLoadCell_Delegate , const FWS_Grid& , Cell );
	UPROPERTY(BlueprintAssignable , Category="WS_Grid")
	FWS_Grid_UnLoadCell_Delegate WS_Grid_UnLoadCell_Delegate;

	UPROPERTY()
	bool Grid_PendingWork = false;
	UFUNCTION(BlueprintCallable, Category = "WS_Grid")
	bool Grid_GetOnPendingLoad();
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWS_Grid_StartPendingLoad);
	UPROPERTY(BlueprintAssignable , Category="WS_Grid")
	FWS_Grid_StartPendingLoad WS_Grid_StartPendingLoad;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWS_Grid_EndPendingLoad);
	UPROPERTY(BlueprintAssignable , Category="WS_Grid")
	FWS_Grid_EndPendingLoad WS_Grid_EndPendingLoad;

	// ------------------------------------
	// Thread
	// ------------------------------------
	UPROPERTY()
	bool Grid_UseThread = true;
	UPROPERTY()
	int32 Grid_NumberOfThread = 4;
	UPROPERTY()
	int32 Grid_NumberOfComputeDataPerSec = 64;
	UPROPERTY()
	float Grid_UpdateComputeDataInterval = 0.5f;
	UPROPERTY()
	float Grid_UpdateComputeData_Count = 0.0f;
	
	EQueuedWorkPriority Grid_ThreadPriority = EQueuedWorkPriority::Normal;
	
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Grid")
	bool Grid_ThreadTimeout = true;
	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Grid")
	float Grid_Timeout = 10.0f;
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Grid", meta = (EditCondition = "false"))
	float Grid_TimeCounter = 0.0f;
	
	TAtomic<bool> Grid_ThreadValidation = true;// OLD
	TQueue<FWS_Grid_SpawnPending,EQueueMode::Mpsc> Queue_Spawn;
	TQueue<FWS_Grid_Queue,EQueueMode::Mpsc> Queue_AsyncCompute;// OLD
	
	TArray<FAsyncTask<GridGenerationThread>*> Grid_ThreadPool;
	TArray<TQueue<FWS_Grid_Queue,EQueueMode::Mpsc>> ArrayQueue_AsyncCompute;
	TArray<TAtomic<bool>> ThreadValidation;
	
	TArray<FWS_Grid_Queue> TempsQueue;
	
	UFUNCTION()
	void TempQueueUpdate( const float& Deltatime )
	{
		Grid_UpdateComputeData_Count += Deltatime;
		if ( Grid_UpdateComputeData_Count >= Grid_UpdateComputeDataInterval )
		{
			Grid_UpdateComputeData_Count = 0.0f;
			
			const int32 MaxSpawn = FMath::Max(Grid_NumberOfComputeDataPerSec * Grid_UpdateComputeDataInterval,1) ;
			if ( !TempsQueue.IsEmpty() )
			{
				const int32 LoopSpawn = FMath::Min(MaxSpawn,TempsQueue.Num());
				for ( int32 i = 0; i < LoopSpawn; i++ )
				{
					if ( Loaded_Cell.Contains(TempsQueue[i].SpawnConf.GridCoord.Hash) )
					{
						Grid_Thread_AddPending( MoveTemp(TempsQueue[i]) , false );
					}
				}
				TempsQueue.RemoveAt( 0 , LoopSpawn );
			}
		}
	}
	
	UPROPERTY()
	int32 Grid_FlipPending = 0;
	UFUNCTION()
	void Grid_FlipNext()
	{
		Grid_FlipPending++;
		Grid_FlipPending = Grid_FlipPending % Grid_NumberOfThread;
	}
	
	UFUNCTION()
	void Grid_Thread_MakePool();
	UFUNCTION()
	void Grid_Thread_StopThreadAndClearQueue( const bool Force = false , const bool Clear = false );
	UFUNCTION()
	bool Grid_Thread_CheckIsDone();
	UFUNCTION()
	void Grid_Thread_AddPending( const FWS_Grid_Queue& Data , const bool Temp = true );
	UFUNCTION()
	void Grid_Thread_CheckAndMake();
	
	// -------------------------------------------------------
	// Grid Hierarchical Foliage
	// -------------------------------------------------------
	/* Sub Divise Grid Component 
	 *
	 * PCG Spawner
	 * Component Spawner and register by Grid
	 * Multi Grid Update
	 * Minimal Grid Size
	 * Maximal GRid Size
	 *
	 * Component Pooling System
	 *
	 * ParallelFor Update
	 * Point Register Array 2D ( with bilinear sampling )
	 *
	 * SubDivise Zone
	 *
	 * Collision Pooling
	 * Defered Collision
	 *
	 * Proxy Billboard
	 * Proxy Grouped
	 * Proxy Imposter ?
	 *
	 * Shadow activation Distance ?
	 * Decal Far Shadow By Render Target ?
	 *
	 * Bilinear WPO Surface blend proxy ?
	 *
	 * Auto Culling by Grid Size ?
	 *
	 * Biome Collection
	 *
	 * Grid Data Biome Blending
	 *
	 * Foliage Representation Data
	 * Foliage Spawner Data
	 *
	 * Locked By Grid Spawner
	 *
	 *	WS_ISM_FoliageComponent
	 *	WS_HISM_FoliageComponent
	 *
	 *	Component :
	 *	- Proxy Mode
	 *	- Collision Validator
	 *	- WPO Static Bilinear Ground
	 *
	 *
	 * DATA :
	 *
	 * WS_GHF_Representation
	 * WS_GHF_Spawner
	 * WS_GHF_Collection
	 * WS_GHF_Biome
	 *
	 * WS_GHF_ISM_Component
	 * WS_GHF_HISM_Component
	 * WS_GHF_CollisionPooling
	 *
	 * WS_GHF_SpawnVolume
	 * WS_GHF_InvalidVolume
	 *
	 * WS_GHF_CellData ( Storage Foliage Component )
	 * WS_GHF_HierarchicalMask
	 *
	 */


	// -------------------------------------------------------
	// Snap Process
	// -------------------------------------------------------
	// If enabled, Snap Actors will use worldscape to manage spawning.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SnapActor")
	bool SnapActor_UsePendingManager = false;
	// time in seconds reserved per tick for spawn.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SnapActor")
	double SnapActor_MaxTimePerTick = 0.0005f;// 0.5ms per tick

	UPROPERTY()
	TArray<AWorldScape_SnapSpawnerActor*> SnapActor_PendingArray;
	UFUNCTION()
	void SnapActor_UpdatePending();
	UFUNCTION()
	void SnapActor_AddActorToPending( AWorldScape_SnapSpawnerActor* SnapActor );
	UFUNCTION()
	void SnapActor_Clear();
	
	UFUNCTION()
	FWS_SnapSpawnData Snap_ComputeCollection( const FTransform& BaseTransform , FRandomStream Random , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume );
	UFUNCTION()
	void Snap_ComputeCollection_ForActor_Async( AWorldScape_SnapSpawnerActor* SnapActor );
	UFUNCTION()
	FWS_VolumeData GetWSVolume_ByBox( const FTransform& Center , const FVector& Extent );

	UFUNCTION()
	void Snap_QuadraticTransform( TArray<FWS_Grid_Actor_Data>& Actors , TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FVector& A , const FVector& B , const FVector& C , const FVector& D );
	// -------------------------------------------------------
	// Pooling Volume
	// -------------------------------------------------------
	// Enable the creation and provision of basic volume in pooling, usable by function.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	bool VolumePooling_Enabled = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	bool VolumePooling_Enabled_OnDedicated = true;
	// Heightmap Pooling
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	TSubclassOf<AHeightMapVolume> Heightmap_Volume_Class = AHeightMapVolume::StaticClass();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Heightmap_Pooling_Minimal = 32;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Heightmap_Pooling_Maximal = 128;
	// Multiply the minimum and maximum number in dedicated server
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	float Heightmap_Pooling_Dedicated_Multiplier = 3.0f;
	UPROPERTY( Transient )
	TArray<AHeightMapVolume*> Pooling_HeightMapVolumes;
	UPROPERTY( Transient )
	TArray<AHeightMapVolume*> Pooling_HeightMapVolumes_Used;
	// Foliage Pooling
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	TSubclassOf<AFoliageMaskVolume> Foliage_Volume_Class = AFoliageMaskVolume::StaticClass();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Foliage_Pooling_Minimal = 32;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Foliage_Pooling_Maximal = 128;
	// Multiply the minimum and maximum number in dedicated server
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	float HFoliage_Pooling_Dedicated_Multiplier = 3.0f;
	UPROPERTY( Transient )
	TArray<AFoliageMaskVolume*> Pooling_Foliage_Volumes;
	UPROPERTY( Transient )
	TArray<AFoliageMaskVolume*> Pooling_Foliage_Volumes_Used;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	bool Pooling_AutoCompute_FoliageMaskLayerAll = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	TArray<int> Pooling_Foliage_MaskAll;
	UFUNCTION(BlueprintCallable, Category = "WS")
	TArray<int> Pooling_GetFoliageMaskLayersAll();
	UFUNCTION()
	void Pooling_ComputeFoliageMask();
	// Hole Pooling
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	TSubclassOf<ATerrainHoleVolume> Hole_Volume_Class = ATerrainHoleVolume::StaticClass();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Hole_Pooling_Minimal = 32;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	int32 Hole_Pooling_Maximal = 128;
	// Multiply the minimum and maximum number in dedicated server
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volume_Pooling")
	float Hole_Pooling_Dedicated_Multiplier = 3.0f;
	UPROPERTY( Transient )
	TArray<ATerrainHoleVolume*> Pooling_HoleVolumes;
	UPROPERTY( Transient )
	TArray<ATerrainHoleVolume*> Pooling_HoleVolumes_Used;
	//
	UFUNCTION()
	AHeightMapVolume* Pooling_Build_HeightMapVolume();
	UFUNCTION(BlueprintCallable, Category = "WS")
	bool Pooling_Take_HeightmapVolume( AHeightMapVolume*& Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_HeightmapVolume( AHeightMapVolume* Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_HeightmapVolume_Multi( UPARAM(ref) TArray<AHeightMapVolume*>& Volumes );
	//
	UFUNCTION()
	AFoliageMaskVolume* Pooling_Build_FoliageMaskVolume();
	UFUNCTION(BlueprintCallable, Category = "WS")
	bool Pooling_Take_FoliageMaskVolume( AFoliageMaskVolume*& Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_FoliageMaskVolume( AFoliageMaskVolume*  Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_FoliageMaskVolume_Multi( UPARAM(ref) TArray<AFoliageMaskVolume*>& Volumes );
	//
	UFUNCTION()
	ATerrainHoleVolume* Pooling_Build_HoleVolume();
	UFUNCTION(BlueprintCallable, Category = "WS")
	bool Pooling_Take_HoleVolume( ATerrainHoleVolume*& Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_HoleVolume( ATerrainHoleVolume* Volume );
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Release_HoleVolume_Multi( UPARAM(ref) TArray<ATerrainHoleVolume*>& Volumes );
	//
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Init();
	UFUNCTION(BlueprintCallable, Category = "WS")
	void Pooling_Clear();
	UFUNCTION(BlueprintCallable, Category = "WS")
	bool WS_Pooling_IsInitialized();
	//
	UPROPERTY(Transient)
	bool WS_Pooling_Initialized = false;
	UPROPERTY(Transient)
	bool WS_Pooling_Dedicated = false;
	
	// -------------------------------------------------------
	// Collision Pooling System
	// -------------------------------------------------------
	// If collision pooling for foliage is enabled , default collision from foliage list is not used , or used for automatique pooling
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Colision_Pooling")
	bool FoliageCollisionPooling_Enabled = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Colision_Pooling")
	double FCP_ActiveDistance = 4000000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Colision_Pooling")
	double FCP_AutoClear_Time = 60.0f;
	
	UFUNCTION(CallInEditor, Category = "Foliage_Colision_Pooling" )
	void AllFoliageToOverrideList();
	UFUNCTION(CallInEditor, Category = "Foliage_Colision_Pooling" )
	void AllFoliageWithCollisionToOverrideList();

private:
	
	UFUNCTION()
	void WS_FCP_Init();
	UFUNCTION()
	void WS_FCP_Clear();
	UFUNCTION()
	void WS_FCP_TickUpdate( const float DeltaSeconds );
	UFUNCTION()
	void WS_FCP_ComputeDistance();

	UPROPERTY()
	bool WS_FCP_IsInit = false;
	UPROPERTY()
	bool WS_FCP_IsRunning = false;
	UPROPERTY()
	bool WS_FCP_OnValidDistance = false;
	UPROPERTY()
	double FCP_Internal_Distance = 0.0f;
	UPROPERTY()
	double FCP_Internal_ValidSquaredDistance = 0.0f;
	UPROPERTY()
	double FCP_Internal_TimeToClearCount = 0.0f;

public:
	// -------------------------------------------------------------------------
	// Settings
	// -------------------------------------------------------------------------
	// Near Settings
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	TMap<UStaticMesh*,FWS_FCP_PoolingData> FCP_NearData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_Near_CheckSize = 6000.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_Near_ForceUpdate_SizeScale = 0.75f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_Near_Minimal_AutoUpdateTime = 2.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_Near_PoolSize_Scale = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_Near_CheckUpdateTime = 0.25f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	int FCP_Near_UpdateCollectionPerFrame = 3;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	float FCP_Near_UpdateTimeLimit = 0.8f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Near_Collision" )
	double FCP_MaxVelocityToCompute_Near = 300000.0f;
	// Far Settings
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	TMap<UStaticMesh*,FWS_FCP_PoolingData> FCP_FarData;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_Far_CheckSize = 30000.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_Far_ForceUpdate_SizeScale = 0.75f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_Far_Minimal_AutoUpdateTime = 4.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_Far_PoolSize_Scale = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_Far_CheckUpdateTime = 0.5f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	int FCP_Far_UpdateCollectionPerFrame = 1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	float FCP_Far_UpdateTimeLimit = 0.8f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling|Far_Collision" )
	double FCP_MaxVelocityToCompute_Far = 600000.0f;
	// limite size for choise to near or far pooling array
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Foliage_Colision_Pooling|Far_Collision")
	double FCP_AutoComputeFar_SectorSizeLimite = 300000.0;
	
	// Settings Global
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	double FCP_VelocityInterpSpeed = 38.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	bool FCP_Override_CollisionSetup = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	FBodyInstance FCP_CollisionSetup;
	// Auto add tags in actor for ohter system
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	TArray<FName> FCP_TagsCollisionActor;
	// Override minimal Scale of instance filter
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	bool FCP_OverrideMinimalScale = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	double FCP_MinimalScale = 0.1;
	// Override all pool size in near and far
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	bool FCP_OverridePoolSize = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	int32 FCP_PoolSize = 100;
	// First filter for ISM component bound
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	double FCP_BoundScale = 2.0f;
	// Velocity multiplier for predicate future location
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	float FCP_PredicateVelocityScale = 0.5f;
	// Separate Compute on multi frame on gamethread or async if Update_Async is true
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	bool FCP_Update_MultiFrame = true;
	// Only on MultiFrame
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Foliage_Colision_Pooling" )
	bool bFCP_Update_Async = true;
	
	// -------------------------------------------------------------------------
	// Make Remove
	// -------------------------------------------------------------------------
	UFUNCTION()
	bool FCP_InitPooling();
	UFUNCTION()
	bool FCP_RemovePooling();
	UFUNCTION()
	void FCP_StartSystem();
	UFUNCTION()
	void FCP_StopSystem();
	// Return Ready
	UFUNCTION()
	void FCP_ComponentReady( UWorldScape_FCP_CollisionComponent* Comp );
	// -------------------------------------------------------------------------
	// Updating Function
	// -------------------------------------------------------------------------
	UFUNCTION()
	void FCP_GetUpdate_Near();
	UFUNCTION()
	void FCP_GetUpdate_Far();
	UFUNCTION()
	bool FCP_GetNeedUpdate( const FVector& NewLocation, const FVector& LastLocation , const double Radius , const double ForceUpdateScale );
	UFUNCTION()
	void FCP_Update( const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling , TMap<UStaticMesh*,FWSFCP_ISM_Collection>& Collection);
	UFUNCTION()
	TArray<UInstancedStaticMeshComponent*> FCP_GetISMComponents_InActor( const FVector& Location ,const double GetRadius , const TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling ,TMap<UStaticMesh*,FWSFCP_ISM_Collection>& Collection);
	UFUNCTION()
	void FCP_Update_Collision( const TArray<UInstancedStaticMeshComponent*>& Instances , const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling );
	// -------------------------------------------------------------------------
	// Timer
	// -------------------------------------------------------------------------
	UFUNCTION()
	void FCP_Start_Near_Timer();
	UFUNCTION()
	void FCP_Stop_Near_Timer();
	UFUNCTION()
	void FCP_Start_Far_Timer();
	UFUNCTION()
	void FCP_Stop_Far_Timer();
	UFUNCTION()
	void FCP_Start_CheckUpdate_Timer_Near();
	UFUNCTION()
	void FCP_Stop_CheckUpdate_Timer_Near();
	UFUNCTION()
	void FCP_Start_CheckUpdate_Timer_Far();
	UFUNCTION()
	void FCP_Stop_CheckUpdate_Timer_Far();

private:
	
	UFUNCTION()
	AWorldScape_FCP_CollisionActor* FCP_CreateActor();
	UPROPERTY(Transient)
	AWorldScape_FCP_CollisionActor* FCP_CollisionActor = nullptr;

	UPROPERTY(Transient)
	TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*> FCP_NearComponents;
	UPROPERTY(Transient)
	TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*> FCP_FarComponents;
	// Location Velocity
	UPROPERTY(Transient)
	FVector FCP_LastNearUpdateLocation;
	UPROPERTY(Transient)
	FVector FCP_LastFarUpdateLocation;
	UPROPERTY(Transient)
	FVector FCP_RawCalculateVelocity;
	UPROPERTY(Transient)
	FVector FCP_Interp_Velocity;
	UPROPERTY(Transient)
	FVector FCP_LastLocationFrame;
	UPROPERTY(Transient)
	FVector FCP_CameraLocation;
	// Timer
	UPROPERTY(Transient)
	FTimerHandle FCP_Near_TimerHandle;
	UPROPERTY(Transient)
	FTimerHandle FCP_Far_TimerHandle;
	UPROPERTY(Transient)
	FTimerHandle FCP_CheckNeedUpdate_Near;
	UPROPERTY(Transient)
	FTimerHandle FCP_CheckNeedUpdate_Far;
	//
	UFUNCTION()
	FVector FCP_GetLocalPlayerCameraLocation();
	UFUNCTION()
	void FCP_Calculate_Velocity( const double DeltaTime );
	// 
	UFUNCTION()
	void FCP_Update_Near();
	UFUNCTION()
	void FCP_Update_Far();
	// Validate
	UPROPERTY(Transient)
	FVector FCP_OwnerLocation = FVector(0, 0, 0);
	UPROPERTY(Transient)
	bool FCP_Near_Valid = false;
	UPROPERTY(Transient)
	bool FCP_Far_Valid = false;
	
	UPROPERTY(Transient)
	TMap<UStaticMesh*,FWSFCP_ISM_Collection> FCP_NearCollection;
	UPROPERTY(Transient)
	TMap<UStaticMesh*,FWSFCP_ISM_Collection> FCP_FarCollection;

	UFUNCTION()
	void FCP_Update_PoolOnTick();
	UFUNCTION()
	void FCP_Update_Collision_SingleMeshCollection( const UStaticMesh* StaticMesh , const TArray<UInstancedStaticMeshComponent*>& Instances , const FVector& Location , const double Radius , TMap<UStaticMesh*,UWorldScape_FCP_CollisionComponent*>& Pooling );
	UFUNCTION()
	void FCP_Update_Async( const TArray<UInstancedStaticMeshComponent*>& Instances , UWorldScape_FCP_CollisionComponent* PoolISM , const FVector& Location , const double Radius );
	UPROPERTY(Transient)
	TArray<UStaticMesh*> FCP_Pool_InAsync;
	
	// Pooling Data
	UFUNCTION()
	void FCP_Build_InternalData();
	UFUNCTION()
	void FCP_ClearInternal_Data();
	UFUNCTION()
	void FCP_AddDataFromFoliage();
	UFUNCTION()
	bool FCP_ComputeNearFar_Asset( const UWorldScapeFoliagesAsset* FoliageAsset );
	UFUNCTION()
	bool FCP_ComputeNearFar_Cluster( const FWorldScapeFoliagesClusterUnit& FoliageClusterUnit , double SectorSize );
	// Internal Data
	UPROPERTY(Transient)
	TMap<UStaticMesh*,FWS_FCP_PoolingData> FCP_Internal_NearData;
	UPROPERTY(Transient)
	TMap<UStaticMesh*,FWS_FCP_PoolingData> FCP_Internal_FarData;

	// -----------------------------------------------------------------
	// Custom Tick Template struct
	// -----------------------------------------------------------------
	struct FCustomTickFunction : public FTickFunction
	{
		AWorldScapeRoot* Target;
		void (AWorldScapeRoot::*TickFunctionPtr)(float);

		virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) override
		{
			if (Target && TickFunctionPtr)
			{
				(Target->*TickFunctionPtr)(DeltaTime);
			}
		}
		virtual FString DiagnosticMessage() override
		{
			return Target->GetFullName() + TEXT("[CustomTick]");
		}
	};
	// Register Base
	FCustomTickFunction WS_Post_Tick;
	
	// Function
	void RegisterSingleTick(FCustomTickFunction& TickFunction, void (AWorldScapeRoot::*FuncPtr)(float), ETickingGroup TickGroup, float TickInterval , bool Enabled);
	// Custom Tick Function
	UFUNCTION()
	void Register_CustomTick();
	UFUNCTION()
	void UnRegister_CustomTick();
	UFUNCTION()
	void WS_Post_TickFunction( float DeltaTime );
	
public:

	// -------------------------------------------------------
	// Gravity
	// -------------------------------------------------------
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	double Gravity_Force = 981.78f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	double Gravity_Planetary_StartFade = 1.25f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	double Gravity_Planetary_EndFadeScale = 1.75f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	double Gravity_Flat_StartFadeDistance = 5000000;//50km
	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	double Gravity_Flat_EndFadeDistance = 10000000;//100km

	UPROPERTY(EditAnywhere,BlueprintReadWrite , Category="Gravity" )
	bool Gravity_ViewDebug = false;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<USphereComponent> Gravity_Sphere_Start = nullptr;
	UPROPERTY()
	TObjectPtr<USphereComponent> Gravity_Sphere_End = nullptr;
	UPROPERTY()
	TObjectPtr<UBoxComponent> Gravity_Box_Start = nullptr;
	UPROPERTY()
	TObjectPtr<UBoxComponent> Gravity_Box_End = nullptr;
#endif

#if WITH_EDITOR
	void Gravity_Editor_Update();
	void Gravity_Editor_Clear();
	
	void Gravity_Editor_CreateSphere();
	void Gravity_Editor_UpdateSphere();
	void Gravity_Editor_DestroySphere();
	
	void Gravity_Editor_CreateBox();
	void Gravity_Editor_UpdateBox();
	void Gravity_Editor_DestroyBox();
	FVector Gravity_Editor_UpdateSize( const bool End );

	FVector Gravity_Editor_ComputeDistance();
#endif
	
	UFUNCTION(BlueprintCallable , Category="Gravity" )
	FVector WS_GetGravityFromLocation( const FVector& Location , FVector& GravityDir , double& Force );
	// -------------------------------------------------------
	// WorldScape
	// -------------------------------------------------------
	
	//Start or stop planet generation, when disabled, the planet is destroyed
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Generate"))
	bool bGenerateWorldScape;
	//Freeze the Planet Generation without destroying the planet
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Freeze Generation"))
	bool bFreezeGeneration;
	//Generate The Ocean (and river/pond if noise is set for)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Generate Ocean"))
	bool bOcean;
	//Radius of the planet in Cm (UE4 units)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Scale", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	float PlanetScale = 637817792;
	// Would like to add World Size limit in future
	//Limit flatworld size
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Scale", meta = (EditCondition = "GenerationType == EWorldScapeType::Flat", DisplayName = "Limit FlatWorld (Experimental)", EditConditionHides))
		bool bLimitFlatWorldSize;
	//Size of the flatworld
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Scale", meta = (EditCondition = "bLimitFlatWorldSize == true", DisplayName = "FlatWorld Size", EditConditionHides))
		FVector2D FlatWorldLimit = FVector2D(1000000, 1000000);
	

	bool bIsInPlay = false;

	double PlanetScaleCode = 637817792;
	
	FVector LastGenerationNormal;
	
	float GridAngle = 5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Material")
	FWSMaterialLodArray TerrainMaterial;
	FWSMaterialLodArray Prev_TerrainMaterial;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Material", meta = (EditCondition = "bOcean == true", EditConditionHides))
	FWSMaterialLodArray OceanMaterial;
	FWSMaterialLodArray Prev_OceanMaterial;
	
	//Project the editor position to where the camera look
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Projected Position (Experimental)"))
	bool bProjectPosition;

	//Foliages List
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Foliages"))
		TArray<UWorldScapeFoliagesCollection*> Foliages;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", AdvancedDisplay, meta = (DisplayName = "Disable Foliages generation on server"))
		bool DisableFoliageDedicatedServer;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", AdvancedDisplay, meta = (DisplayName = "Foliages Minimum Spawn Chance", ToolTip = "Minimum Spawn chance for a foliage to be spawned, can greatly improve foliage spawn rate when increased"))
		float FoliageMinimuMSpawnChanceToNotIgnore = 0.01f;
	
	//Contact Shadow for Terrain
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (DisplayName = "Contact Shadows"))
		bool bContactShadow = false;
	bool Prev_bContactShadow;
	//Override the player 0 position (for visual generation)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Override", meta = (DisplayName = "Override Player Position"))
	bool bOverridePlayerPosition;
	//Overrided Player Position in World Position
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default|Override", meta = (DisplayName = "New Player Position", EditCondition = "bOverridePlayerPosition == true", EditConditionHides))
	FVector OverridedPlayerPosition;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Default", meta = (ToolTip = "Distance from the planet at which the planet highly slowdown the update to prevent performance impact"))
		float DistanceToFreezeGeneration;
	
	//Maximum Lod when in Runtime
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters")
	int32 MaxLod;

	//Number of Polygon on X axis per on Lod 0 , Need to be a multiplier or 4
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters")
	int32 LodResolution;
	
	//Triangle Size in Unit;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters")
	float TriangleSize;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	AActor* DebugObject;
	
	//Maximum Lod when in Runtime
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", meta = (EditCondition = "bOcean == true", EditConditionHides))
	int32 OceanMaxLod;

	//Number of Polygon on X axis per on Lod 0 , Need to be a multiplier of 4
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", meta = (EditCondition = "bOcean == true", EditConditionHides))
	int32 OceanLodResolution;

	//Triangle Size in Unit;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", meta = (EditCondition = "bOcean == true", EditConditionHides))
	float OceanTriangleSize;

	//Generate accurate mesh tangents, it'll give better result when using normal, but greatly slow the generation;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", AdvancedDisplay, meta = (DisplayName = "Generate Accurate Mesh Tangents (Slow)"))
		bool bGenerateTangents;

	//Should Collision have an overlaping quad on each size, prevent weird issues when going from a chunk to another, slighty lower performance
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (DisplayName = "Pad Collisions"), AdvancedDisplay)
		bool bPaddedCollision = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (DisplayName = "RunTime Collision"))
	bool bGenerateCollision = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (DisplayName = "All Collision For All Player", ToolTip = "Generate collision for all Collision dependant actor for all player when on server and not just the server/host player"))
	bool bGenerateCollisionForAllPlayer;
	//bCollision Generation in Editor
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (DisplayName = "Editor Collision"))
	bool bGenerateCollisionInEditor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (EditCondition = "bGenerateCollisionInEditor == true", DisplayName = "Static Editor Collision", EditConditionHides))
	bool bStaticCollisionInEditor;

	//Number of Polygon on X axis per on Lod 0 , Need to be a multiplier of 4
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (EditCondition = "bGenerateCollision == true || bGenerateCollisionInEditor == true", EditConditionHides))
		int32 CollisionResolution;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (EditCondition = "bGenerateCollision == true || bGenerateCollisionInEditor == true", EditConditionHides))
	float CollisionTriangleSize;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (EditCondition = "bGenerateCollision == true || bGenerateCollisionInEditor == true", DisplayName = "Display Collision", EditConditionHides))
	bool bDisplayCollision;

	bool Prev_bDisplayCollision;

	//List of Actor generating collision on WorldScape;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collision", meta = (EditCondition = "bGenerateCollision == true || bGenerateCollisionInEditor == true", EditConditionHides))
	TArray<AActor*> CollisionDependantActor;

	//At which Altitue shall the Triangle Size increase in scale
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters")
	float HeightAnchor;
	//Maximum value of the grid angle at high altitude used by the terrain
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", meta = (DisplayName = "Lod Angle Max (Experimental)"))
	float GridAngleMax = 5;
	//Minimum value of the grid angle at low altitude used by the terrain
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lod Parameters", meta = (DisplayName = "Lod Angle min (Experimental)"))
	float GridAngleMin = 5;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Debug")
	TArray<FVegetation> FoliageDataList;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	TArray<FQueueVegetation> FoliageDataToSpawn;
	
	TAtomic<bool> FoliageGenerationDone;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Debug")
		bool FoliageGenerationInProgress;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, AdvancedDisplay, Category = "Volumes",meta = (DisplayName = "Terrain Hole List (Experimental)"))
		TArray<ATerrainHoleVolume*> TerrainHoleList;
	
	//Noise Properties
	//The Scale of the noise, Higger value = wide details
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	float NoiseScale = 800;
	//Intensity of the noise in cm, Higger value = Higger montains
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	float NoiseIntensity= 1200000;
	//Seed value
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	int32 Seed = 10;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	FRotator LattitudeRotation = FRotator(0,0,0);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	FVector NoiseOffset = FVector(0,0,0);
	//Height of the ocean
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise", meta = (EditCondition = "bOcean == true", EditConditionHides))
	float OceanHeight = 0;
	//Custom Noise Subclass
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise")
	UWorldScapeNoiseClass* WorldScapeNoise = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	bool bUsePlanetaryHeightMap = false;
	bool Prev_bUsePlanetaryHeightMap = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	UHeightMapVolumeData* PlanetaryHeightMap = nullptr;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	EPlanetaryHeightMapBlendType PlanetaryHeightMapBlend = EPlanetaryHeightMapBlendType::Addition;
	EPlanetaryHeightMapBlendType Prev_PlanetaryHeightMapBlend = EPlanetaryHeightMapBlendType::Addition;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet",UIMin = "0", UIMax = "1", EditConditionHides))
	float PlanetaryBlendAlpha = 1;
	float Prev_PlanetaryBlendAlpha = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	bool bPlanetaryInvertMapBlend = false;
	bool Prev_bPlanetaryInvertMapBlend = false;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet", EditConditionHides))
	EHMSamplingMethod SamplingMethod = EHMSamplingMethod::Bilinear;
	EHMSamplingMethod Prev_SamplingMethod = EHMSamplingMethod::Bilinear;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bUsePlanetaryHeightMap == true", EditConditionHides))
	bool bPlanetaryHeight = false;
	bool Prev_bPlanetaryHeight = false;
	//Height of the heightmap (Red Channel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHeight == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryMapHeight = 800000; //8 Km
	float Prev_PlanetaryMapHeight = 800000; //8 Km
	//Base Altitude of the heightmap (Red Channel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHeight == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryAltitude = -1000000; // -10km
	float Prev_PlanetaryAltitude = -1000000; // -10km
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHeight == true && bUsePlanetaryHeightMap == true",UIMin = "0", UIMax = "1", EditConditionHides))
	float HeightBlendAlpha = 1;
	float Prev_HeightBlendAlpha = 1;
	
	float PlanetaryMaxHeight = 0;
	float PlanetaryMinHeight = 0;
	
	//Does it Affect Temperature ?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bUsePlanetaryHeightMap == true", EditConditionHides))
	bool bPlanetaryTemperature = false;
	bool Prev_bPlanetaryTemperature = false;
	//Maximum temperature of the heightmap (Green Chanel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (UIMin = 0, UIMax = 1), meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryTemperature == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryMaxTemperature = 1;
	float Prev_PlanetaryMaxTemperature = 1;
	//Minimum temperature of the heightmap (Green Chanel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (UIMin = 0, UIMax = 1), meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryTemperature == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryMinTemperature = 0;
	float Prev_PlanetaryMinTemperature = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHeight == true && bUsePlanetaryHeightMap == true",UIMin = "0", UIMax = "1", EditConditionHides))
	float TemperatureBlendAlpha = 1;
	float Prev_TemperatureBlendAlpha = 1;
	
	//Does it Affect Humidity ?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bUsePlanetaryHeightMap == true", EditConditionHides))
	bool bPlanetaryHumidity = false;
	bool Prev_bPlanetaryHumidity = false;
	//Maximum Humidity of the heightmap (Blue Chanel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (UIMin = 0, UIMax = 1), meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHumidity == true", EditConditionHides))
	float PlanetaryMaxHumidity = 1;
	float Prev_PlanetaryMaxHumidity = 1;
	//Minimum Humidity of the heightmap (Blue Chanel)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (UIMin = 0, UIMax = 1), meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHumidity == true", EditConditionHides))
	float PlanetaryMinHumidity = 0;
	float Prev_PlanetaryMinHumidity = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryHeight == true && bUsePlanetaryHeightMap == true",UIMin = "0", UIMax = "1", EditConditionHides))
	float HumidityBlendAlpha = 1;
	float Prev_HumidityBlendAlpha = 1;
	
	//Use the HeightMap Alpha ?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bUsePlanetaryHeightMap == true && ( bPlanetaryHeight == true || bPlanetaryHumidity == true || bPlanetaryTemperature == true )", EditConditionHides))
	bool bPlanetaryHeightAlpha = false;
	bool Prev_bPlanetaryHeightAlpha = false;
	
	//Does it Affect Ocean height ?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bUsePlanetaryHeightMap == true", EditConditionHides))
	bool bPlanetaryOceanHeight = false;
	bool Prev_bPlanetaryOceanHeight = false;
	//Maximum Height of the heightmap (Red Channel Second Texture)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryOceanHeight == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryMaxOceanHeight = 0;
	float Prev_PlanetaryMaxOceanHeight = 0;
	//Minimum Height of the heightmap (Alpha Channel Second Texture)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryOceanHeight == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	float PlanetaryMinOceanHeight = 0;
	float Prev_PlanetaryMinOceanHeight = 0;
	//Use the Ocean HeightMap Alpha ?
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Noise|PlanetaryHeightmap", meta = (EditCondition = "GenerationType == EWorldScapeType::Planet && bPlanetaryOceanHeight == true && bUsePlanetaryHeightMap == true", EditConditionHides))
	bool bPlanetaryOceanHeightAlpha = false;
	bool Prev_bPlanetaryOceanHeightAlpha = false;
	
	FDVector2D GetPlanetaryCoordinate(DVector Position) const;

	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetPlanetaryDataFromPosition(FVector Position) const;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	bool bEnableVolumes;

	// Old value is 500
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	float BaseVolumeSize = 500; // Base 500
	// Old Value is 250
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	float BaseVolumeSizeBox = 1000; // Base 250
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	TArray<AHeightMapVolume*> HeightMapVolumeList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	TArray<ANoiseVolume*> NoiseVolumeList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Volumes")
	TArray<AFoliageMaskVolume*> FoliageMaskList;

	TArray<FHeightMapVolumeDataCopy> HeightMapVolumeDataList;
	TArray<FNoiseVolumeData> NoiseVolumeDataList;
	TArray<FFoliageMaskVolumeData> FoliageMaskDataList;
	TArray<FTerrainHoleVolumeData> TerrainHoleDataList;

	//Player Distance to the ground (based on noise and Height map Volumes)
	UPROPERTY(BlueprintReadWrite, Transient, EditAnywhere, Category = "Info" , meta = (EditCondition = "false"))
	float PlayerDistanceToGround;
	//Player Altitudes to the Ocean Level
	UPROPERTY(BlueprintReadWrite, Transient, EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
	float PlayerAltitude;
	//Player Altitudes to the Ocean Level
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
		float TemperatureAtPlayerPosition;
	//Player Altitudes to the Ocean Level
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
		float HumidityAtPlayerPosition;
	//Foliages instance Left to generate in the current sector
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
	int32 FoliageCountLeft;
	//Foliages Sector Left to generate for the current foliage
	UPROPERTY(BlueprintReadWrite, Transient, EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
	int32 FoliageSectorLeft;
	//Foliages Left to generate
	UPROPERTY(BlueprintReadWrite, Transient,EditAnywhere, Category = "Info", meta = (EditCondition = "false"))
	int32 FoliageLeft;
	
	/**
	* Component Used so the Actor keep it's Transform... It's totaly an hack
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	UWorldScapeLod* TransformKeeper= nullptr;
	/*
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Debug")
	TArray<UHierarchicalInstancedStaticMeshComponent*> InstancedMeshs;
	*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Debug")
	TArray<UInstancedStaticMeshComponent*> InstancedMeshs;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Debug")
	TArray<UInstancedStaticMeshComponent*> ISMMeshDebug;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	bool bGenerateFoliages;

	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Code")
	TArray<UWorldScapeLod*> WorldScapeLod;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Code")
	TArray<UWorldScapeLod*> WorldScapeLodOcean;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Code")
	TArray<UWorldScapeLod*> CollisionLods;
	UPROPERTY(BlueprintReadWrite, EditAnywhere,Transient, Category = "Code")
	TMap<UWorldScapeLod*, bool> WorldScapeLodInGeneration;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Debug")
	FVector PlayerPos;

	/**
	* The Function that check for Update on Important parameters change based on buffer parameters
	* @param bUpdate does it update the Buffer ?
	* @return true if the Planet need to be regenerated, false otherwise.
	*/
	bool CheckForRegenerate(const bool bUpdate = true);
	/**
	* The Function that check for HeightMapVolume modifications and regenerate the planet if that the case
	* TODO : Make update only on needed Vertex;
	*/
	void CheckForHeightmapModifier();


	//Position TransformFunctions

	//Convert from RebasedWorld to 'Earth-Centered,Earth-Fixed Coordinate'
	DVector WorldToECEF(const DVector Position);
	/**Convert from RebasedWorld to 'Earth-Centered,Earth-Fixed Coordinate'
	* #WARNING : MAY LACK PRECISION
	**/
	DVector WorldToECEF(const FVector FloatPosition);



	//Convert from 'Earth-Centered,Earth-Fixed Coordinate' to Rebased World
	DVector ECEFToWorld(const  DVector Position);
	/**Convert from 'Earth-Centered,Earth-Fixed Coordinate' to Rebased World
	* #WARNING : MAY LACK PRECISION
	**/
	DVector ECEFToWorld(const  FVector FloatPosition);


	DVector ECEFToProjectedPos(const DVector Position);

	FFoliagesSector GetFoliageSector(const DVector ECEFPosition, const  int32 SectorLod);
	TArray<FFoliagesSector> GetSurroundingFoliageSector(const DVector Position, const  double pSectorSize);

	void GetVolumesData(TArray<FHeightMapVolumeDataCopy>& HeightMapVolumesData,
		TArray<FNoiseVolumeData>& NoiseVolumesData,
		TArray<FFoliageMaskVolumeData>& FoliageMaskVolumesData,
		TArray<FTerrainHoleVolumeData>& TerrainHoleVolumesData,
		const FBox pFBox
	);
	void GetVolumesData(TArray<FHeightMapVolumeDataCopy>& HeightMapVolumesData,
		TArray<FNoiseVolumeData>& NoiseVolumesData,
		TArray<FTerrainHoleVolumeData>& TerrainHoleVolumesData,
		const FBox pFBox
	);
	/**
	* Get the List of HeightMapVolume from position
	* @param Position Position to obtain the Volume from
	* @return List of Volume affecting the noise at Position
	*/
	TArray<AHeightMapVolume*> GetHeightMapVolumeList(const DVector Position);
	/**
	* Get the List of HeightMapVolume from FBox
	* @param pFBox FBox used for Intersect
	* @return List of Volume affecting the noise in the FBox
	*/
	TArray<AHeightMapVolume*> GetHeightMapVolumeList(const FBox pFBox);

	/**
	* Get the List of TerrainHole from position
	* @param Position Position to obtain the NoiseVolume from
	* @return List of TerrainHole affecting the noise at Position
	*/
	TArray<ATerrainHoleVolume*> GetTerrainHoleList(DVector Position);
	/**
	* Get the List of TerrainHole from FBox
	* @param pFBox FBox used for Intersect
	* @return List of TerrainHole affecting the noise in the FBox
	*/
	TArray<ATerrainHoleVolume*> GetTerrainHoleList(FBox pFBox);



	TArray<FHeightMapVolumeDataCopy> GetHeightMapVolumeDataCopyList(const DVector Position);
	TArray<FHeightMapVolumeDataCopy> GetHeightMapVolumeDataCopyList(const FBox pFBox);

	/**
	* Get the List of NoiseVolume from position
	* @param Position Position to obtain the NoiseVolume from
	* @return List of NoiseVolume affecting the noise at Position
	*/
	TArray<ANoiseVolume*> GetNoiseVolumeList(const DVector Position);
	/**
	* Get the List of NoiseVolume from FBox
	* @param pFBox FBox used for Intersect
	* @return List of NoiseVolume affecting the noise in the FBox
	*/
	TArray<ANoiseVolume*> GetNoiseVolumeList(const FBox pFBox);

	TArray<FNoiseVolumeData> GetNoiseVolumeDataList(const DVector Position);
	TArray<FNoiseVolumeData> GetNoiseVolumeDataList(const FBox pFBox);


	/**
	* Get the List of NoiseVolume from position
	* @param Position Position to obtain the NoiseVolume from
	* @return List of NoiseVolume affecting the noise at Position
	*/
	TArray<AFoliageMaskVolume*> GetFoliageMaskList(const DVector Position);
	/**
	* Get the List of NoiseVolume from FBox
	* @param pFBox FBox used for Intersect
	* @return List of NoiseVolume affecting the noise in the FBox
	*/
	TArray<AFoliageMaskVolume*> GetFoliageMaskList(const FBox pFBox);


	TArray<FFoliageMaskVolumeData> GetFoliageMaskVolumeDataList(const DVector Position);
	TArray<FFoliageMaskVolumeData> GetFoliageMaskVolumeDataList(const FBox pFBox);

	/**
	* Get the Noise from Position
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return Noises Data
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	FNoiseData GetNoise(const FVector& Position, const  bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetNoise(const FVector& Position, 
		const  TArray<FNoiseVolumeData>& NoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FTerrainHoleVolumeData>& TerrainHoleListVar,
		const  TArray<FTransform>& VolumeTransformList, const bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetNoise(const DVector& Position, const  bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetNoise(const DVector& Position,
		const  TArray<FNoiseVolumeData>& NoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FTerrainHoleVolumeData>& TerrainHoleListVar,
		const  TArray<FTransform>& VolumeTransformList, const bool Water = false, const bool IgnoreWorldBorder = false);

	FNoiseData GetNoise_HOnly(const DVector& Position,
		const  TArray<FNoiseVolumeData>& NoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FTerrainHoleVolumeData>& TerrainHoleListVar,
		const  TArray<FTransform>& VolumeTransformList, const bool Water = false, const bool IgnoreWorldBorder = false);

	FNoiseData GetNoise_WSVolume( const FVector& Location , const FWS_VolumeData& Volume , const bool Water = false , const bool IgnoreWorldBorder = false);
	/**
	* Get the Noise from Position projected to the ground
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return Noises Data
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	FNoiseData GetGroundNoise(const FVector& Position, const bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetGroundNoise(const FVector& Position, 
		const TArray<FNoiseVolumeData>& NoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FTerrainHoleVolumeData>& TerrainHoleListVar,
		const TArray<FTransform>& VolumeTransformList, const bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetGroundNoise(const DVector& Position, const bool Water = false, const bool IgnoreWorldBorder = false);
	FNoiseData GetGroundNoise(const DVector& Position,
		const TArray<FNoiseVolumeData>& NoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FTerrainHoleVolumeData>& TerrainHoleListVar,
		const TArray<FTransform>& VolumeTransformList, const bool Water = false, const bool IgnoreWorldBorder = false);

	FNoiseData GetGroundNoise_HOnly(const DVector& Position, const bool Water = false ,const bool IgnoreWorldBorder = false);
	TArray<double> GetGroundNoise_Multi_HOnly(const TArray<DVector>& Position, const bool Water = false,const bool IgnoreWorldBorder = false);
	
	/*
	UFUNCTION(BlueprintCallable, Category = "Info")
		FVector GetGroundNormal(FVector ECEFPosition);
	*/

	FVector GetTangentFromECEFPoint(const FVector ECEFPosition);
	FVector GetBiTangentFromECEFPoint(const FVector ECEFPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	FVector GetPawnNormal(const FVector PawnWorldPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	FVector GetPawnSnappedNormal(const FVector PawnWorldPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	FVector GetPawnTangent(const FVector PawnWorldPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	FVector GetPawnBiTangent(const FVector PawnWorldPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetPawnAltitude(const FVector PawnWorldPosition, const bool WaterNoiseHeight = false);
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetPawnDistanceFromGround(const FVector PawnPosition, const bool ECEFCoordinate = false);
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetDistanceFromWater(const FVector Location, const bool ECEFCoordinate = false);
	// Cyberalien
	UFUNCTION(BlueprintCallable, Category = "Info")
	void GetVolumeAtPosition(const FVector Location, TArray<ANoiseVolume*>& NoiseVolume, TArray<AHeightMapVolume*>& HeightMapVolume, TArray<ATerrainHoleVolume*>& TerrainHole);
	UFUNCTION(BlueprintCallable, Category = "Info")
	bool GetLocationInSelectedVolume(const FVector Location, AHeightMapVolume* HeightMapVolume);
	UFUNCTION(BlueprintCallable, Category = "Info")
	bool GetVolumeCanSample(AHeightMapVolume* HeightMapVolume);
	UFUNCTION(BlueprintCallable, Category = "Info")
	bool GetLocationIsInWSVolume(const FVector Location, const FTransform VolumeTransform, const FVector VolumeBoxExtend);
	UFUNCTION(BlueprintCallable, Category = "Info")
	double GetGroundNoiseWithVolume(const FVector Location , TArray<ANoiseVolume*> NoiseVolume, TArray<AHeightMapVolume*> HeightMapVolume, TArray<ATerrainHoleVolume*> TerrainHole);

	float GetPawnDistanceFromGround(const DVector PawnPosition, const bool ECEFCoordinate = false);
	float GetDistanceFromWater(const DVector Location, const bool ECEFCoordinate = false);

	/**
	* Get the Noise Height at set position
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return float
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetGroundHeight(const FVector PawnWorldPosition, const bool Water = false);
	UFUNCTION(BlueprintCallable, Category = "Info")
	double GetGroundHeight_HOnly(const FVector PawnWorldPosition);
	UFUNCTION(BlueprintCallable, Category = "Info")
	TArray<double> GetGroundHeight_HOnly_Multi( const TArray<FVector>& LocationTest );
	/**
	* Get the Noise Normalized Height at set position
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return float
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetGroundHeightNormalize(const FVector PawnWorldPosition, const bool Water = false);


	/**
	* Get the Noise Height at set position
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return float
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetHeight(const FVector PawnWorldPosition, const bool Water = false);
	/**
	* Get the Noise Normalized Height at set position
	* @param Position Position to obtain the Noise From
	* @param Water Is the noise sampled for water or ground
	* @return float
	*/
	UFUNCTION(BlueprintCallable, Category = "Info")
	float GetHeightNormalize(const FVector PawnWorldPosition, const bool Water = false);

	float GetLattitude(const FVector Position);
	double GetLattitude(const DVector Position);

	/**
	* Generate the Base Mesh, used when Check for Regenerate is true
	*/
	void GenerateBaseMesh();
	/**
	* Delete all chunks and mesh instances
	*/
	void CleanComponents();
	//Main Update Function.
	void UpdatePosition();
	//Check if lod generation are done, and update them.
	void CheckForLodGeneration();
	//Foliages Related Ticks
	void FoliageHandleTick();

	void CreateInstancedMesh(FFoliagesSector* Sector, const UWorldScapeFoliagesInterface* Foliage);
	
	void CollisionLodHandler(const FVector HardenNormal, const  double AltitudeMultiplier, const bool ResetCollisions, const double GroundHeight);
	void CheckCollisionDeletion(const FVector HardenNormal, const bool ResetCollisions);
	int32 CreateNewColisionMesh(const DVector RelativePosition);


	// Cy _ Add Blueprint Interface
	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_ForceUpdatePotion()
	{
		UpdatePosition();
	};
	// Force Reset collision for spawn at surface camp
	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_ForceResetCollision()
	{
		for (int32 i = 0; i < CollisionLods.Num(); i++)
		{
			CollisionLods[i]->Mesh->DestroyComponent();
			CollisionLods[i]->DestroyComponent();
		}
		CollisionLods.Empty();
	}

	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_ForceRegenerate_WS( const bool ForceFoliage )
	{
		ForceRegenerate = true;
		ForceRegenerateFoliage = ForceFoliage;
	}
	bool ForceRegenerate = false;
	bool ForceRegenerateFoliage = false;
	
	UFUNCTION(BlueprintCallable, Category = "WS")
	void WS_ForceRegenerate_Foliage()
	{
		ForceRegenerateFoliage = true;
	}
	
	bool init = false;
	bool HMIForceUpdate;

	//Altitude of the player at the last tick
	float LastAltitude;
	//PlayerPosition In WorldSpace
	DVector PlayerWorldPos;
	//Actual Planet Location 
	DVector PlanetLocation;

	TArray<DVector> CollisionActorList;
	TArray<DVector> SnapedCollisionActorList;
	CustomNoise PlanetNoise;
	TArray<UWorldScapeFoliagesInterface*> FoliagesList;

	bool bRegenFoliages = false;

	//Save of previous State of default variable (change lead to a complete new regen)
	EWorldScapeType GenerationType_Previous;
	int32 Prev_MaxLoD;
	int32 Prev_OceanMaxLoD;
	int32 Prev_LodResolution;
	int32 Prev_OceanLodResolution;
	double Prev_PlanetScale;
	float Prev_OceanHeight;
	float Prev_HeightAnchor;
	float Prev_TriangleSize;
	float Prev_OceanTriangleSize;
	UWorldScapeNoiseClass* Prev_WorldScapeNoise;
	bool Prev_Ocean;
	FTransform Prev_PlanetTransform;
	TArray<FWorldScapeFoliagesCollectionData> Prev_Foliages;
	int32 Prev_NoiseScale;
	int32 Prev_NoiseIntensity;
	int32 Prev_Seed;
	FVector Prev_NoiseOffset;
	FRotator Prev_LattitudeRotation;
	FVector Prev_Normal;
	int32 Prev_HeightMult;
	int32 Prev_WaterHeightMult;
	//Check if a rebase is in progress to wait for it before regenerating lod and foliages 
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void OnBeginRebase();
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void OnFinishedRebase();
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rebase")
	bool RebaseInProgress = false;


	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void UpdateTerrainMaterial(const FWSMaterialLodArray pMaterialLodArray);
	UFUNCTION(BlueprintCallable, Category = "Defaults")
	void UpdateOceanMaterial(const FWSMaterialLodArray pMaterialLodArray);


	TArray<FAsyncTask<LodGenerationThread>*> LodGenerationThreadPool;
	FAsyncTask<FoliageGenerationThread>* FoliageGenerationThreadPool = nullptr;
	
	
};




class LodGenerationThread : public FNonAbandonableTask
{
public:
	
	LodGenerationThread(UWorldScapeLod* pWorldScapeLod, AWorldScapeRoot* pRoot, DVector pRelativePosition, DVector pPlanetLocation, FVector2D pSubPosition,
		double pPlanetScale, int32 pLod, float pSize, int32 pVertWidth, double pPlayerAltitude, FVector pHardenNormal, bool pFlatWorld, bool pIsInViewPort);

	~LodGenerationThread();

	//required by ue4, is required 


	bool IsInViewPort;

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NormalThreadTask, STATGROUP_ThreadPoolAsyncTasks);
	}
	UWorldScapeLod* WorldScapeLodRef = nullptr;
	AWorldScapeRoot* RootRef = nullptr;


	LodData MainPatch;
	LodData PatchA;
	LodData PatchB;

	void DoWork();
	void DoWorkMain();
	

	void SetPointPosition();

	//Step 3
	void CalculateNoise();
	void CalculateNormal(LodData &PatchData, int32 PatchID = 0);

	int32 Lod;
	
	DVector OffSetHelper;
	DVector RelativePosition;
	FVector HardenNormal;
	double PlanetScaleCode;

	float Size;
	int32 TriResolution;
	double PlayerDistanceToGround;
	FVector2D SubPosition;

	bool bFlatWorld;
};


class ColisionGeneration
{
public:
	ColisionGeneration(UWorldScapeLod* pWorldScapeLod, AWorldScapeRoot* pRoot, DVector pRelativePosition, DVector pPlanetLocation, FVector2D pSubPosition,
		double pPlanetScale, int32 pLod, float pSize, int32 pVertWidth, double pPlayerAltitude, DVector pHardenNormal, bool pFlatWorld);

	//required by ue4, is required 



	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NormalThreadTask, STATGROUP_ThreadPoolAsyncTasks);
	}
	UWorldScapeLod* WorldScapeLodRef = nullptr;
	AWorldScapeRoot* RootRef = nullptr;


	LodData MainPatch;
	LodData PatchA;
	LodData PatchB;

	void SetPointPosition();
	//Step 3
	void CalculateNoise();

	int32 Lod;

	DVector RelativePosition;
	DVector HardenNormal;
	double PlanetScaleCode;

	float Size;
	int32 TriResolution;
	double PlayerDistanceToGround;
	FVector2D SubPosition;

	bool bFlatWorld;

};


class FoliageGenerationThread : public FNonAbandonableTask
{
public:

	FoliageGenerationThread(AWorldScapeRoot* pRoot,
		const double& pPlanetScale, const bool& pFlatWorld,const DVector& pPlayerPosition ,const TArray< FVegetation>& pVegetationList, const double& pPlayerDistanceToGround);

	~FoliageGenerationThread();

	//required by ue4, is required 

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NormalThreadTask, STATGROUP_ThreadPoolAsyncTasks);
	}
	AWorldScapeRoot* RootRef;

	void DoWork();
	void DoWorkMain();

	bool CheckIfAllFoliageCanBeSpawned(UWorldScapeFoliagesInterface* ActiveFoliage);

	bool ShouldSectorBeSampled(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID);
	bool CheckPositionValidity(UWorldScapeFoliagesInterface* Foliage, UWorldScapeFoliagesCollection* FoliageCollection,
		const DVector& Position, const TArray<FHeightMapVolumeDataCopy>& VolumeListVar, const TArray<FNoiseVolumeData>& NoiseVolumeListVar,
		const TArray<FTerrainHoleVolumeData>& TerrainHoleVolumeListVar, const TArray<FTransform>& VolumeTransformArray
	);

	void GenerateVegetationForSector(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID);
	void GenerateVegetationForSector_Para(FFoliagesSector& Sector, int32 FoliageCollectionID, int32 FoliageID, int32 FLID);
	
	DVector RelativePosition;
	DVector PlayerPos;
	double PlanetScaleCode;

	bool bFlatWorld;
	
	TArray< FVegetation> VegetationList;

	double PlayerDistanceToGround;
};
// Grid Thread Declaration
class GridGenerationThread : public FNonAbandonableTask
{
	//friend class FAutoDeleteAsyncTask<GridGenerationThread>;
	TQueue<FWS_Grid_Queue,EQueueMode::Mpsc>& TaskQueue;
	TAtomic<bool>& bShouldRun;
	
	TQueue<FWS_Grid_SpawnPending,EQueueMode::Mpsc>& SpawnQueue;
	
	public:
	GridGenerationThread(AWorldScapeRoot* WSRoot , TQueue<FWS_Grid_Queue,EQueueMode::Mpsc>& inTaskQueue , TAtomic<bool>& inbShouldRun , TQueue<FWS_Grid_SpawnPending,EQueueMode::Mpsc>& InSpawnQueue )
		: TaskQueue(inTaskQueue), bShouldRun(inbShouldRun) , SpawnQueue(InSpawnQueue) , RootRef(WSRoot){}

	virtual ~GridGenerationThread()
	{};

	AWorldScapeRoot* RootRef;
	
	virtual void DoWork();
	
	void GridThread_ComputeSpawn_Actor( const FWS_Grid_SpawnConf& SpawnConf , const TArray<FWS_Grid_Spawner_Actor>& Actors , const FWS_VolumeData& WSVolume);
	void GridThread_ComputeSpawn_StaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume);
	void GridThread_ComputeSpawn_Collection( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_Actor_Data>& Actors , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume);
	void GridThread_ComputeSpawn_CollectionStaticMesh( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const FWS_VolumeData& WSVolume);
	void GridThread_ComputeSpawn_Foliage( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize , const FWS_VolumeData& WSVolume);
	void GridThread_ComputeSpawn_Foliage_Para( const FWS_Grid_SpawnConf& SpawnConf  , const TArray<FWS_Grid_StaticMesh_Data>& StaticMesh , const double& FoliageSectorSize , const FWS_VolumeData& WSVolume);
	
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NormalThreadTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
/*
// Grid Worker Thread
class Grid_WorkThread : public FRunnableThread
{
public:
	Grid_WorkThread(AWorldScapeRoot* WSRoot);
	virtual ~Grid_WorkThread() override;
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	virtual void Suspend();
	virtual void Resume();
	virtual void Sleep();
	virtual void WakeUp();
	virtual void WakeUpAsync();
	virtual void SleepAsync();
	virtual void Exit();
	virtual void ExitAsync();
	virtual void SuspendAsync();
	virtual void ResumeAsync();
	virtual void SleepAsyncAsync();
	virtual void ExitAsyncAsync();
	virtual void WakeUpAsyncAsync();
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NormalThreadTask, STATGROUP_ThreadPoolAsyncTasks);
	}
	
protected:
	
	AWorldScapeRoot* RootRef;
	
private:

	FRunnableThread* Thread;
	bool bRunThread;
	
};
*/

