#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponMountActor.generated.h"
class UStaticMeshComponent;
// Safe visual BP parent for gun mounts, missile rails and carried missile meshes.
// Projectile spawning and loadout remain owned by the existing weapon simulation.
UCLASS(Blueprintable)
class STARSHATTERWARS_API AWeaponMountActor : public AActor
{
    GENERATED_BODY()
public:
    AWeaponMountActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount") TObjectPtr<USceneComponent> MountRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount") TObjectPtr<UStaticMeshComponent> MountMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount") TObjectPtr<UStaticMeshComponent> StoreMesh;
};
