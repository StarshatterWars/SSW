// Copyright 2024 IOLACORP STUDIO. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "IntVectorTypes.h"
#include "Engine/StaticMesh.h"

#include "WorldScapeVolume/Public/TerrainHoleVolumeData.h"
#include "WorldScapeVolume/Public/NoiseVolumeData.h"
#include "WorldScapeVolume/Public/HeightMapVolumeDataCopy.h"
#include "WorldScapeVolume/Public/FoliageMaskVolumeData.h"

#include "WorldScape_GridStruct.generated.h"




class AWorldScape_GridSpawnerVolume;
class AWorldScape_GridSMProxy;
class UWorldScape_GridDataAsset;

// -------------------------------------------------------
// 
// -------------------------------------------------------
USTRUCT()
struct FWS_Grid_VectorInt2D
{
	GENERATED_BODY()
	UPROPERTY()
	int32 X = 0;
	UPROPERTY()
	int32 Y = 0;
	FWS_Grid_VectorInt2D() = default;
	FWS_Grid_VectorInt2D( const int32& inX , const int32& inY )
		: X( inX ) , Y( inY )
	{}
};
// -------------------------------------------------------
// Static Data
// -------------------------------------------------------
struct FWS_Grid_AdjacentFaces
{
	int32 Top = 0;
	int32 Right = 0;
	int32 Bottom = 0;
	int32 Left = 0;
};
// Cube Face Adjacent Index , Contains Face alignement
const FWS_Grid_AdjacentFaces WS_Grid_AdjacentFacesLookup[6] = {
	{4, 2, 5, 3},  // x+ (0)
	{4, 3, 5, 2},  // x- (1)
	{4, 1, 5, 0},  // y+ (2)
	{4, 0, 5, 1},  // y- (3)
	{3, 2, 1, 0},  // z+ (4)
	{1, 2, 3, 0}   // z- (5)

	/*
	{4, 2, 5, 3},  // x+ (0)
	{4, 3, 5, 2},  // x- (1)
	{4, 1, 5, 0},  // y+ (2)
	{4, 0, 5, 1},  // y- (3)
	{3, 2, 1, 0},  // z+ (4)
	{1, 2, 3, 0}   // z- (5)
	*/
};
// -----------------------------------------------
const FVector WS_Grid_AxisFaceLookup[6] = {
		{0.0f, 0.0f, -1.0f},   // x+ (0) - axe X local pointe vers -Z
	   {0.0f, 0.0f, 1.0f},    // x- (1) - axe X local pointe vers +Z
	   {1.0f, 0.0f, 0.0f},    // y+ (2) - axe X local pointe vers +X
	   {1.0f, 0.0f, 0.0f},    // y- (3) - axe X local pointe vers +X
	   {1.0f, 0.0f, 0.0f},    // z+ (4) - axe X local pointe vers +X
	   {1.0f, 0.0f, 0.0f}     // z- (5) - axe X local pointe vers +X
};
// Kernel Index for get adjacent cell in vector 2D
const FWS_Grid_VectorInt2D  WS_Grid_Vec2DKernelIndex[8] = {
	{FWS_Grid_VectorInt2D(-1,-1)},
	{FWS_Grid_VectorInt2D(1,1)},
	{FWS_Grid_VectorInt2D(-1,1)},
	{FWS_Grid_VectorInt2D(1,-1)},
	{FWS_Grid_VectorInt2D(0,-1)},
	{FWS_Grid_VectorInt2D(-1,0)},
	{FWS_Grid_VectorInt2D(0,1)},
	{FWS_Grid_VectorInt2D(1,0)}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Proxy
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Proxy")
	bool UseProxy = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Proxy")
	TSoftObjectPtr<UStaticMesh> ProxyMesh = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Proxy")
	double ProxySwitchDistance = 100000; // 1km

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Proxy")
	bool Use_AdditionalComponent  = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Proxy")
	TSoftClassPtr<UActorComponent> AdditionalComponentClass;
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Placement_Data
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	FTransform Transform = FTransform();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	bool SnapGround = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	bool RotateToGround = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	bool RND_Yaw = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	FVector2D Yaw_Rotate = FVector2D(1.0f,360.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	float Z_Offset = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	bool Use_DiskRND = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	float DiskRND_Size = 400.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	bool RND_LinearScale = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Placement")
	FVector2D LinearScale = FVector2D(0.5f,1.5f);
	
	FWS_Grid_Placement_Data() = default;
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Spawner_DataAsset
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	float SpawnScale = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Data")
	UWorldScape_GridDataAsset* GridDataAsset = nullptr;
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Spawner_Actor
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	float SpawnableChance = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TSoftClassPtr<AActor> Class;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	FWS_Grid_Proxy ProxyData = FWS_Grid_Proxy();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	bool UseSerializeData = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TArray<uint8> SerializedData;
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Spawner_StaticMesh
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	float SpawnableChance = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool Collision = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool CastShadow = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	FWS_Grid_Proxy ProxyData = FWS_Grid_Proxy();
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Actor_Data
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	float SpawnableChance = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TSoftClassPtr<AActor> Class = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	FVector ActorBound = FVector(1.0f,1.0f,1.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	FBoxSphereBounds ActorBounds = FBoxSphereBounds(1);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	FWS_Grid_Placement_Data Placement = FWS_Grid_Placement_Data();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	bool UseSerializeData = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TArray<uint8> SerializedData;
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_StaticMesh_Data
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	float SpawnableChance = 1.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	FVector MeshBound = FVector(1.0f,1.0f,1.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	FBoxSphereBounds MeshBounds = FBoxSphereBounds(1);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool Collision = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool CastShadow = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	FWS_Grid_Placement_Data Placement = FWS_Grid_Placement_Data();
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Actor_SpawnData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	UClass* SpawnableClass = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TSoftClassPtr<AActor> SoftClass = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	FTransform SpawnableTransform = FTransform();
	bool UseSerialize = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Actor")
	TArray<uint8> SerializedData;
	FWS_Grid_Actor_SpawnData() = default;
	FWS_Grid_Actor_SpawnData( UClass* inClass , const FTransform& inTransform )
	{
		SpawnableClass = inClass;
		SpawnableTransform = inTransform;
	}
	FWS_Grid_Actor_SpawnData( const TSoftClassPtr<AActor>& inClass , const FTransform& inTransform , const FWS_Grid_Actor_Data& inData )
	{
		SoftClass = inClass;
		SpawnableTransform = inTransform;
		if ( inData.UseSerializeData && !inData.SerializedData.IsEmpty() )
		{
			UseSerialize = true;
			SerializedData = inData.SerializedData;
		}
	}
	FWS_Grid_Actor_SpawnData( const TSoftClassPtr<AActor>& inClass , const FTransform& inTransform , const FWS_Grid_Spawner_Actor& inData )
	{
		SoftClass = inClass;
		SpawnableTransform = inTransform;
		if ( inData.UseSerializeData && !inData.SerializedData.IsEmpty() )
		{
			UseSerialize = true;
			SerializedData = inData.SerializedData;
		}
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Static_SpawnData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	UStaticMesh* SpawnableStaticMesh = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	TSoftObjectPtr<UStaticMesh> SoftStaticMesh = nullptr;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	FTransform SpawnableTransform = FTransform();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool Collision = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "StaticMesh")
	bool CastShadow = true;
	FWS_Grid_Static_SpawnData() = default;
	FWS_Grid_Static_SpawnData( UStaticMesh* inStaticMesh, const FTransform& inTransform , const bool inCollision )
	{
		SpawnableStaticMesh = inStaticMesh;
		SpawnableTransform = inTransform;
		Collision = inCollision;
	}
	FWS_Grid_Static_SpawnData( const TSoftObjectPtr<UStaticMesh>& inStaticMesh, const FTransform& inTransform , const bool inCollision , const bool inCastShadow )
	{
		SoftStaticMesh = inStaticMesh;
		SpawnableTransform = inTransform;
		Collision = inCollision;
		CastShadow = inCastShadow;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Collection_SpawnData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Collection")
	FTransform BaseTransform = FTransform();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Collection")
	TArray<FWS_Grid_Actor_SpawnData> Actors;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Collection")
	TArray<FWS_Grid_Static_SpawnData> StaticMeshes;
	FWS_Grid_Collection_SpawnData() = default;
	FWS_Grid_Collection_SpawnData( const FTransform& inTransform , const TArray<FWS_Grid_Actor_SpawnData>& inActors , const TArray<FWS_Grid_Static_SpawnData>& inStaticMeshes )
	{
		BaseTransform = inTransform;
		Actors = inActors;
		StaticMeshes = inStaticMeshes;
	}
	FWS_Grid_Collection_SpawnData( const FTransform& inTransform , const TArray<FWS_Grid_Static_SpawnData>& inStaticMeshes )
	{
		BaseTransform = inTransform;
		StaticMeshes = inStaticMeshes;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Actor_SpawnData_Viz
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Viz")
	FWS_Grid_Actor_SpawnData Data = FWS_Grid_Actor_SpawnData();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Viz")
	FWS_Grid_Proxy ProxyData = FWS_Grid_Proxy();
	FWS_Grid_Actor_SpawnData_Viz() = default;
	FWS_Grid_Actor_SpawnData_Viz(  UClass* inClass , const FTransform& inTransform , const FWS_Grid_Proxy& Viz , const FWS_Grid_Actor_Data& inData )
	{
		Data = FWS_Grid_Actor_SpawnData(inClass, inTransform,inData);
		ProxyData = Viz;
	}
	FWS_Grid_Actor_SpawnData_Viz(  UClass* inClass , const FTransform& inTransform , const FWS_Grid_Proxy& Viz , const FWS_Grid_Spawner_Actor& inData )
	{
		Data = FWS_Grid_Actor_SpawnData(inClass, inTransform,inData);
		ProxyData = Viz;
	}
	
	FWS_Grid_Actor_SpawnData_Viz( const TSoftClassPtr<AActor>& inClass , const FTransform& inTransform , const FWS_Grid_Proxy& Viz , const FWS_Grid_Actor_Data& inData )
	{
		Data = FWS_Grid_Actor_SpawnData(inClass, inTransform,inData);
		ProxyData = Viz;
	}
	FWS_Grid_Actor_SpawnData_Viz( const TSoftClassPtr<AActor>& inClass , const FTransform& inTransform , const FWS_Grid_Proxy& Viz , const FWS_Grid_Spawner_Actor& inData )
	{
		Data = FWS_Grid_Actor_SpawnData(inClass, inTransform,inData);
		ProxyData = Viz;
	}
};
// -------------------------------------------------------
// 
// -------------------------------------------------------

// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SizeData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector A = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector B = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector C = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector D = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	double CircleInternalSize = 0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector BoxExtend = FVector(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "SizeData")
	FVector XAxis = FVector(1.0,0.0,0.0);
	
	FWS_Grid_SizeData() = default;
	FWS_Grid_SizeData( const FVector& inA , const FVector& inB , const FVector& inC , const FVector& inD , const FVector& inCircleInternalSize , const FVector& inBoxExtend )
	{
		A = inA;
		B = inB;
		C = inC;
		D = inD;
		BoxExtend = inCircleInternalSize;
		CircleInternalSize = 0;
	}
	FVector GetCenterInCell( const FVector2D& Offset )
	{
		return FMath::Lerp( FMath::Lerp(A,B,Offset.X) , FMath::Lerp(D,C,Offset.X) , Offset.Y );
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int32 X = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int32 Y = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	uint8 Face = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int32 Depth = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	FVector CellCenterLocation = FVector(0,0,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	FVector CellCenterNormal = FVector(0,0,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	FWS_Grid_SizeData SizeData = FWS_Grid_SizeData();
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int64 Hash = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int32 FlatIndex = 0;
	
	FWS_Grid() = default;
	FWS_Grid( const int32 ix, const int32 iy , const uint8 iface , const int32 idepth )
	{
		X = ix;
		Y = iy;
		Face = iface;
		Depth = idepth;
	}
	FWS_Grid( const int32 ix, const int32 iy , const uint8 iface , const int32 idepth , const FVector& BaseLocation , const double& Radius )
	{
		X = ix;
		Y = iy;
		Face = iface;
		Depth = idepth;
		CellCenterLocation = CenterLocation( BaseLocation , Radius , CellCenterNormal );
		ComputeGridSizeData( BaseLocation , Radius );
		Hash = HashCoord();
		FlatIndex = Get_FlatIndex();
	}
	FWS_Grid( const FVector& BaseLocation , const FVector& Location , const int32 inDepth , const double& Radius )
	{
		Hash = MakeGrid( BaseLocation , Location , inDepth , Radius );
	}
	int64 HashCoord() const
	{
		return  ((int64)(X & 0xFFFFF) |          // 20 bits pour X
				((int64)(Y & 0xFFFFF) << 20) |   // 20 bits pour Y
				((int64)Face << 40));            // 8 bits pour Face
	}
	void FromHash( const int64& inHash )
	{
		X = (int32)(inHash & 0xFFFFF);
		Y = (int32)((inHash >> 20) & 0xFFFFF);
		Face = (uint8)((inHash >> 40) & 0xFF);
	}
	static void DecodeHash(const int64& inHash, int32& OutX, int32& OutY, uint8& OutFace)
	{
		OutX = (int32)(inHash & 0xFFFFF);
		OutY = (int32)((inHash >> 20) & 0xFFFFF);
		OutFace = (uint8)((inHash >> 40) & 0xFF);
	}
	int64 MakeGrid( const FVector& BaseLocation , const FVector& Location , const int32 inDepth , const double& Radius )
	{
		Depth = inDepth;
		
		const FVector Direction = (Location - BaseLocation).GetSafeNormal();
		const double AbsX = FMath::Abs(Direction.X);
		const double AbsY = FMath::Abs(Direction.Y);
		const double AbsZ = FMath::Abs(Direction.Z);
		const double MaxComponent = FMath::Max3(AbsX, AbsY, AbsZ);
		// Facteur de conversion pour transformer les coordonnées [-1,1] en indices de grille
		const int32 HalfDepth = (inDepth / 2);

		if (MaxComponent == AbsX)
		{
			if (FMath::Sign(Direction.X) < 0)
			{
				Face = 1;
				// Convertir de [-1,1] à [-HalfDepth,HalfDepth]
				X = FMath::RoundToInt((Direction.Y/AbsX) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Z/AbsX) * HalfDepth);
			}
			else
			{
				Face = 0;
				X = FMath::RoundToInt((Direction.Y/AbsX) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Z/AbsX) * HalfDepth);
			}
			CellCenterLocation = CenterLocation( BaseLocation , Radius , CellCenterNormal);
			ComputeGridSizeData( BaseLocation , Radius );
			FlatIndex = Get_FlatIndex();
			return HashCoord();
		}
		else if (MaxComponent == AbsY)
		{
			if (FMath::Sign(Direction.Y) < 0)
			{
				Face = 3;
				X = FMath::RoundToInt((Direction.X/AbsY) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Z/AbsY) * HalfDepth);
			}
			else
			{
				Face = 2;
				X = FMath::RoundToInt((Direction.X/AbsY) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Z/AbsY) * HalfDepth);
			}
			CellCenterLocation = CenterLocation( BaseLocation , Radius , CellCenterNormal);
			ComputeGridSizeData( BaseLocation , Radius );
			FlatIndex = Get_FlatIndex();
			return HashCoord();
		}
		else
		{
			if (FMath::Sign(Direction.Z) < 0)
			{
				Face = 5;
				X = FMath::RoundToInt((Direction.X/AbsZ) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Y/AbsZ) * HalfDepth);
			}
			else
			{
				Face = 4;
				X = FMath::RoundToInt((Direction.X/AbsZ) * HalfDepth);
				Y = FMath::RoundToInt((Direction.Y/AbsZ) * HalfDepth);
			}
			
			CellCenterLocation = CenterLocation( BaseLocation , Radius , CellCenterNormal);
			ComputeGridSizeData( BaseLocation , Radius );
			FlatIndex = Get_FlatIndex();
			return HashCoord();
		}
	}
	FVector CenterLocation( const FVector& BaseLocation , const double& Radius , FVector& Normal ) const
	{
		const int32 HalfDepth = (Depth / 2);
		
		double NormalizedX = ((static_cast<double>(X)+0.5)/HalfDepth);
		double NormalizedY = ((static_cast<double>(Y)+0.5)/HalfDepth);
		
		const double Size = 1.0f;
		
		FVector Direction;
		switch (Face)
		{
			case 0: Direction = FVector(Size, NormalizedX, NormalizedY); break;
			case 1: Direction = FVector(-Size, NormalizedX, NormalizedY); break;
			case 2: Direction = FVector(NormalizedX, Size, NormalizedY); break;
			case 3: Direction = FVector(NormalizedX, -Size, NormalizedY); break;
			case 4: Direction = FVector(NormalizedX, NormalizedY, Size); break;
			case 5: Direction = FVector(NormalizedX, NormalizedY, -Size); break;
		default:
			UE_LOG(LogTemp, Error, TEXT("Invalid Face value: %d"), Face);
			Direction = FVector::ZeroVector; // Valeur par défaut
			break;
		}
		Normal = Direction.GetSafeNormal();
		return BaseLocation + (Direction.GetSafeNormal()*Radius);
	}
	FVector InGridLocation( const FVector& BaseLocation , const double& Radius , FVector& Normal , const float& InGridX , const float& InGridY ) const
	{
		const int32 HalfDepth = (Depth / 2) ;
		
		double NormalizedX = ((static_cast<double>(X)+InGridX)/HalfDepth);
		double NormalizedY = ((static_cast<double>(Y)+InGridY)/HalfDepth);
		
		const double Size = 1.0f;
		
		FVector Direction;
		switch (Face)
		{
		case 0: Direction = FVector(Size, NormalizedX, NormalizedY); break;
		case 1: Direction = FVector(-Size, NormalizedX, NormalizedY); break;
		case 2: Direction = FVector(NormalizedX, Size, NormalizedY); break;
		case 3: Direction = FVector(NormalizedX, -Size, NormalizedY); break;
		case 4: Direction = FVector(NormalizedX, NormalizedY, Size); break;
		case 5: Direction = FVector(NormalizedX, NormalizedY, -Size); break;
		default:
			UE_LOG(LogTemp, Error, TEXT("Invalid Face value: %d"), Face);
			Direction = FVector::ZeroVector; // Valeur par défaut
			break;
		}
		Normal = Direction.GetSafeNormal();
		return BaseLocation + (Direction.GetSafeNormal()*Radius);
	}
	void ComputeGridSizeData( const FVector& BaseLocation , const double& Radius )
	{
		FVector Normal;
		SizeData.A = InGridLocation( BaseLocation , Radius , Normal , 0.0f , 0.0f );
		SizeData.B = InGridLocation( BaseLocation , Radius , Normal , 1.0f , 0.0f );
		SizeData.C = InGridLocation( BaseLocation , Radius , Normal , 1.0f , 1.0f );
		SizeData.D = InGridLocation( BaseLocation , Radius , Normal , 0.0f , 1.0f );

		// Cheap version
		double A = (CellCenterLocation-SizeData.A).SquaredLength();
		double B = (CellCenterLocation-SizeData.B).SquaredLength();
		double C = (CellCenterLocation-SizeData.C).SquaredLength();
		double D = (CellCenterLocation-SizeData.D).SquaredLength();
		//SizeData.CircleInternalSize = FMath::Sqrt( FMath::Min(FMath::Min(A, B),FMath::Min(C, D)))*0.75;

		// Alternative calcule
		auto PointToLineDistance = [](const FVector& P, const FVector& P1, const FVector& P2) -> double
		{
			FVector LineVector = P2 - P1;
			FVector PointVector = P - P1;

			double LineLength = LineVector.Size();
			if (LineLength < KINDA_SMALL_NUMBER) // Gestion des tolérances
			{
				return 0.0; // Si la ligne est dégénérée, distance = 0
			}

			double CrossProductMagnitude = FVector::CrossProduct(LineVector, PointVector).Size();
			return CrossProductMagnitude / LineLength;
		};

		// Calculer les distances aux 4 côtés
		double DistanceToAB = PointToLineDistance(CellCenterLocation, SizeData.A, SizeData.B);
		double DistanceToBC = PointToLineDistance(CellCenterLocation, SizeData.B, SizeData.C);
		double DistanceToCD = PointToLineDistance(CellCenterLocation, SizeData.C, SizeData.D);
		double DistanceToDA = PointToLineDistance(CellCenterLocation, SizeData.D, SizeData.A);

		//SizeData.CircleInternalSize = FMath::Min(FMath::Min(DistanceToAB, DistanceToBC),FMath::Min(DistanceToCD, DistanceToDA));
		SizeData.CircleInternalSize = FMath::Max(FMath::Max(DistanceToAB, DistanceToBC),FMath::Max(DistanceToCD, DistanceToDA));

		
		SizeData.XAxis = WS_Grid_AxisFaceLookup[Face];
	}
	void ComputeFaceIndex( int32& InX, int32& InY , const int32& inDepth , uint8& InFace ) const
	{
		const int32 HalfDepth = inDepth / 2;
		if ( InX < -HalfDepth )
		{
			InX = HalfDepth;
			InFace = WS_Grid_AdjacentFacesLookup[InFace].Left;
		}
		else if ( InX > HalfDepth )
		{
			InX = -HalfDepth;
			InFace = WS_Grid_AdjacentFacesLookup[InFace].Right;
		}
		else if ( InY < -HalfDepth )
		{
			InY = HalfDepth;
			InFace = WS_Grid_AdjacentFacesLookup[InFace].Top;
		}
		else if ( InY > HalfDepth )
		{
			InY = -HalfDepth;
			InFace = WS_Grid_AdjacentFacesLookup[InFace].Bottom;
		}
	}
	TArray<FWS_Grid> Get_Adjacent_Cells( const FVector& BaseLocation , const double& Radius ) const
	{
		TArray<FWS_Grid> Local;
		for ( const auto& Kernel : WS_Grid_Vec2DKernelIndex )
		{
			int32 CellX = X + Kernel.X;
			int32 CellY = Y + Kernel.Y;
			uint8 inFace = Face;
			ComputeFaceIndex( CellX, CellY , Depth , inFace );
			Local.Add( FWS_Grid( CellX, CellY , inFace , Depth , BaseLocation , Radius ));
		}
		return Local;
	}
	int32 Get_FlatIndex() const
	{
		const int32 HalfDepth = Depth / 2;
		const int32 CoordX = X + HalfDepth;
		const int32 CoordY = Y + HalfDepth;
		const int32 BaseIndex = Face*Depth*Depth;
		return BaseIndex + (CoordY*Depth) + CoordX;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Value
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	FWS_Grid GridData = FWS_Grid();
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	bool OverrideDataAsset = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	bool AdditionalDataAsset = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	TArray<UWorldScape_GridDataAsset*> GridDataAsset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	float SpawnScale = 1.0f;
	FWS_Grid_Value() = default;
	FWS_Grid_Value( const FWS_Grid& Coord )
	{
		GridData = Coord;
	}
	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool OverrideFoliageList = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AdditionalDataFoliageList = false;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TArray<UWorldScapeFoliagesCollection*> FoliagesList;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FoliageSpawnScale = 1.0f;
	*/
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_Visualisation_Data
{
	GENERATED_BODY()
	UPROPERTY()
	bool Switch_Visualisation = false;
	UPROPERTY()
	bool IsVisible = true;
	UPROPERTY()
	FVector Location = FVector(0,0,0);
	UPROPERTY()
	double VisualisationSizeSquared = 4000.0f;

	FWS_Grid_Visualisation_Data() = default;
	FWS_Grid_Visualisation_Data( const FVector& inLocation , const double& inSize )
	{
		Location = inLocation;
		VisualisationSizeSquared = inSize*inSize;
	}
	bool GetVisualisation( const FVector& VizLocation ) const
	{
		if ( Switch_Visualisation )
		{
			return (VizLocation-Location).SquaredLength() < VisualisationSizeSquared;
		}
		return true;
	}
	bool GetVisualisation( const TArray<FVector>& VizLocation ) const
	{
		if ( Switch_Visualisation )
		{
			for ( const auto& Viz : VizLocation )
			{
				if ( (Viz-Location).SquaredLength() < VisualisationSizeSquared )
				{
					return true;
				}
			}
			return false;
		}
		return true;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Instance
{
	GENERATED_BODY()
	UPROPERTY()
	FWS_Grid_Visualisation_Data Visibility = FWS_Grid_Visualisation_Data();
	UPROPERTY()
	TArray<AActor*> Actors_Instance;
	UPROPERTY()
	AWorldScape_GridSMProxy* SM_Proxy = nullptr;
	
	UPROPERTY()
	TArray<FWS_Grid_Actor_SpawnData> ActorData;
	UPROPERTY()
	TArray<FWS_Grid_Static_SpawnData> StaticMeshData;

	FWS_Grid_Instance() = default;
	FWS_Grid_Instance( const FWS_Grid_Visualisation_Data& inViz )
	{
		Visibility = inViz;
	}
	void Add_ActorData( const TArray<FWS_Grid_Actor_SpawnData>& inData )
	{
		ActorData = inData;
	}
	void Add_StaticMeshData( const TArray<FWS_Grid_Static_SpawnData>& inData )
	{
		StaticMeshData = inData;
	}
	FWS_Grid_Instance( AActor* inActor )
	{
		Actors_Instance = { inActor };
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_Container
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	FWS_Grid Coord = FWS_Grid();
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Grid")
	TArray<FWS_Grid_Instance> Instances;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	bool OnLoad = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	int32 OnPendingLoad = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = "Grid")
	TSet<int64> InstanceSeed;
	
	FWS_Grid_Container() = default;
	FWS_Grid_Container( const FWS_Grid& Data )
	{
		Coord = Data;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SpawnParams
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MinElevation = -200000.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MaxElevation = 200000.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MinTemperature = -100.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MaxTemperature = 100.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MinHumidity = 0.5f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MaxHumidity = 1.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float MaxSlope = 0.1f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	bool CheckDifference = true;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float Max_ElevationDifference = 650.0f;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	bool SpawnableInWater = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	bool RotateToGround = true;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	double InvalidSize = 85000.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	double SpawnSize = 450.0f;
	// PoissonDisc Sampling
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float PoissonDiscDensity = 20;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Spawn")
	float PoissonDiscDensityVariation = 25;
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_InvalidZone
{
	GENERATED_BODY()
	UPROPERTY()
	FVector Location = FVector(0,0,0);
	UPROPERTY()
	double Size = 0;

	FWS_Grid_InvalidZone() = default;
	FWS_Grid_InvalidZone( const FVector& inLocation , const double& inSize )
	{
		Location = inLocation;
		Size = inSize;
	}
};

// -----------------------------------------------
UENUM()
enum EWS_Grid_SpawnType : uint8
{
	Actor,
	StaticMesh,
	Collection,
	CollectionStaticMesh,
	Foliage,
	DataAsset,
	OnlyData
};
// -----------------------------------------------
UENUM()
enum EWS_Grid_SamplingType : uint8
{
	PoissonDisk UMETA(Hidden), 
	CircleRND,
	Grid UMETA(Hidden)
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_UpdateViz
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FWS_Grid_Visualisation_Data> Viz;
	FWS_Grid_UpdateViz() = default;
	FWS_Grid_UpdateViz( const TArray<FWS_Grid_Visualisation_Data>& inViz )
	{
		Viz = inViz;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_SpawnConf
{
	GENERATED_BODY()
	UPROPERTY()
	TEnumAsByte<EWS_Grid_SamplingType> SamplingType = CircleRND;
	UPROPERTY()
	int32 Seed = 0;
	UPROPERTY()
	FWS_Grid GridCoord = FWS_Grid();
	UPROPERTY()
	FWS_Grid_Proxy Proxy = FWS_Grid_Proxy();
	UPROPERTY()
	FWS_Grid_SpawnParams SpawnParams = FWS_Grid_SpawnParams();
	UPROPERTY()
	double Z_Offset = 0.0f;
	UPROPERTY()
	int32 Max_TestToSpawn = 200;
	UPROPERTY()
	int32 Max_Spawn = 20;
	UPROPERTY()
	TArray<FWS_Grid_InvalidZone> ZoneData;
	UPROPERTY()
	double ZoneSize = 0;
	FWS_Grid_SpawnConf() = default;
	FWS_Grid_SpawnConf( const EWS_Grid_SamplingType& SType , const int32& inSeed , const FWS_Grid& inGridCoord , const FWS_Grid_Proxy& inProxy , const FWS_Grid_SpawnParams& inSpawnParams , const double& inZ_Offset , const int32& inMax_TestToSpawn , const int32& inMax_Spawn , const TArray<FWS_Grid_InvalidZone>& Zone , const double& inZoneSize )
	{
		Seed = inSeed;
		GridCoord = inGridCoord;
		Proxy = inProxy;
		SpawnParams = inSpawnParams;
		Z_Offset = inZ_Offset;
		Max_Spawn = inMax_Spawn;
		Max_TestToSpawn = inMax_TestToSpawn;
		ZoneData = Zone;
		ZoneSize = inZoneSize;
		SamplingType = SType;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SpawnPending
{
	GENERATED_BODY()
	UPROPERTY()
	FWS_Grid GridCoord = FWS_Grid();
	UPROPERTY()
	FTransform InstanceTransform = FTransform();
	UPROPERTY()
	FWS_Grid_Proxy VizData = FWS_Grid_Proxy();
	UPROPERTY()
	TArray<FWS_Grid_Actor_SpawnData> Actors;
	UPROPERTY()
	TArray<FWS_Grid_Static_SpawnData> StaticMesh;
	UPROPERTY()
	bool IsFoliage = false;
	UPROPERTY()
	int64 Seed = 0;
	FWS_Grid_SpawnPending() = default;
	FWS_Grid_SpawnPending( const FWS_Grid& inCoord , const FTransform& inInstanceTransform , const FWS_Grid_Proxy& inVizData , const TArray<FWS_Grid_Actor_SpawnData>& inActors , const TArray<FWS_Grid_Static_SpawnData>& inStaticMesh , const bool& inIsFoliage = false , const int64& inSeed = 0 )
	{
		GridCoord = inCoord;
		InstanceTransform = inInstanceTransform;
		VizData = inVizData;
		Actors = inActors;
		StaticMesh = inStaticMesh;
		IsFoliage = inIsFoliage;
		Seed = inSeed;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_VizValue
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<bool> Viz;
	FWS_Grid_VizValue() = default;
	FWS_Grid_VizValue( const TArray<bool>& inViz )
	{
		Viz = inViz;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_SpawnerVolumeData
{
	GENERATED_BODY()
	UPROPERTY()
	FWS_Grid Coord = FWS_Grid();
	UPROPERTY()
	FVector Position = FVector(0,0,0);
	UPROPERTY()
	double Size = 0.0f;
	UPROPERTY()
	float ScaleSpawn = 1.0f;
	UPROPERTY()
	TArray<TSoftObjectPtr<UWorldScape_GridDataAsset>> SpawnDataAsset;
	FWS_Grid_SpawnerVolumeData() = default;
	FWS_Grid_SpawnerVolumeData( const FWS_Grid& inCoord , const FVector& inPos , const double& inSize , const TArray<TSoftObjectPtr<UWorldScape_GridDataAsset>>& inSpawnData , const float& inScaleSpawn )
	{
		Coord = inCoord;
		Position = inPos;
		Size = inSize;
		SpawnDataAsset = inSpawnData;
		ScaleSpawn = inScaleSpawn;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_SpawnerArray
{
	GENERATED_BODY()
	TMap<AWorldScape_GridSpawnerVolume*,FWS_Grid_SpawnerVolumeData> Spawners;
	FWS_Grid_SpawnerArray() = default;
	FWS_Grid_SpawnerArray( AWorldScape_GridSpawnerVolume*& InSpawner , const FWS_Grid_SpawnerVolumeData& Data )
	{
		Spawners.Add(InSpawner,Data);
	}
};
// -----------------------------------------------
// Serialized
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SerializedActor
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<uint8> Data;
	FWS_Grid_SerializedActor() = default;
	FWS_Grid_SerializedActor( const TArray<uint8>& inData )
	{
		Data = inData;
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SerializedActor_Array
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FWS_Grid_SerializedActor> Datas;
	FWS_Grid_SerializedActor_Array() = default;
	FWS_Grid_SerializedActor_Array( const TArray<FWS_Grid_SerializedActor>& inData )
	{
		Datas= inData;
	}
	FWS_Grid_SerializedActor_Array( const TArray<uint8>& Data )
	{
		Datas = {FWS_Grid_SerializedActor(Data)};
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_SerializedData
{
	GENERATED_BODY()
	UPROPERTY()
	TMap<int64,FWS_Grid_SerializedActor_Array> Datas;
	FWS_Grid_SerializedData() = default;
	FWS_Grid_SerializedData( const TMap<int64,FWS_Grid_SerializedActor_Array>& inData )
	{
		Datas = inData;
	}
	FWS_Grid_SerializedData( const int64& ID , const TArray<uint8>& Data )
	{
		Datas.Add(ID,FWS_Grid_SerializedActor_Array(Data));
	}
	void AddData( const int64& ID , const TArray<uint8>& Data )
	{
		if ( FWS_Grid_SerializedActor_Array* Found = Datas.Find(ID) )
		{
			(*Found).Datas.Add(Data);
		}
		else
		{
			Datas.Add(ID,FWS_Grid_SerializedActor_Array(Data));
		}
	}
};
// -----------------------------------------------
USTRUCT(BlueprintType)
struct FWS_Grid_StorageData
{
	GENERATED_BODY()
	UPROPERTY()
	FWS_Grid_SerializedData Datas = FWS_Grid_SerializedData();
	UPROPERTY()
	TArray<int64> Loaded;
	FWS_Grid_StorageData() = default;
	FWS_Grid_StorageData( const FWS_Grid_SerializedData& inData )
	{
		Datas = inData;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_FoliageSpawnData
{
	GENERATED_BODY()
	UPROPERTY()
	FTransform BaseTransform = FTransform();
	UPROPERTY()
	TArray<FWS_Grid_Static_SpawnData> MeshData;
	FWS_Grid_FoliageSpawnData() = default;
	FWS_Grid_FoliageSpawnData( const FTransform& inBaseTransform , const TArray<FWS_Grid_Static_SpawnData>& inMeshData )
	{
		BaseTransform = inBaseTransform;
		MeshData = inMeshData;
	}
	
};
// -----------------------------------------------
USTRUCT()
struct FWS_VolumeData
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FNoiseVolumeData> NoiseVolumeListVar;
	UPROPERTY()
	TArray<FHeightMapVolumeDataCopy> VolumeListVar;
	UPROPERTY()
	TArray<FTerrainHoleVolumeData> TerrainHoleListVar;
	UPROPERTY()
	TArray<FTransform> VolumeTransformList;
	FWS_VolumeData() = default;
	FWS_VolumeData( const TArray<FNoiseVolumeData>& inNoiseVolumeListVar, const TArray<FHeightMapVolumeDataCopy>& inVolumeListVar, const TArray<FTerrainHoleVolumeData>& inTerrainHoleListVar,const TArray<FTransform>& inVolumeTransformList )
	{
		NoiseVolumeListVar = inNoiseVolumeListVar;
		VolumeListVar = inVolumeListVar;
		TerrainHoleListVar = inTerrainHoleListVar;
		VolumeTransformList = inVolumeTransformList;
	}
};
// -----------------------------------------------
USTRUCT()
struct FWS_Grid_Queue
{
	GENERATED_BODY()
	UPROPERTY()
	TEnumAsByte<EWS_Grid_SpawnType> SpawnType = Actor;
	UPROPERTY()
	FWS_Grid_SpawnConf SpawnConf = FWS_Grid_SpawnConf();
	UPROPERTY()
	TArray<FWS_Grid_Spawner_Actor> ActorData;
	UPROPERTY()
	TArray<FWS_Grid_Actor_Data> CollectionActor;
	UPROPERTY()
	TArray<FWS_Grid_StaticMesh_Data> StaticMeshData;
	UPROPERTY()
	double FoliageSectorSize = 0;
	UPROPERTY()
	FWS_VolumeData WS_Volume = FWS_VolumeData();
	
	FWS_Grid_Queue() = default;
	FWS_Grid_Queue( const TEnumAsByte<EWS_Grid_SpawnType>& inType , const FWS_Grid_SpawnConf& inSpawnData )
	{
		SpawnType = inType;
		SpawnConf = inSpawnData;
	}
	void Add_Actor( const TArray<FWS_Grid_Spawner_Actor>& inData )
	{
		ActorData = inData;
	}
	void Add_StaticMesh( const TArray<FWS_Grid_StaticMesh_Data>& inData )
	{
		StaticMeshData = inData;
	}
	void Add_CollectionActor( const TArray<FWS_Grid_Actor_Data>& inData )
	{
		CollectionActor = inData;
	}
	void SetFoliageSectorSize( double inFoliageSectorSize )
	{
		FoliageSectorSize = inFoliageSectorSize;
	}
	void SetWS_Volume( const FWS_VolumeData& inData )
	{
		WS_Volume = inData;
	}
};

USTRUCT()
struct FWS_FoliageAsyncStruct
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FTransform> VegetationTransform;
	FWS_FoliageAsyncStruct() = default;
	void AddTransform( const FTransform& inTransform )
	{
		VegetationTransform.Add(inTransform);
	}
};
USTRUCT()
struct FWS_FoliageAsyncData_Array
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FWS_FoliageAsyncStruct> Data;
	FWS_FoliageAsyncData_Array() = default;
	FWS_FoliageAsyncData_Array( const int32& Num )
	{
		Data.SetNum(Num);
	}
	FWS_FoliageAsyncData_Array( const TArray<FWS_FoliageAsyncStruct>& inData )
	{
		Data = inData;
	}
};
