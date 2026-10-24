#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataAsset.h"
#include "TurretActor.generated.h"
class UStaticMeshComponent;
class USceneComponent;
class Weapon;
class Ship;
class Camera;

// Visual-only actor. Legacy Weapon remains responsible for targeting and firing.
UCLASS(Blueprintable)
class STARSHATTERWARS_API ATurretActor : public AActor
{
    GENERATED_BODY()
public:
    ATurretActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret") TObjectPtr<USceneComponent> MountRoot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret") TObjectPtr<USceneComponent> BasePivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret") TObjectPtr<USceneComponent> GunPivot;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret") TObjectPtr<UStaticMeshComponent> BaseMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turret") TObjectPtr<UStaticMeshComponent> GunMesh;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") FName WeaponType;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") FString LegacyGunModel;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") FString LegacyBaseModel;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") float LegacyModelScale = 1.0f;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") int32 BaseAxis = 0;
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="Turret|Definition") TArray<FVector> LegacyMuzzles;
    // Mesh-relative transforms/materials are authored in BP; pivots alone are driven by simulation.
    UFUNCTION(BlueprintPure, Category="Turret") bool GetMuzzleTransform(int32 BarrelIndex, FTransform& OutTransform) const;
    void SetPreviewAim(double Azimuth, double Elevation);
    void SyncFromWeapon(const Weapon& InWeapon, const Ship& InShip);
    void SyncFromWeaponFrame(const Weapon& InWeapon, const Camera& SimShip, const FQuat& DisplayShip);
private:
    bool bHasRuntimeMuzzles = false;
    TArray<FVector> RuntimeMuzzles;
};

USTRUCT(BlueprintType)
struct FShipTurretMountDef
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret") FName WeaponType;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret") FName MountName;
    // Unscaled ship.def coordinates: right, up, forward. Converted only for display.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret") FVector LegacyLocation = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret") double AzimuthRadians = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Turret") double ElevationRadians = 0;
};

// One shared asset can supply Blueprint classes to every ship Blueprint.
UCLASS(BlueprintType)
class STARSHATTERWARS_API UTurretVisualSet : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Turret") TMap<FName,TSubclassOf<ATurretActor>> Classes;
};
