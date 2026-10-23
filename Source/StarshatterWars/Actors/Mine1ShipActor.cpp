#include "Mine1ShipActor.h"
AMine1ShipActor::AMine1ShipActor()
{
    SetActorScale3D(FVector(1.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AMine1ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Fwd Cannon");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Fwd Cannon_Base");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
