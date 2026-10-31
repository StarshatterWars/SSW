// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "PVTLibrary.h"
#include "Async/AsyncWork.h"

class AVortexTunnelGenerator;
class UVortexMeshGenerator;
/**
 * 
 */
class PROCEDURALVORTEXTUNNEL_API FVortexMeshGenerationTask : public FNonAbandonableTask
{
public:
	FVortexMeshGenerationTask(AVortexTunnelGenerator* InGenerator, UVortexMeshGenerator* InMeshGenerator, 
							  const FVortexParameters& InParams);

	void DoWork();

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FVortexMeshGenerationTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	// Getter functions for the generated mesh data
	TArray<FVector>& GetVertices() { return Vertices; }
	TArray<int32>& GetTriangles() { return Triangles; }
	TArray<FVector>& GetNormals() { return Normals; }
	TArray<FVector2D>& GetUV0() { return UV0; }
	TArray<FProcMeshTangent>& GetTangents() { return Tangents; }

private:
	AVortexTunnelGenerator* Generator;
	UVortexMeshGenerator* MeshGenerator;
	FVortexParameters Params;

	// Storage for generated mesh data
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FProcMeshTangent> Tangents;
};