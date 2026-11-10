#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DeveloperSettings.h"
#include "NativeAsteroidActor.generated.h"
class UStaticMesh;
class UStaticMeshComponent;
class Asteroid;
class SimRegion;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Starshatter Asteroid Library"))
class STARSHATTERWARS_API UAsteroidLibrarySettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    // Assign imported library meshes here when import completes. No legacy DEF changes.
    UPROPERTY(Config,EditAnywhere,Category="Library") TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

};

UCLASS(Blueprintable)
class STARSHATTERWARS_API ANativeAsteroidActor : public AActor
{
    GENERATED_BODY()
public:
    ANativeAsteroidActor();
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> AsteroidMesh;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Asteroid") double RenderedRadius=0;
    // The mesh is cosmetic; legacy simulation remains the only collision authority.
    void UpdateFromSimulation(const Asteroid& Rock,const FVector& VisualPosition,UStaticMesh* Mesh);
    static double RawRadius(double MeshRadius);
    void SetContactIdentity(const SimRegion* Region, int32 Index);
    const Asteroid* ResolveContact(SimRegion* Region) const;
    int32 GetContactIndex() const { return ContactIndex; }
private:
    const SimRegion* ContactRegion = nullptr;
    int32 ContactIndex = INDEX_NONE;
};
