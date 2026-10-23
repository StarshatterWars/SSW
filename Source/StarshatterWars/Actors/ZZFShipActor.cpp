#include "ZZFShipActor.h"
AZZFShipActor::AZZFShipActor()
{
    SetActorScale3D(FVector(0.25f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AZZFShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(10.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
