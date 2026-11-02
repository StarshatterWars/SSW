#include "ShipWeaponMountComponent.h"
#include "WeaponBlueprintResolver.h"
#include "ShipStorePresentation.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponPresentationFrame.h"
#include "BoltWeaponActor.h"

UShipWeaponMountComponent::UShipWeaponMountComponent()
{
    bEditableWhenInherited=true;
    PrimaryComponentTick.bCanEverTick=false;
}

void UShipWeaponMountComponent::ConfigureProjectileBlueprint(const FString& DefaultPath)
{
    FSoftObjectPath Path(DefaultPath);
    if (ProjectileBlueprint.IsNull() || ProjectileBlueprint.ToSoftObjectPath() == AutoProjectileBlueprint)
        ProjectileBlueprint = TSoftClassPtr<AActor>(Path);
    AutoProjectileBlueprint = Path;
}
UClass* UShipWeaponMountComponent::ResolveProjectileClass() const
{
    return ProjectileBlueprint.LoadSynchronous();
}

void UShipWeaponMountComponent::BindRuntimeWeapon(Weapon* InWeapon,FName InWeaponName,FName InWeaponType)
{
    if (RuntimeWeapon != InWeapon) ClearRuntimeWeapon();
    if (InWeapon && InWeapon->Design() && !InWeapon->Design()->primary
        && (!GetChildActorClass() || GetChildActorClass() == ResolvedStoreClass.Get()))
    {
        if (UClass* Class = WeaponBlueprintResolver::Resolve(
            ANSI_TO_TCHAR(InWeapon->Design()->model.data()), ANSI_TO_TCHAR(InWeapon->Design()->name.data())))
        {
            SetChildActorClass(Class);
            ResolvedStoreClass = Class;
        }
    }
    if (InWeapon && InWeapon->Design())
    {
        const WeaponDesign* Design = InWeapon->Design();
        if (Design->projectile.length())
            ConfigureProjectileBlueprint(WeaponBlueprintResolver::ProjectileClassPath(ANSI_TO_TCHAR(Design->projectile.data())));
        else if (Design->primary && !Design->beam)
            ConfigureProjectileBlueprint(ABoltWeaponActor::BlueprintPathForWeapon(InWeaponType.ToString()));
        else if (!Design->primary)
            if (UClass* Class = WeaponBlueprintResolver::Resolve(ANSI_TO_TCHAR(Design->model.data()), InWeaponType.ToString()))
                ConfigureProjectileBlueprint(Class->GetPathName());
    }
    RuntimeWeapon=InWeapon;
    RuntimeWeaponName=InWeaponName;
    RuntimeWeaponType=InWeaponType;
    UpdateRuntimeStore();
}

void UShipWeaponMountComponent::ClearRuntimeWeapon()
{
    if (bRuntimeAimApplied) SetRelativeRotation(RestMountRotation);
    bRuntimeAimApplied = false;
    if (ResolvedStoreClass.IsValid() && GetChildActorClass() == ResolvedStoreClass.Get())
        SetChildActorClass(nullptr);
    ResolvedStoreClass.Reset();
    RuntimeWeapon=nullptr;
    RuntimeWeaponName=NAME_None;
    RuntimeWeaponType=NAME_None;
    UpdateRuntimeStore();
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

void UShipWeaponMountComponent::UpdateRuntimeAim(const Camera& SimShip, const FQuat& DisplayShip)
{
    // Fixed fighter gun/pylon placement must not be rotated by an aim camera.
    if (!RuntimeWeapon || bLoadoutHardpoint || !RuntimeWeapon->Design() || !RuntimeWeapon->Design()->turret.length()) return;
    if (!bRuntimeAimApplied) { RestMountRotation = GetRelativeRotation().Quaternion(); bRuntimeAimApplied = true; }
    const FQuat Aim = WeaponPresentationFrame::Display(RuntimeWeapon->GetTurretAimCamera(), SimShip, DisplayShip);
    const FQuat Parent = GetAttachParent() ? GetAttachParent()->GetComponentQuat() : FQuat::Identity;
    SetRelativeRotation(Parent.Inverse()*Aim);
}

int32 UShipWeaponMountComponent::GetRemainingRounds() const
{
    return RuntimeWeapon ? FMath::Max(0, RuntimeWeapon->Ammo()) : 0;
}
int32 UShipWeaponMountComponent::GetStoreCapacity() const
{
    if (!RuntimeWeapon || !RuntimeWeapon->Design()) return 0;
    return RuntimeWeapon->Design()->nstores>0 ? RuntimeWeapon->GetTurretBarrelCount() :
        FMath::Max(0,RuntimeWeapon->Design()->ammo*RuntimeWeapon->GetTurretBarrelCount());
}
bool UShipWeaponMountComponent::IsStoreAvailable() const
{
    return RuntimeWeapon && RuntimeWeapon->GetAvailability()>1.0;
}
void UShipWeaponMountComponent::UpdateRuntimeStore()
{
    if (!bLoadoutHardpoint) return;
    // Primary guns retain their hull presentation even with an empty magazine.
    if (RuntimeWeapon && RuntimeWeapon->Design() && RuntimeWeapon->Design()->primary)
    {
        if (AActor* Gun = GetChildActor()) Gun->SetActorHiddenInGame(false);
        return;
    }
    const int32 Rounds = GetRemainingRounds();
    const int32 Capacity = GetStoreCapacity();
    const bool Available = IsStoreAvailable();
    AActor* Child = GetChildActor();
    // Empty missile stations should release their carried actor, rather than
    // retain an invisible child for the rest of the fighter's lifetime. Fired
    // missiles are separate world actors managed by the simulation shot.
    if (Rounds <= 0 && Child)
    {
        DestroyChildActor();
        Child = nullptr;
    }
    else if (Rounds > 0 && !Child && GetChildActorClass())
    {
        CreateChildActor();
        Child = GetChildActor();
    }
    if (Child) Child->SetActorHiddenInGame(Rounds<=0 || !Available);
    if (PresentedRounds!=Rounds || PresentedCapacity!=Capacity || bPresentedAvailable!=Available || PresentedStore.Get()!=Child)
    {
        PresentedRounds=Rounds; PresentedCapacity=Capacity; bPresentedAvailable=Available; PresentedStore=Child;
        if (Child && Child->GetClass()->ImplementsInterface(UShipStorePresentation::StaticClass()))
            IShipStorePresentation::Execute_UpdateStorePresentation(Child,Rounds,Capacity,Available);
    }
}
