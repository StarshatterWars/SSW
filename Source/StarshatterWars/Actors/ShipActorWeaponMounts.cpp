#include "ShipActor.h"

#include "ShipDesignRegistry.h"
#include "Ship.h"
#include "ShipWeaponMountComponent.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponGroup.h"
#include "BoltWeaponActor.h"
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
    // Older visual Blueprints derive from the generic ship/fighter class and
    // have no native stations. Supply missing presentation mounts from the DEF;
    // never change the simulation loadout or replace authored mount overrides.
    auto EnsureMount = [&](int32 MountIndex, const FVector& Location, double Azimuth, double Elevation)
    {
        if (AuthoredWeaponPointComponents.IsValidIndex(MountIndex)
            && Cast<UShipWeaponMountComponent>(AuthoredWeaponPointComponents[MountIndex].Get())) return;
        if (InWeaponMountConstructor() || !GetWorld()) return;
        UShipWeaponMountComponent* Mount = NewObject<UShipWeaponMountComponent>(this);
        AddInstanceComponent(Mount);
        Mount->SetMobility(EComponentMobility::Movable);
        Mount->SetupAttachment(PointRoot);
        const double Scale = FMath::Abs(Design->Scale) > SMALL_NUMBER ? Design->Scale : 1.0;
        Mount->SetRelativeLocation(WeaponMountLocation(Location / Scale));
        Mount->SetRelativeRotation(WeaponMountRotation(Azimuth, Elevation));
        Mount->RegisterComponent();
        if (AuthoredWeaponPointComponents.Num() <= MountIndex)
            AuthoredWeaponPointComponents.SetNum(MountIndex + 1);
        AuthoredWeaponPointComponents[MountIndex] = Mount;
        NativeWeaponHardpoints = AuthoredWeaponPointComponents;
    };
    int32 Index = 0;
    for (const FShipWeapon& Src : Design->Weapon)
    {
        EnsureMount(Index, Src.Location, Src.AzimuthRadians, Src.ElevationRadians);
        UShipWeaponMountComponent* Mount = AuthoredWeaponPointComponents.IsValidIndex(Index)
            ? Cast<UShipWeaponMountComponent>(AuthoredWeaponPointComponents[Index].Get()) : nullptr;
        ++Index;
        if (!Mount) continue;
        Mount->StationName = FName(*Src.Name);
        Mount->bLoadoutHardpoint = false;
        Mount->AllowedWeaponTypes = {FName(*Src.WeaponType)};
        Mount->ConfigureProjectileBlueprint(Src.ProjectileBlueprint.IsEmpty()
            ? ABoltWeaponActor::BlueprintPathForWeapon(Src.WeaponType) : Src.ProjectileBlueprint);
        Mount->LegacyLocation = Src.Location;
        Mount->LegacyMuzzles = Src.Muzzles;
        Mount->LegacyMuzzle = Src.Muzzles.Num() ? Src.Muzzles[0] : Src.Location;
    }
    for (const FShipHardPoint& Hp : Design->Hardpoint)
    {
        EnsureMount(Index, Hp.Location, Hp.AzimuthRadians, Hp.ElevationRadians);
        UShipWeaponMountComponent* Mount = AuthoredWeaponPointComponents.IsValidIndex(Index)
            ? Cast<UShipWeaponMountComponent>(AuthoredWeaponPointComponents[Index].Get()) : nullptr;
        ++Index;
        if (!Mount) continue;
        Mount->StationName = FName(*(Hp.Name.IsEmpty() ? Hp.Abbrev : Hp.Name));
        Mount->bLoadoutHardpoint = true;
        Mount->ConfigureProjectileBlueprint(Hp.ProjectileBlueprint);
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

        // The runtime weapon retains its hardpoint's legacy mount location,
        // including definitions whose station abbreviation is omitted.
        if (WD && !WD->primary)
        {
            for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
            {
                auto* Mount = Cast<UShipWeaponMountComponent>(Base.Get());
                if (Mount && !Mount->RuntimeWeapon && Mount->bLoadoutHardpoint
                    && Mount->LegacyLocation.Equals(W->GetMountRelativeLocation(), 0.001)
                    && Mount->AllowedWeaponTypes.Contains(WeaponType))
                { Match = Mount; break; }
            }
        }

        // HardPoint::CreateWeapon retains the station abbreviation. Group
        // iteration order can differ from station order for mixed loadouts.
        if (!Match && WD && !WD->primary && W->Abbreviation() && W->Abbreviation()[0])
        {
            const FName Station(ANSI_TO_TCHAR(W->Abbreviation()));
            for (TObjectPtr<USceneComponent>& Base : NativeWeaponHardpoints)
            {
                auto* Mount = Cast<UShipWeaponMountComponent>(Base.Get());
                if (Mount && !Mount->RuntimeWeapon && Mount->bLoadoutHardpoint
                    && Mount->StationName == Station && Mount->AllowedWeaponTypes.Contains(WeaponType))
                { Match = Mount; break; }
            }
        }

        // Installed weapons: exact station/name match first.
        if (!Match && !WeaponName.IsNone())
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

UClass* AShipActor::GetRuntimeWeaponProjectileClass(const Weapon* InWeapon) const
{
    if (!InWeapon) return nullptr;
    for (const auto& Base : NativeWeaponHardpoints)
        if (const auto* Mount = Cast<UShipWeaponMountComponent>(Base.Get()))
            if (Mount->IsBoundTo(InWeapon)) return Mount->ResolveProjectileClass();
    return nullptr;
}
