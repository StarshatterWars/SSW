#include "VendettaShipActor.h"
AVendettaShipActor::AVendettaShipActor()
{

    SetActorScale3D(FVector(4.2f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AVendettaShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Gamma Laser 1");
      Point.LocalOffset = FVector(600.000000f, -8.000000f, 33.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Gamma Laser 2");
      Point.LocalOffset = FVector(600.000000f, 8.000000f, 33.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Missile 1");
      Point.LocalOffset = FVector(-92.000000f, -282.000000f, 40.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Missile 2");
      Point.LocalOffset = FVector(-92.000000f, 282.000000f, 40.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(147.000000f, 0.000000f, 90.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG");
      Point.LocalOffset = FVector(-548.000000f, 0.000000f, 105.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1");
      Point.LocalOffset = FVector(422.000000f, 0.000000f, 73.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2");
      Point.LocalOffset = FVector(506.000000f, 0.000000f, -143.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3");
      Point.LocalOffset = FVector(-360.000000f, 0.000000f, -104.000000f);
      Point.LocalRotation = FRotator(180.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 4");
      Point.LocalOffset = FVector(-96.000000f, 209.000000f, 88.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 5");
      Point.LocalOffset = FVector(-96.000000f, -209.000000f, 88.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(147.000000f, 0.000000f, 90.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("18INMAG_Base");
      Point.LocalOffset = FVector(-548.000000f, 0.000000f, 105.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1_Base");
      Point.LocalOffset = FVector(422.000000f, 0.000000f, 73.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2_Base");
      Point.LocalOffset = FVector(506.000000f, 0.000000f, -143.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3_Base");
      Point.LocalOffset = FVector(-360.000000f, 0.000000f, -104.000000f);
      Point.LocalRotation = FRotator(180.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 4_Base");
      Point.LocalOffset = FVector(-96.000000f, 209.000000f, 88.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 5_Base");
      Point.LocalOffset = FVector(-96.000000f, -209.000000f, 88.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
