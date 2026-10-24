#include "ShipWeaponMountComponent.h"
#include "Weapon.h"

UShipWeaponMountComponent::UShipWeaponMountComponent()
{
    bEditableWhenInherited=true;
    PrimaryComponentTick.bCanEverTick=false;
}

void UShipWeaponMountComponent::BindRuntimeWeapon(Weapon* InWeapon,FName InWeaponName,FName InWeaponType)
{
    RuntimeWeapon=InWeapon;
    RuntimeWeaponName=InWeaponName;
    RuntimeWeaponType=InWeaponType;
}

void UShipWeaponMountComponent::ClearRuntimeWeapon()
{
    RuntimeWeapon=nullptr;
    RuntimeWeaponName=NAME_None;
    RuntimeWeaponType=NAME_None;
}

int32 UShipWeaponMountComponent::GetMuzzleCount() const
{
    return BarrelComponents.Num()>0 ? BarrelComponents.Num() : (LegacyMuzzles.Num()>0 ? LegacyMuzzles.Num() : 1);
}

FVector UShipWeaponMountComponent::GetMuzzleWorldLocation(int32 MuzzleIndex) const
{
    if (BarrelComponents.IsValidIndex(MuzzleIndex) && BarrelComponents[MuzzleIndex])
        return BarrelComponents[MuzzleIndex]->GetComponentLocation();
    FVector ShipSpaceMuzzle=LegacyMuzzle;
    if (LegacyMuzzles.Num()>0)
    {
        const int32 SafeIndex=FMath::Clamp(MuzzleIndex,0,LegacyMuzzles.Num()-1);
        ShipSpaceMuzzle=LegacyMuzzles[SafeIndex];
    }
    const FVector LegacyDelta=ShipSpaceMuzzle-LegacyLocation;
    // Legacy local: X=right, Y=up, Z=forward
    // UE actor local: X=forward, Y=right, Z=up
    const FVector LocalFromStation(LegacyDelta.Z,LegacyDelta.X,LegacyDelta.Y);
    return GetComponentTransform().TransformPosition(LocalFromStation);
}

FTransform UShipWeaponMountComponent::GetMuzzleWorldTransform(int32 MuzzleIndex) const
{
    if (BarrelComponents.IsValidIndex(MuzzleIndex) && BarrelComponents[MuzzleIndex])
        return BarrelComponents[MuzzleIndex]->GetComponentTransform();
    FTransform Result=GetComponentTransform();
    Result.SetLocation(GetMuzzleWorldLocation(MuzzleIndex));
    return Result;
}
