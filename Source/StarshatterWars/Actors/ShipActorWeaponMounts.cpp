#include "ShipActor.h"

#include "ShipDesignRegistry.h"
#include "Ship.h"
#include "ShipWeaponMountComponent.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"
#include "UObject/UObjectThreadContext.h"

namespace
{
    bool InWeaponMountConstructor()
    {
        return FUObjectThreadContext::Get().IsInConstructor > 0;
    }

    FVector WeaponMountLocation(const FVector& LegacyLocation)
    {
        return FVector(LegacyLocation.Z,LegacyLocation.X,LegacyLocation.Y);
    }

    FRotator WeaponMountRotation(double AzimuthRadians,double ElevationRadians)
    {
        return FRotator(
            FMath::RadiansToDegrees(ElevationRadians),
            FMath::RadiansToDegrees(AzimuthRadians),
            0.0);
    }
}

void AShipActor::ConfigureWeaponHardpointsFromShipDef(const FString& ShipName)
{
    const FShipDesign* Design = ShipDesignRegistry::Find(ShipName);
    if (!Design) return;
    NativeWeaponHardpoints = AuthoredWeaponPointComponents;
    int32 Index = 0;
    for (const FShipWeapon& Src : Design->Weapon)
    {
        UShipWeaponMountComponent* Mount = AuthoredWeaponPointComponents.IsValidIndex(Index)
            ? Cast<UShipWeaponMountComponent>(AuthoredWeaponPointComponents[Index].Get()) : nullptr;
        ++Index;
        if (!Mount) continue;
        Mount->StationName = FName(*Src.Name);
        Mount->bLoadoutHardpoint = false;
        Mount->AllowedWeaponTypes = {FName(*Src.WeaponType)};
        Mount->LegacyLocation = Src.Location;
        Mount->LegacyMuzzles = Src.Muzzles;
        Mount->LegacyMuzzle = Src.Muzzles.Num() ? Src.Muzzles[0] : Src.Location;
    }
    for (const FShipHardPoint& Hp : Design->Hardpoint)
    {
        UShipWeaponMountComponent* Mount = AuthoredWeaponPointComponents.IsValidIndex(Index)
            ? Cast<UShipWeaponMountComponent>(AuthoredWeaponPointComponents[Index].Get()) : nullptr;
        ++Index;
        if (!Mount) continue;
        Mount->StationName = FName(*Hp.Name);
        Mount->bLoadoutHardpoint = true;
        Mount->AllowedWeaponTypes.Reset();
        for (const FString& Type : Hp.AllowedWeaponTypes)
            Mount->AllowedWeaponTypes.Add(FName(*Type));
        Mount->LegacyLocation = Hp.Location;
        Mount->LegacyMuzzle = Hp.Muzzle;
        Mount->LegacyMuzzles = {Hp.Muzzle};
    }
}

void AShipActor::BindRuntimeWeaponsToMounts()
{
    for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
        if (UShipWeaponMountComponent* Mount=Cast<UShipWeaponMountComponent>(Base.Get()))
            Mount->ClearRuntimeWeapon();

    if (!RuntimeShip) return;

    TArray<Weapon*> RuntimeWeapons;
    ListIter<WeaponGroup> Groups=RuntimeShip->GetWeapons();
    while (++Groups)
    {
        WeaponGroup* Group=Groups.value();
        if (!Group) continue;
        ListIter<Weapon> Weapons=Group->GetWeapons();
        while (++Weapons)
            if (Weapons.value()) RuntimeWeapons.Add(Weapons.value());
    }

    for (Weapon* W : RuntimeWeapons)
    {
        if (!W) continue;
        const FName WeaponName=W->GetName() && W->GetName()[0]
            ? FName(ANSI_TO_TCHAR(W->GetName())) : NAME_None;
        const WeaponDesign* WD=W->Design();
        const FName WeaponType=WD && WD->name.length()
            ? FName(ANSI_TO_TCHAR(WD->name.data())) : NAME_None;

        UShipWeaponMountComponent* Match=nullptr;

        // Installed weapons: exact station/name match first.
        if (!WeaponName.IsNone())
        {
            for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
            {
                UShipWeaponMountComponent* Mount=Cast<UShipWeaponMountComponent>(Base.Get());
                if (Mount && !Mount->RuntimeWeapon && !Mount->bLoadoutHardpoint && Mount->StationName==WeaponName)
                { Match=Mount; break; }
            }
        }

        // Loadout weapons: bind to the first free compatible station.
        if (!Match && !WeaponType.IsNone())
        {
            for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
            {
                UShipWeaponMountComponent* Mount=Cast<UShipWeaponMountComponent>(Base.Get());
                if (Mount && !Mount->RuntimeWeapon && Mount->bLoadoutHardpoint && Mount->AllowedWeaponTypes.Contains(WeaponType))
                { Match=Mount; break; }
            }
        }

        // Final fallback for fixed weapons with a unique compatible type.
        if (!Match && !WeaponType.IsNone())
        {
            for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
            {
                UShipWeaponMountComponent* Mount=Cast<UShipWeaponMountComponent>(Base.Get());
                if (Mount && !Mount->RuntimeWeapon && Mount->AllowedWeaponTypes.Contains(WeaponType))
                { Match=Mount; break; }
            }
        }

        if (Match)
        {
            Match->BindRuntimeWeapon(W,WeaponName,WeaponType);
            UE_LOG(LogTemp,Verbose,TEXT("[ShipActorWeaponBinding] Ship='%hs' Weapon='%s' Type='%s' -> Station='%s'"),
                RuntimeShip->GetName(),*WeaponName.ToString(),*WeaponType.ToString(),*Match->StationName.ToString());
        }
        else
        {
            UE_LOG(LogTemp,Warning,TEXT("[ShipActorWeaponBinding] UNBOUND Ship='%hs' Weapon='%s' Type='%s'"),
                RuntimeShip->GetName(),*WeaponName.ToString(),*WeaponType.ToString());
        }
    }
}

bool AShipActor::GetRuntimeWeaponMuzzleTransform(const Weapon* InWeapon,int32 MuzzleIndex,FTransform& OutTransform) const
{
    if (!InWeapon) return false;
    for (const TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
    {
        const UShipWeaponMountComponent* Mount=Cast<UShipWeaponMountComponent>(Base.Get());
        if (Mount && Mount->IsBoundTo(InWeapon))
        {
            OutTransform=Mount->GetMuzzleWorldTransform(MuzzleIndex);
            return true;
        }
    }
    return false;
}
