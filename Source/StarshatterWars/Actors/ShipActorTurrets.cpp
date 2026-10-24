#include "ShipActor.h"
#include "Components/ChildActorComponent.h"
#include "ShipWeaponMountComponent.h"
#include "TurretActor.h"
#include "Ship.h"
#include "Weapon.h"
#include "WeaponDesign.h"
#include "WeaponPresentationFrame.h"

void AShipActor::ConfigureTurretsFromShipDef(const FString& ShipName)
{
    // Compatibility entry point; constructors author native points after defaults.
    // In particular, Orion calls here before its authored arrays are populated.
    NativeTurretMounts = AuthoredTurretPointComponents;
}

void AShipActor::ClearTurretActors()
{
    // UChildActorComponent owns the lifetime of each visual actor.
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();
    TurretVisuals.Reset();
}

void AShipActor::RebuildTurretActors()
{
    ClearTurretActors();
    NativeTurretMounts = AuthoredTurretPointComponents;
    TurretMountDefs = TurretBasePointDefs;
    for (int32 Index = 0; Index < AuthoredTurretPointComponents.Num(); ++Index)
    {
        UChildActorComponent* Mount = Cast<UChildActorComponent>(AuthoredTurretPointComponents[Index].Get());
        if (!Mount) continue;
        if (bEnableTurretActors)
        {
            if (TurretBlueprintClasses.IsValidIndex(Index) && TurretBlueprintClasses[Index])
                Mount->SetChildActorClass(TurretBlueprintClasses[Index]);
            else if (TurretClassOverrides.IsValidIndex(Index) && TurretClassOverrides[Index])
                Mount->SetChildActorClass(TurretClassOverrides[Index]);
            else if (NativeTurretBPSlots.IsValidIndex(Index) && NativeTurretBPSlots[Index])
                Mount->SetChildActorClass(NativeTurretBPSlots[Index]);
        }
        Mount->SetVisibility(bEnableTurretActors, true);
        if (AActor* Child = Mount->GetChildActor())
        {
            Child->SetActorHiddenInGame(!bEnableTurretActors);
            TurretComponents.Add(Child);
            SpawnedTurretTypes.Add(Mount->GetChildActorClass());
        }
    }
}

void AShipActor::UpdateTurretActors()
{
    if (!RuntimeShip) return;
    TSet<USceneComponent*> DrivenBases;
    for (int32 WeaponIndex=0; WeaponIndex<NativeWeaponHardpoints.Num(); ++WeaponIndex)
    {
        auto* Mount=Cast<UShipWeaponMountComponent>(NativeWeaponHardpoints[WeaponIndex].Get());
        Weapon* W=Mount ? Mount->RuntimeWeapon : nullptr;
        const WeaponDesign* Design=W ? W->Design() : nullptr;
        if (!Mount || Mount->bLoadoutHardpoint || !Design || !Design->turret.length()) continue;
        Mount->UpdateRuntimeAim(RuntimeShip->GetCam(),GetActorQuat());
        if (ATurretActor* Visual=Cast<ATurretActor>(Mount->GetChildActor()))
            Visual->SyncFromWeaponFrame(*W,RuntimeShip->GetCam(),GetActorQuat());
        // Turret base entries are authored in weapon order and share their mount
        // position. Match by authored position; never attach/spawn a second gun.
        if (!WeaponMountPointDefs.IsValidIndex(WeaponIndex)) continue;
        for (int32 Index=0; Index<AuthoredTurretPointComponents.Num(); ++Index)
        {
            USceneComponent* Base=AuthoredTurretPointComponents[Index].Get();
            if (!Base || DrivenBases.Contains(Base) || !TurretBasePointDefs.IsValidIndex(Index) ||
                !TurretBasePointDefs[Index].LocalOffset.Equals(WeaponMountPointDefs[WeaponIndex].LocalOffset,0.001)) continue;
            if (!RuntimeTurretBaseRestRotations.Contains(Base))
                RuntimeTurretBaseRestRotations.Add(Base,Base->GetRelativeRotation().Quaternion());
            Camera BaseFrame; BaseFrame.Clone(RuntimeShip->GetCam());
            if (Design->turret_axis==1) BaseFrame.Pitch(W->GetElevation()+W->GetTurretElevationOffset());
            else BaseFrame.Yaw(W->GetAzimuth()+W->GetTurretAzimuthOffset());
            const FQuat World=WeaponPresentationFrame::Display(BaseFrame,RuntimeShip->GetCam(),GetActorQuat());
            const FQuat Parent=Base->GetAttachParent() ? Base->GetAttachParent()->GetComponentQuat() : FQuat::Identity;
            Base->SetRelativeRotation(Parent.Inverse()*World);
            if (auto* Child=Cast<UChildActorComponent>(Base))
                if (ATurretActor* Visual=Cast<ATurretActor>(Child->GetChildActor()))
                    Visual->SyncFromWeaponFrame(*W,RuntimeShip->GetCam(),GetActorQuat());
            DrivenBases.Add(Base); break;
        }
    }
    for (auto It=RuntimeTurretBaseRestRotations.CreateIterator(); It; ++It)
        if (!DrivenBases.Contains(It.Key()))
        {
            if (IsValid(It.Key())) It.Key()->SetRelativeRotation(It.Value());
            It.RemoveCurrent();
        }
}
