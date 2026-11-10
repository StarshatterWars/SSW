#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeTerrainActor.generated.h"
class UProceduralMeshComponent;
class Terrain;
class USkyAtmosphereComponent;
class UVolumetricCloudComponent;
class UDirectionalLightComponent;
UCLASS()
class STARSHATTERWARS_API ANativeTerrainActor : public AActor
{
    GENERATED_BODY()
public:
    ANativeTerrainActor();
    void SetSurfaceSky(bool bEnabled);
    void UpdateTerrain(Terrain* Model,const FVector& Observer,const FVector& RegionOrigin);
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Terrain") TObjectPtr<UProceduralMeshComponent> Mesh;
private:
    UPROPERTY(Transient) TObjectPtr<USkyAtmosphereComponent> SurfaceAtmosphere;
    UPROPERTY(Transient) TObjectPtr<UVolumetricCloudComponent> SurfaceClouds;
    UPROPERTY(Transient) TObjectPtr<UDirectionalLightComponent> SurfaceSun;
    Terrain* LastModel=nullptr;
    FIntPoint LastCell=FIntPoint(MAX_int32,MAX_int32);
};
