#include "ZZDDShipActor.h"
AZZDDShipActor::AZZDDShipActor()
{
    SetActorScale3D(FVector(2.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("ZZDD"));
}

void AZZDDShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Laser 1");
      Point.LocalOffset = FVector(-228.000000f, 0.000000f, 200.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Laser 2");
      Point.LocalOffset = FVector(-228.000000f, 0.000000f, -200.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
