#include "ZZCAShipActor.h"
AZZCAShipActor::AZZCAShipActor()
{
    SetActorScale3D(FVector(4.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("ZZCA"));
}

void AZZCAShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Laser 1");
      Point.LocalOffset = FVector(128.000000f, -60.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Laser 2");
      Point.LocalOffset = FVector(128.000000f, 60.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Laser 3");
      Point.LocalOffset = FVector(128.000000f, 0.000000f, -60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Laser 4");
      Point.LocalOffset = FVector(128.000000f, 0.000000f, 60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
