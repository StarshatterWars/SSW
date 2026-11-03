#include "ShipActor.h"
#include "Components/ChildActorComponent.h"
#include "ShipWeaponMountComponent.h"
#include "TurretActor.h"
#include "TurretBaseActor.h"
#include "TurretBaseComponent.h"
#include "TurretBaseCatalog.h"
#include "Components/StaticMeshComponent.h"
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
    for(const auto& Point:NativeWeaponHardpoints)
        if(const auto* Mount=Cast<UShipWeaponMountComponent>(Point.Get()))
            if(auto* Gun=Mount->GetTurretVisualActor())Gun->SetSeparateBasePresentation(false);
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();
    TurretVisuals.Reset();
}

void AShipActor::RebuildTurretActors()
{
    ClearTurretActors();
    NativeTurretMounts = AuthoredTurretPointComponents;
    TurretMountDefs = TurretBasePointDefs;
    for(const auto& Point:NativeWeaponHardpoints)
        if(auto* Mount=Cast<UShipWeaponMountComponent>(Point.Get()))
        {
            const FName Type=Mount->RuntimeWeapon?FName(ANSI_TO_TCHAR(Mount->RuntimeWeapon->Design()->name.data()))
                :(Mount->AllowedWeaponTypes.Num()?Mount->AllowedWeaponTypes[0]:NAME_None);
            if(!Mount->bLoadoutHardpoint || Mount->RuntimeWeapon)Mount->ConfigureTurretVisual(Type);
            if(auto* Gun=Mount->GetTurretVisualActor()){
                Gun->SetActorHiddenInGame(!bEnableTurretActors);TurretComponents.Add(Gun);SpawnedTurretTypes.Add(Gun->GetClass());
            }
        }
    for (int32 Index = 0; Index < AuthoredTurretPointComponents.Num(); ++Index)
    {
        UChildActorComponent* Mount = Cast<UChildActorComponent>(AuthoredTurretPointComponents[Index].Get());
        if (!Mount) continue;
        if(auto* Base=Cast<UTurretBaseComponent>(Mount)) Base->Configure(Base->WeaponType);
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
            if(const auto* Base=Cast<UTurretBaseComponent>(Mount))
                if(NativeWeaponHardpoints.IsValidIndex(Base->SourceWeaponIndex))
                    if(const auto* GunMount=Cast<UChildActorComponent>(NativeWeaponHardpoints[Base->SourceWeaponIndex].Get()))
                        if(auto* Gun=Cast<ATurretActor>(GunMount->GetChildActor()))
                            if(const auto* Visual=Cast<ATurretBaseActor>(Child))
                                Gun->SetSeparateBasePresentation(bEnableTurretActors && Visual->BaseMesh && Visual->BaseMesh->GetStaticMesh());
        }
    }
}

void AShipActor::UpdateTurretActors()
{
    if (!RuntimeShip) return;
    TSet<USceneComponent*> DrivenBases;
    TSet<ATurretActor*> SplitGuns;
    for (int32 WeaponIndex=0; WeaponIndex<NativeWeaponHardpoints.Num(); ++WeaponIndex)
    {
        auto* Mount=Cast<UShipWeaponMountComponent>(NativeWeaponHardpoints[WeaponIndex].Get());
        Weapon* W=Mount ? Mount->RuntimeWeapon : nullptr;
        const WeaponDesign* Design=W ? W->Design() : nullptr;
        if (!Mount || !Design || !Design->turret.length()) continue;
        if(Mount->bLoadoutHardpoint){
            if(auto* Visual=Mount->GetTurretVisualActor())Visual->SyncFromWeaponFrame(*W,RuntimeShip->GetCam(),GetActorQuat());
            continue;
        }
        Mount->UpdateRuntimeAim(RuntimeShip->GetCam(),GetActorQuat());
        if (ATurretActor* Visual=Mount->GetTurretVisualActor())
            Visual->SyncFromWeaponFrame(*W,RuntimeShip->GetCam(),GetActorQuat());
        // Turret base entries are authored in weapon order and share their mount
        // position. Match by authored position; never attach/spawn a second gun.
        for (int32 Index=0; Index<AuthoredTurretPointComponents.Num(); ++Index)
        {
            USceneComponent* Base=AuthoredTurretPointComponents[Index].Get();
            if (!Base || DrivenBases.Contains(Base))continue;
            const auto* Typed=Cast<UTurretBaseComponent>(Base);
            if(Typed && Typed->SourceWeaponIndex!=INDEX_NONE)
            {if(Typed->SourceWeaponIndex!=WeaponIndex)continue;}
            else if(!TurretBasePointDefs.IsValidIndex(Index) || !WeaponMountPointDefs.IsValidIndex(WeaponIndex) ||
                !TurretBasePointDefs[Index].LocalOffset.Equals(WeaponMountPointDefs[WeaponIndex].LocalOffset,.001))continue;
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
            if(bEnableTurretActors)
                if(const auto* BaseSlot=Cast<UChildActorComponent>(Base))
                    if(const auto* Visual=Cast<ATurretBaseActor>(BaseSlot->GetChildActor()))
                        if(Visual->BaseMesh && Visual->BaseMesh->GetStaticMesh())
                            if(auto* Gun=Mount->GetTurretVisualActor())
                            {Gun->SetSeparateBasePresentation(true);SplitGuns.Add(Gun);}
            DrivenBases.Add(Base); break;
        }
    }
    for(const auto& Point:NativeWeaponHardpoints)
        if(const auto* Mount=Cast<UShipWeaponMountComponent>(Point.Get()))
            if(auto* Gun=Mount->GetTurretVisualActor())
                if(!SplitGuns.Contains(Gun))Gun->SetSeparateBasePresentation(false);
    for (auto It=RuntimeTurretBaseRestRotations.CreateIterator(); It; ++It)
        if (!DrivenBases.Contains(It.Key()))
        {
            if (IsValid(It.Key())) It.Key()->SetRelativeRotation(It.Value());
            It.RemoveCurrent();
        }
}

void AShipActor::ConfigureRuntimeTurretBases()
{
    for(int32 Index=0;Index<NativeWeaponHardpoints.Num();++Index)
    {
        auto* Mount=Cast<UShipWeaponMountComponent>(NativeWeaponHardpoints[Index].Get());
        if(!Mount || Mount->bLoadoutHardpoint || !Mount->RuntimeWeapon || !Mount->RuntimeWeapon->Design())continue;
        const FName Type(ANSI_TO_TCHAR(Mount->RuntimeWeapon->Design()->name.data()));
        FString Model;float Scale;TurretBaseCatalog::Definition(Type,Model,Scale);
        UTurretBaseComponent* Base=nullptr;
        for(const auto& Point:AuthoredTurretPointComponents)
            if(auto* Candidate=Cast<UTurretBaseComponent>(Point.Get()))
                if(Candidate->SourceWeaponIndex==Index){Base=Candidate;break;}
        if(!Base && !Model.IsEmpty() && GetWorld())
        {
            Base=NewObject<UTurretBaseComponent>(this,MakeUniqueObjectName(this,UTurretBaseComponent::StaticClass(),TEXT("RuntimeTurretBase")));
            AddInstanceComponent(Base);Base->SetupAttachment(TurretMountRoot);Base->SourceWeaponIndex=Index;
            Base->Configure(Type);Base->RegisterComponent();Base->SetWorldTransform(Mount->GetComponentTransform());
            AuthoredTurretPointComponents.Add(Base);
            FShipPointDef Def;Def.PointName=Base->GetFName();Def.LocalOffset=Base->GetRelativeLocation();Def.LocalRotation=Base->GetRelativeRotation();
            TurretBasePointDefs.Add(Def);
        }
        if(Base)Base->Configure(Type);
    }
    NativeTurretMounts=AuthoredTurretPointComponents;
    SyncShipAttachmentCounts();
}
