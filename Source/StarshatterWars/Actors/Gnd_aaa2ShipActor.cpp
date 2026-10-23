#include "Gnd_aaa2ShipActor.h"
AGnd_aaa2ShipActor::AGnd_aaa2ShipActor()
{
    SetActorScale3D(FVector(2.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AGnd_aaa2ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 160.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Cannon_Base");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 160.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
