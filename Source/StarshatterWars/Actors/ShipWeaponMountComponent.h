#pragma once
#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "ShipWeaponMountComponent.generated.h"

class Weapon;

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
    bool IsBoundTo(const Weapon* InWeapon) const { return RuntimeWeapon==InWeapon; }

    int32 GetMuzzleCount() const;
    FVector GetMuzzleWorldLocation(int32 MuzzleIndex=0) const;
    FTransform GetMuzzleWorldTransform(int32 MuzzleIndex=0) const;
};

struct FNativeWeaponMountDef
{
    FName Name;
    bool bLoadout=false;
    TArray<FName> Types;
    FVector Location=FVector::ZeroVector;
    FVector Muzzle=FVector::ZeroVector;
    double Azimuth=0;
    double Elevation=0;
};
namespace ShipWeaponMountCatalog { void Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out); }
