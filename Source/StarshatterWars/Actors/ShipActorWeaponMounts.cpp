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
    const bool bInConstructor=InWeaponMountConstructor();

    auto MakeRoot=[this,bInConstructor](FName Name)->USceneComponent*
    {
        USceneComponent* Comp=bInConstructor
            ? CreateDefaultSubobject<USceneComponent>(Name)
            : NewObject<USceneComponent>(this,Name);
        if (!Comp) return nullptr;
        Comp->SetupAttachment(GetRootComponent());
        if (!bInConstructor) Comp->RegisterComponent();
        return Comp;
    };

    auto MakeMount=[this,bInConstructor](FName Name,USceneComponent* Parent)->UShipWeaponMountComponent*
    {
        UShipWeaponMountComponent* Comp=bInConstructor
            ? CreateDefaultSubobject<UShipWeaponMountComponent>(Name)
            : NewObject<UShipWeaponMountComponent>(this,Name);
        if (!Comp) return nullptr;
        Comp->SetupAttachment(Parent ? Parent : GetRootComponent());
        if (!bInConstructor) Comp->RegisterComponent();
        return Comp;
    };

    if (!WeaponHardpointRoot)
        WeaponHardpointRoot=MakeRoot(TEXT("WeaponHardpointRoot"));

    if (!bInConstructor)
    {
        for (TObjectPtr<USceneComponent>& Mount : NativeWeaponHardpoints)
            if (Mount) Mount->DestroyComponent();
    }
    NativeWeaponHardpoints.Reset();
    WeaponMountPointDefs.Reset();

    const FShipDesign* Design=ShipDesignRegistry::Find(ShipName);
    if (!Design)
    {
        UE_LOG(LogTemp,Warning,TEXT("[ShipActorWeaponMounts] No FShipDesign row for '%s'"),*ShipName);
        return;
    }

    // Fixed/internal weapons. These are actual installed weapons, not loadout rails.
    for (int32 Index=0; Index<Design->Weapon.Num(); ++Index)
    {
        const FShipWeapon& Src=Design->Weapon[Index];
        if (Src.WeaponType.IsEmpty()) continue;

        const FName StationName=!Src.Name.IsEmpty()
            ? FName(*Src.Name)
            : FName(*FString::Printf(TEXT("Weapon_%d"),Index));

        UShipWeaponMountComponent* Mount=MakeMount(
            FName(*FString::Printf(TEXT("FixedWeaponMount_%d"),Index)),
            WeaponHardpointRoot);
        if (!Mount) continue;

        Mount->StationName=StationName;
        Mount->bLoadoutHardpoint=false;
        Mount->AllowedWeaponTypes={FName(*Src.WeaponType)};
        Mount->LegacyLocation=Src.Location;
        Mount->LegacyMuzzles=Src.Muzzles;
        Mount->LegacyMuzzle=Src.Muzzles.Num()>0 ? Src.Muzzles[0] : Src.Location;
        Mount->SetRelativeLocation(WeaponMountLocation(Src.Location));
        Mount->SetRelativeRotation(WeaponMountRotation(Src.AzimuthRadians,Src.ElevationRadians));
        NativeWeaponHardpoints.Add(Mount);
    }

    // Loadout hardpoints. Keep WeaponMountPointDefs hardpoint-only so existing
    // compatibility callers retain their old indexing semantics.
    for (int32 Index=0; Index<Design->Hardpoint.Num(); ++Index)
    {
        const FShipHardPoint& Hp=Design->Hardpoint[Index];
        if (Hp.Name.IsEmpty() && Hp.DesignName.IsEmpty() && Hp.AllowedWeaponTypes.Num()==0)
            continue;

        const FName StationName=!Hp.Name.IsEmpty()
            ? FName(*Hp.Name)
            : FName(*FString::Printf(TEXT("HP_%d"),Index));

        FShipPointDef Point;
        Point.PointName=StationName;
        Point.LocalOffset=WeaponMountLocation(Hp.Location);
        Point.LocalRotation=WeaponMountRotation(Hp.AzimuthRadians,Hp.ElevationRadians);
        WeaponMountPointDefs.Add(Point);

        UShipWeaponMountComponent* Mount=MakeMount(
            FName(*FString::Printf(TEXT("LoadoutWeaponMount_%d"),Index)),
            WeaponHardpointRoot);
        if (!Mount) continue;

        Mount->StationName=StationName;
        Mount->bLoadoutHardpoint=true;
        Mount->AllowedWeaponTypes.Reset();
        for (const FString& Type : Hp.AllowedWeaponTypes)
            if (!Type.IsEmpty()) Mount->AllowedWeaponTypes.Add(FName(*Type));
        Mount->LegacyLocation=Hp.Location;
        Mount->LegacyMuzzle=Hp.Muzzle;
        Mount->LegacyMuzzles={Hp.Muzzle};
        Mount->SetRelativeLocation(WeaponMountLocation(Hp.Location));
        Mount->SetRelativeRotation(WeaponMountRotation(Hp.AzimuthRadians,Hp.ElevationRadians));
        NativeWeaponHardpoints.Add(Mount);
    }

    RebuildWeaponMountPoints();

    UE_LOG(LogTemp,Log,TEXT("[ShipActorWeaponMounts] Ship='%s' NativeMounts=%d LoadoutPoints=%d"),
        *ShipName,NativeWeaponHardpoints.Num(),WeaponMountPoints.Num());
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
