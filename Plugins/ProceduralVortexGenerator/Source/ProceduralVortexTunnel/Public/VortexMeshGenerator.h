// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProceduralMeshComponent.h"
#include "PVTLibrary.h"
#include "VortexMeshGenerator.generated.h"




/**
 * 
 */
UCLASS()
class PROCEDURALVORTEXTUNNEL_API UVortexMeshGenerator : public UObject
{
	GENERATED_BODY()

public:
	UVortexMeshGenerator();

	void GenerateVortexVertices(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params);
	void GenerateVortexMesh(UProceduralMeshComponent* Mesh, const FVortexParameters& Params);

	void ProcessQueuedProgressUpdates();

private:
	float SimpleNoise3D(FVector v);
	void GenerateTriangles(TArray<int32>& Triangles, int32 RadialSegments, int32 HeightSegments);
	void AddTriangle(TArray<int32>& Triangles, int32 V1, int32 V2, int32 V3);
	float FractalNoise(FVector v, int32 octaves, float persistence, float lacunarity);


	FCriticalSection ProgressLock;
	float CurrentProgress;
	FText CurrentStatusText;

	void QueueProgressUpdate(float Progress, const FText& StatusText);
	
	TSharedPtr<SNotificationItem> ProgressNotification;
	void UpdateProgress(float Progress, const FText& StatusText);

	void GenerateNormalVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params);
	void GenerateWiredVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params);
	void GenerateCubeClusterVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params);
	void GenerateRingVortex(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVortexParameters& Params);

	
	FVector GenerateOrganicNoise(const FVector& Position, const FVortexParameters& Params);
	void GenerateCube(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector2D>& UV0, const FVector& Center, float Size, const FRotator& Rotation);
};