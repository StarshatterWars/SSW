#include "ShipActor.h"
#include "ShipWeaponMountComponent.h"
#include "Components/SceneComponent.h"
#include "Camera.h"
#include "Math/RotationMatrix.h"
void AShipActor::ConfigureWeaponHardpointsFromShipDef(FName ShipDef)
{
    if(!WeaponHardpointRoot || NativeWeaponHardpoints.Num()) return;
    TArray<FNativeWeaponMountDef> Definitions;
    ShipWeaponMountCatalog::Get(ShipDef,Definitions);
    int32 LoadoutIndex=0, FixedIndex=0;
    for(const FNativeWeaponMountDef& Def:Definitions) {
        FString Label=Def.Name.ToString();
        for(TCHAR& Ch:Label) if(!FChar::IsAlnum(Ch)) Ch=TEXT('_');
        const int32 Index=Def.bLoadout ? ++LoadoutIndex : ++FixedIndex;
        const FName Name(*FString::Printf(TEXT("%s_%02d_%s"),Def.bLoadout ? TEXT("MissileHardpointBP") : TEXT("WeaponBP"),Index,*Label));
        UShipWeaponMountComponent* Slot=CreateDefaultSubobject<UShipWeaponMountComponent>(Name);
        Slot->SetupAttachment(WeaponHardpointRoot);
        Slot->StationName=Def.Name;
        Slot->bLoadoutHardpoint=Def.bLoadout;
        Slot->AllowedWeaponTypes=Def.Types;
        Slot->LegacyLocation=Def.Location;
        Slot->LegacyMuzzle=Def.Muzzle;
        Slot->SetRelativeLocation(FVector(Def.Location.Z,Def.Location.X,Def.Location.Y));
        Camera Aim; Aim.Yaw(Def.Azimuth); Aim.Pitch(Def.Elevation);
        const FVector F=Aim.vpn(), U=Aim.vup();
        Slot->SetRelativeRotation(FRotationMatrix::MakeFromXZ(FVector(F.Z,F.X,F.Y),FVector(U.Z,U.X,U.Y)).Rotator());
        // Ship-scale is inherited once. Child BP meshes can supply model import scale.
        NativeWeaponHardpoints.Add(Slot);
    }
}
