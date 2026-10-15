#include "ShipActor.h"
#include "TurretCatalog.h"
#include "Components/ChildActorComponent.h"
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
}
void AShipActor::ClearTurretActors()
{
    for(UChildActorComponent* C:TurretComponents) if(IsValid(C)) C->DestroyComponent();
    TurretComponents.Reset();
    SpawnedTurretTypes.Reset();
}
void AShipActor::RebuildTurretActors()
{
    ClearTurretActors();
    if(!bEnableTurretActors || !GetWorld() || !VFXRoot) return;
    FTurretWeapons Weapons; CollectTurrets(RuntimeShip,Weapons);
    const int32 Count=RuntimeShip ? Weapons.Num() : TurretMountDefs.Num();
    for(int32 I=0;I<Count;++I) {
        const FName Type=RuntimeShip ? FName(ANSI_TO_TCHAR(Weapons[I]->Design()->name.data())) : TurretMountDefs[I].WeaponType;
        TSubclassOf<ATurretActor> Class=TurretCatalog::FindClass(Type);
        if(TurretVisuals) if(const auto* Entry=TurretVisuals->Classes.Find(Type)) if(*Entry) Class=*Entry;
        if(const auto* Entry=TurretClassOverrides.Find(Type)) if(*Entry) Class=*Entry;
        UChildActorComponent* C=NewObject<UChildActorComponent>(this);
        C->CreationMethod=EComponentCreationMethod::UserConstructionScript;
        C->SetupAttachment(VFXRoot);
        // Wep.def model scale is independent of ship.def scale, just as in legacy.
        C->SetAbsolute(false,false,true);
        C->SetChildActorClass(Class);
        C->RegisterComponent();
        SpawnedTurretTypes.Add(Type);
        TurretComponents.Add(C);
        if(!RuntimeShip) {
            const FShipTurretMountDef& M=TurretMountDefs[I];
            C->SetRelativeLocation(RuntimeEmitterLocalPosition(DisplayPoint(M.LegacyLocation)));
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
        C->SetRelativeLocation(RuntimeEmitterLocalPosition(DisplayPoint(Weapons[I]->GetTurretMountRelative())));
        if(ATurretActor* Turret=Cast<ATurretActor>(C->GetChildActor())) Turret->SyncFromWeapon(*Weapons[I],*RuntimeShip);
    }
}
