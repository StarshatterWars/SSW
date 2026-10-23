#include "S7ShipActor.h"
AS7ShipActor::AS7ShipActor()
{
    SetActorScale3D(FVector(15.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AS7ShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Graser 1");
      Point.LocalOffset = FVector(100.000000f, 240.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Graser 2");
      Point.LocalOffset = FVector(100.000000f, -240.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Graser 3");
      Point.LocalOffset = FVector(-100.000000f, 240.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Graser 4");
      Point.LocalOffset = FVector(-100.000000f, -240.000000f, 50.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A");
      Point.LocalOffset = FVector(704.000000f, 105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A");
      Point.LocalOffset = FVector(-640.000000f, 105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A");
      Point.LocalOffset = FVector(704.000000f, -105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4A");
      Point.LocalOffset = FVector(-640.000000f, -105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B");
      Point.LocalOffset = FVector(704.000000f, 105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B");
      Point.LocalOffset = FVector(-640.000000f, 105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B");
      Point.LocalOffset = FVector(704.000000f, -105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4B");
      Point.LocalOffset = FVector(-640.000000f, -105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A_Base");
      Point.LocalOffset = FVector(704.000000f, 105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A_Base");
      Point.LocalOffset = FVector(-640.000000f, 105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A_Base");
      Point.LocalOffset = FVector(704.000000f, -105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4A_Base");
      Point.LocalOffset = FVector(-640.000000f, -105.000000f, 36.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B_Base");
      Point.LocalOffset = FVector(704.000000f, 105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B_Base");
      Point.LocalOffset = FVector(-640.000000f, 105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B_Base");
      Point.LocalOffset = FVector(704.000000f, -105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 4B_Base");
      Point.LocalOffset = FVector(-640.000000f, -105.000000f, -24.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
