// Copyright 2018-2025 Athian Games. All Rights Reserved. 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "FVortexMeshGenerationTask.h"
#include "VortexSpline.h"
#include "VortexMeshGenerator.h"
#include "Async/AsyncWork.h"
#include "VortexTunnelGenerator.generated.h"

UCLASS()
class PROCEDURALVORTEXTUNNEL_API AVortexTunnelGenerator : public AActor
{
	GENERATED_BODY()

public:
	AVortexTunnelGenerator();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
	AVortexSpline* SourceSpline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
	FVortexParameters VortexProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
	int32 NumberOfSegments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
	float RelativeStartDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex")
	float RelativeEndDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vortex|Material")
	UMaterialInterface* VortexMaterial;

	UPROPERTY()
	bool bIsPreviewMesh = true;

	UFUNCTION(CallInEditor, Category = "Vortex")
	void GeneratePreviewVortex();

	UFUNCTION(CallInEditor, Category = "Vortex")
	void GenerateVortexTunnel();

	void ProcessNextTask();

	void ReceiveGeneratedMeshData(TArray<FVector>& Vertices, TArray<int32>& Triangles,
	                              TArray<FVector>& Normals, TArray<FVector2D>& UV0,
	                              TArray<FProcMeshTangent>& Tangents);
private:

	virtual bool ShouldTickIfViewportsOnly() const override;
	
	TUniquePtr<FAsyncTask<FVortexMeshGenerationTask>> CurrentTask;
	TUniquePtr<FAsyncTask<FVortexMeshGenerationTask>> PendingTask;
	
	UPROPERTY()
	UBillboardComponent* RootBillboard;

	UPROPERTY()
	UVortexMeshGenerator* MeshGenerator;

	UPROPERTY()
	UProceduralMeshComponent* PreviewMesh;

	void CleanupMesh(TArray<FVector>& Vertices, TArray<int32>& Triangles);
	
};