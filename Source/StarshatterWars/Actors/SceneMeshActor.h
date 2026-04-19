/*  Project Starshatter Wars
    Fractal Dev Studios
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneMeshActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;

UCLASS()
class STARSHATTERWARS_API ASceneMeshActor : public AActor
{
    GENERATED_BODY()

public:
    ASceneMeshActor();

    UFUNCTION(BlueprintCallable)
    bool SetSceneMesh(UStaticMesh* InMesh);

    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetMeshComponent() const { return MeshComponent; }

protected:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
};
