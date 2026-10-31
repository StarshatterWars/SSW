// Copyright 2018-2025 Athian Games. All Rights Reserved. 


#include "FVortexMeshGenerationTask.h"

#include "KismetProceduralMeshLibrary.h"
#include "VortexMeshGenerator.h"
#include "VortexTunnelGenerator.h"

FVortexMeshGenerationTask::FVortexMeshGenerationTask(AVortexTunnelGenerator* InGenerator,
	UVortexMeshGenerator* InMeshGenerator, const FVortexParameters& InParams)
	: Generator(InGenerator)
	, MeshGenerator(InMeshGenerator)
	, Params(InParams)
{
}

void FVortexMeshGenerationTask::DoWork()
{
	if(MeshGenerator)
	{
		MeshGenerator->GenerateVortexVertices(Vertices, Triangles, UV0, Params);

		// Process any queued progress updates
		MeshGenerator->ProcessQueuedProgressUpdates();

		// Calculate normals and tangents
		UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);
	}

	// Pass the generated data back to the game thread
	AsyncTask(ENamedThreads::GameThread, [this]() {
		if (Generator)
		{
			Generator->ReceiveGeneratedMeshData(Vertices, Triangles, Normals, UV0, Tangents);
		}
	});
}