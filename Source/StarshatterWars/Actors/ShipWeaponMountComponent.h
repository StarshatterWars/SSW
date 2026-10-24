#pragma once
#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "ShipWeaponMountComponent.generated.h"

class Weapon;
class Camera;

UCLASS(ClassGroup=(Starshatter), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UShipWeaponMountComponent : public UChildActorComponent
{
    GENERATED_BODY()
public:
    UShipWeaponMountComponent();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FName StationName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") bool bLoadoutHardpoint=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") TArray<FName> AllowedWeaponTypes;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FVector LegacyLocation=FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FVector LegacyMuzzle=FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") TArray<FVector> LegacyMuzzles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Runtime") FName RuntimeWeaponName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Runtime") FName RuntimeWeaponType;

    // Non-UObject simulation pointer; never owned here.
    Weapon* RuntimeWeapon=nullptr;

    void BindRuntimeWeapon(Weapon* InWeapon,FName InWeaponName,FName InWeaponType);
    void ClearRuntimeWeapon();
    void UpdateRuntimeAim(const Camera& SimShip, const FQuat& DisplayShip);
private:
    bool bRuntimeAimApplied = false;
    FQuat RestMountRotation = FQuat::Identity;
public:
    bool IsBoundTo(const Weapon* InWeapon) const { return RuntimeWeapon==InWeapon; }

    /** One native BP child per authored fixed-weapon muzzle, parented to this mount. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Barrels")
    TArray<TObjectPtr<UChildActorComponent>> BarrelComponents;
    UFUNCTION(BlueprintPure, Category="Weapon Mount|Barrels")
    int32 GetMuzzleCount() const;
    UFUNCTION(BlueprintPure, Category="Weapon Mount|Barrels")
    FVector GetMuzzleWorldLocation(int32 MuzzleIndex=0) const;
    UFUNCTION(BlueprintPure, Category="Weapon Mount|Barrels")
    FTransform GetMuzzleWorldTransform(int32 MuzzleIndex=0) const;
};

struct FNativeWeaponMountDef
{
    FName Name;
    bool bLoadout=false;
    TArray<FName> Types;
    FVector Location=FVector::ZeroVector;
    FVector Muzzle=FVector::ZeroVector;
    TArray<FVector> Muzzles;
    TArray<FVector> BarrelOffsets;
    double Azimuth=0;
    double Elevation=0;
};
namespace FighterWeaponMountCatalog { void Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out); }
namespace ShipWeaponMountCatalog { void Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out); }
