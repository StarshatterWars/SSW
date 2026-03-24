/*  Project nGenEx
	Fractal Dev Games
	Copyright (C) 2024. All Rights Reserved.

	SUBSYSTEM:    SSW
	FILE:         SystemOverview.h
	AUTHOR:       Carlos Bott
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SystemOverview.generated.h"

class USceneComponent;
class USceneCaptureComponent2D;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;
class UPointLightComponent;

USTRUCT(BlueprintType)
struct FOverviewBody
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Name;

	// Distance from parent (star -> planet, planet -> moon)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OrbitKm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RadiusKm = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OrbitAngleDeg = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InclinationDeg = 0.f;

	// -1 = root star, otherwise index into Bodies array
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ParentIndex = INDEX_NONE;
};

UCLASS()
class STARSHATTERWARS_API ASystemOverview : public AActor
{
	GENERATED_BODY()

public:
	ASystemOverview();

	virtual void BeginPlay() override;

	/** Build all bodies (star / planets / moons) in one diorama */
	void BuildDiorama(const TArray<FOverviewBody>& InBodies);

	/** Assign render target (owned by GameInstance) */
	void SetRenderTarget(UTextureRenderTarget2D* InRT);

	/** Capture once (scheduled safely on next tick) */
	void CaptureOnce();

	/** Reframe camera so all bodies fit in view */
	void FrameDiorama(float PaddingMultiplier = 1.15f);

	/** Configure capture flags for clean UI output */
	void ConfigureCaptureForUI();

private:
	// Root scene
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	// Scene capture
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneCaptureComponent2D> Capture;

	// Optional light (recommended for non-black previews)
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> KeyLight;

	// Runtime body meshes
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BodyMeshes;

	// Shared sphere mesh
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	// Scaling
	UPROPERTY(EditAnywhere, Category = "Overview")
	float KmToUU = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Overview")
	float SphereRadiusUU = 50.f;

	UPROPERTY(EditAnywhere, Category = "Overview")
	float MinBodyScale = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Overview")
	float MaxBodyScale = 10.0f;

	// Helpers
	FVector ComputeBodyWorldPosition(
		const FOverviewBody& Body,
		const FVector& ParentWorldPos
	) const;

	float ComputeBodyUniformScale(const FOverviewBody& Body) const;

	void ClearBodies();
};
