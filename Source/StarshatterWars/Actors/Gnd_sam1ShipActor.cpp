#include "Gnd_sam1ShipActor.h"
AGnd_sam1ShipActor::AGnd_sam1ShipActor()
{
    SetActorScale3D(FVector(2.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Gnd_sam1"));
}

void AGnd_sam1ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("SAM");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 90.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
