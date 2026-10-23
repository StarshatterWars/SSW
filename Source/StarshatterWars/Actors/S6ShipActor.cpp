#include "S6ShipActor.h"
AS6ShipActor::AS6ShipActor()
{
    SetActorScale3D(FVector(9.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AS6ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1");
      Point.LocalOffset = FVector(920.000000f, 0.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2");
      Point.LocalOffset = FVector(920.000000f, 0.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3");
      Point.LocalOffset = FVector(-920.000000f, 0.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4");
      Point.LocalOffset = FVector(-920.000000f, 0.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1_Base");
      Point.LocalOffset = FVector(920.000000f, 0.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2_Base");
      Point.LocalOffset = FVector(920.000000f, 0.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3_Base");
      Point.LocalOffset = FVector(-920.000000f, 0.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4_Base");
      Point.LocalOffset = FVector(-920.000000f, 0.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
