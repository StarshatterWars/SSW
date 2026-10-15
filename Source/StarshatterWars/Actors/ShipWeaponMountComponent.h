#pragma once
#include "CoreMinimal.h"
#include "Components/ChildActorComponent.h"
#include "ShipWeaponMountComponent.generated.h"

// Assign the visual actor BP through the inherited Child Actor Class field.
// Does not change mission loadout, spawn projectiles, or own simulation weapons.
UCLASS(ClassGroup=(Starshatter), meta=(BlueprintSpawnableComponent))
class STARSHATTERWARS_API UShipWeaponMountComponent : public UChildActorComponent
{
    GENERATED_BODY()
public:
    UShipWeaponMountComponent();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FName StationName;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") bool bLoadoutHardpoint=false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") TArray<FName> AllowedWeaponTypes;
    // Unscaled ship.def coordinates (right, up, forward), for reference.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FVector LegacyLocation=FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon Mount|Definition") FVector LegacyMuzzle=FVector::ZeroVector;
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
namespace ShipWeaponMountCatalog {
    void Get(FName ShipDef,TArray<FNativeWeaponMountDef>& Out);
}
