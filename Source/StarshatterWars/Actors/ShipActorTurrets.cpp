#include "ShipActor.h"
#include "TurretCatalog.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Ship.h"
#include "ShipDesign.h"
#include "Weapon.h"
#include "WeaponGroup.h"

namespace {
using FTurretWeapons = TArray<Weapon*,TInlineAllocator<32>>;
void CollectTurrets(Ship* Runtime, FTurretWeapons& Out)
{
    if (!Runtime) return;
    auto& Groups=Runtime->GetWeapons();
    for(int32 G=0;G<Groups.size();++G) {
        if(!Groups[G]) continue;
        for(int32 W=0;W<Groups[G]->NumWeapons();++W) {
            Weapon* Item=Groups[G]->GetWeapon(W);
            if(Item && Item->Design() && Item->Design()->turret.length()>0) Out.Add(Item);
        }
    }
}
FVector DisplayPoint(const FVector& P) { return FVector(P.Z,P.X,P.Y); }
}
void AShipActor::ConfigureTurretsFromShipDef(FName ShipDef)
{
    TurretCatalog::ShipMounts(ShipDef,TurretMountDefs);
    // Called by the native ship constructor. Never rebuild these in OnConstruction:
    // that would remove inherited components and overwrite Blueprint adjustments.
    if (!TurretMountRoot || NativeTurretMounts.Num()) return;
    for (int32 I=0; I<TurretMountDefs.Num(); ++I) {
        const FShipTurretMountDef& Def=TurretMountDefs[I];
        FString Label=Def.WeaponType.ToString();
        for (TCHAR& Ch : Label) if (!FChar::IsAlnum(Ch)) Ch=TEXT('_');
        const FName Name(*FString::Printf(TEXT("TurretMount_%02d_%s"),I+1,*Label));
        USceneComponent* Mount=CreateDefaultSubobject<USceneComponent>(Name);
        Mount->SetupAttachment(TurretMountRoot);
        Mount->bEditableWhenInherited=true;
        Mount->SetRelativeLocation(DisplayPoint(Def.LegacyLocation));
        NativeTurretMounts.Add(Mount);
        // Keep existing SceneComponent mount names/types to preserve BP overrides.
        const FName SlotName(*FString::Printf(TEXT("TurretBP_%02d_%s"),I+1,*Label));
        UChildActorComponent* Slot=CreateDefaultSubobject<UChildActorComponent>(SlotName);
        Slot->SetupAttachment(Mount);
        Slot->bEditableWhenInherited=true;
        Slot->SetAbsolute(false,false,true);
        // Deliberately no default class: direct BP assignment wins over catalogs.
        NativeTurretBPSlots.Add(Slot);
    }
}
void AShipActor::ClearTurretActors()
{
    // Native slots and their BP class assignments belong to the ship Blueprint.
    for(UChildActorComponent* Slot:NativeTurretBPSlots)
        if(IsValid(Slot)) if(AActor* Child=Slot->GetChildActor()) Child->SetActorHiddenInGame(true);
    for(UChildActorComponent* C:TurretComponents)
        if(IsValid(C) && !NativeTurretBPSlots.Contains(C)) C->DestroyComponent();
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();
}
void AShipActor::RebuildTurretActors()
{
    ClearTurretActors();
    if(!bEnableTurretActors || !GetWorld() || !VFXRoot) return;
    FTurretWeapons Weapons; CollectTurrets(RuntimeShip,Weapons);
    const int32 Count=RuntimeShip ? Weapons.Num() : TurretMountDefs.Num();
    TSet<int32> UsedMounts;
    for(int32 I=0;I<Count;++I) {
        const FName Type=RuntimeShip ? FName(ANSI_TO_TCHAR(Weapons[I]->Design()->name.data())) : TurretMountDefs[I].WeaponType;
        TSubclassOf<ATurretActor> Class=TurretCatalog::FindClass(Type);
        if(TurretVisuals) if(const auto* Entry=TurretVisuals->Classes.Find(Type)) if(*Entry) Class=*Entry;
        if(const auto* Entry=TurretClassOverrides.Find(Type)) if(*Entry) Class=*Entry;

        int32 MountIndex=INDEX_NONE;
        if (!RuntimeShip) MountIndex=I;
        else {
            // Runtime weapons are grouped; their order need not match ship.def.
            const double DesignScale=RuntimeShip->Design() && FMath::Abs(RuntimeShip->Design()->scale)>SMALL_NUMBER ? RuntimeShip->Design()->scale : 1.0;
            const FVector AuthoredLocation=Weapons[I]->GetTurretMountRelative()/DesignScale;
            double BestDistance=TNumericLimits<double>::Max();
            for(int32 M=0;M<TurretMountDefs.Num();++M) {
                if(UsedMounts.Contains(M) || !NativeTurretMounts.IsValidIndex(M) || !IsValid(NativeTurretMounts[M]) || TurretMountDefs[M].WeaponType!=Type) continue;
                const double Distance=FVector::DistSquared(AuthoredLocation,TurretMountDefs[M].LegacyLocation);
                if(Distance<BestDistance) {BestDistance=Distance;MountIndex=M;}
            }
        }
        USceneComponent* Mount=NativeTurretMounts.IsValidIndex(MountIndex) ? NativeTurretMounts[MountIndex].Get() : nullptr;
        UChildActorComponent* C=NativeTurretBPSlots.IsValidIndex(MountIndex) ? NativeTurretBPSlots[MountIndex].Get() : nullptr;
        const bool bDirectAssignment=IsValid(C) && C->GetChildActorClass()!=nullptr;
        if(bDirectAssignment) {
            // Never replace an explicit per-mount Child Actor Class.
            if(AActor* Child=C->GetChildActor()) Child->SetActorHiddenInGame(false);
        }
        else {
            C=NewObject<UChildActorComponent>(this);
            C->CreationMethod=EComponentCreationMethod::UserConstructionScript;
            C->SetupAttachment(IsValid(Mount) ? Mount : VFXRoot.Get());
            C->SetAbsolute(false,false,true);
            C->SetChildActorClass(Class);
            C->RegisterComponent();
        }
        if(IsValid(Mount)) UsedMounts.Add(MountIndex);
        SpawnedTurretTypes.Add(Type);
        TurretComponents.Add(C);
        if(!RuntimeShip) {
            const FShipTurretMountDef& M=TurretMountDefs[I];
            if(!Mount) C->SetRelativeLocation(RuntimeEmitterLocalPosition(DisplayPoint(M.LegacyLocation)));
            if(ATurretActor* Turret=Cast<ATurretActor>(C->GetChildActor())) Turret->SetPreviewAim(M.AzimuthRadians,M.ElevationRadians);
        }
    }
    if(RuntimeShip) UpdateTurretActors();
}
void AShipActor::UpdateTurretActors()
{
    if(!bEnableTurretActors) { if(TurretComponents.Num()) ClearTurretActors(); return; }
    FTurretWeapons Weapons; CollectTurrets(RuntimeShip,Weapons);
    if(Weapons.Num()!=TurretComponents.Num()) { RebuildTurretActors(); return; }
    for(int32 I=0;I<Weapons.Num();++I) {
        const FName Type(ANSI_TO_TCHAR(Weapons[I]->Design()->name.data()));
        if(!SpawnedTurretTypes.IsValidIndex(I) || SpawnedTurretTypes[I]!=Type) { RebuildTurretActors(); return; }
        UChildActorComponent* C=TurretComponents[I];
        if(!IsValid(C)) continue;
        // Native mounts belong to the Blueprint. Do not overwrite their transform.
        if(C->GetAttachParent()==VFXRoot)
            C->SetRelativeLocation(RuntimeEmitterLocalPosition(DisplayPoint(Weapons[I]->GetTurretMountRelative())));
        if(ATurretActor* Turret=Cast<ATurretActor>(C->GetChildActor())) Turret->SyncFromWeapon(*Weapons[I],*RuntimeShip);
    }
}
