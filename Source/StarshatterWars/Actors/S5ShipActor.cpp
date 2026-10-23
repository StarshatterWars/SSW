#include "S5ShipActor.h"
AS5ShipActor::AS5ShipActor()
{
    SetActorScale3D(FVector(8.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AS5ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A");
      Point.LocalOffset = FVector(0.000000f, 1150.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B");
      Point.LocalOffset = FVector(0.000000f, 1150.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A");
      Point.LocalOffset = FVector(-1150.000000f, 0.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B");
      Point.LocalOffset = FVector(-1150.000000f, 0.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A");
      Point.LocalOffset = FVector(0.000000f, -1150.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B");
      Point.LocalOffset = FVector(0.000000f, -1150.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4A");
      Point.LocalOffset = FVector(1150.000000f, 0.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4B");
      Point.LocalOffset = FVector(1150.000000f, 0.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A_Base");
      Point.LocalOffset = FVector(0.000000f, 1150.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B_Base");
      Point.LocalOffset = FVector(0.000000f, 1150.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A_Base");
      Point.LocalOffset = FVector(-1150.000000f, 0.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B_Base");
      Point.LocalOffset = FVector(-1150.000000f, 0.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A_Base");
      Point.LocalOffset = FVector(0.000000f, -1150.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B_Base");
      Point.LocalOffset = FVector(0.000000f, -1150.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4A_Base");
      Point.LocalOffset = FVector(1150.000000f, 0.000000f, 488.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4B_Base");
      Point.LocalOffset = FVector(1150.000000f, 0.000000f, 320.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
