#include "ImperialShipActor.h"
AImperialShipActor::AImperialShipActor()
{

    SetActorScale3D(FVector(4.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AImperialShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Gamma Laser 1");
      Point.LocalOffset = FVector(620.000000f, -42.000000f, -9.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Gamma Laser 2");
      Point.LocalOffset = FVector(620.000000f, 42.000000f, -9.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Missile 1");
      Point.LocalOffset = FVector(-159.000000f, -183.000000f, 29.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Missile 2");
      Point.LocalOffset = FVector(-159.000000f, 183.000000f, 29.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(153.000000f, 0.000000f, 68.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(313.000000f, -91.000000f, -47.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(313.000000f, 91.000000f, -47.000000f);
      Point.LocalRotation = FRotator(180.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(-228.000000f, 0.000000f, 92.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12");
      Point.LocalOffset = FVector(-172.000000f, -139.000000f, 72.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12");
      Point.LocalOffset = FVector(-172.000000f, 139.000000f, 72.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12");
      Point.LocalOffset = FVector(-172.000000f, 139.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12");
      Point.LocalOffset = FVector(-172.000000f, -139.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(153.000000f, 0.000000f, 68.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(313.000000f, -91.000000f, -47.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(313.000000f, 91.000000f, -47.000000f);
      Point.LocalRotation = FRotator(180.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(-228.000000f, 0.000000f, 92.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12_Base");
      Point.LocalOffset = FVector(-172.000000f, -139.000000f, 72.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12_Base");
      Point.LocalOffset = FVector(-172.000000f, 139.000000f, 72.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12_Base");
      Point.LocalOffset = FVector(-172.000000f, 139.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("RAIL12_Base");
      Point.LocalOffset = FVector(-172.000000f, -139.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
