#include "HulkBroadswordShipActor.h"
AHulkBroadswordShipActor::AHulkBroadswordShipActor()
{

    SetActorScale3D(FVector(3.5f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Hulk-Broadsword"));
}

void AHulkBroadswordShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 1");
      Point.LocalOffset = FVector(340.000000f, -45.000000f, -33.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 2");
      Point.LocalOffset = FVector(340.000000f, 44.000000f, -33.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 3");
      Point.LocalOffset = FVector(340.000000f, -45.000000f, -55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 4");
      Point.LocalOffset = FVector(340.000000f, 44.000000f, -55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Missile Array");
      Point.LocalOffset = FVector(220.000000f, 0.000000f, -1.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Rail-1");
      Point.LocalOffset = FVector(-27.000000f, 0.000000f, 73.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Rail-2");
      Point.LocalOffset = FVector(-267.000000f, 0.000000f, 71.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(299.000000f, -31.000000f, 81.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(299.000000f, 31.000000f, 81.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3");
      Point.LocalOffset = FVector(96.000000f, 0.000000f, -110.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4");
      Point.LocalOffset = FVector(-486.000000f, 0.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5");
      Point.LocalOffset = FVector(-486.000000f, 0.000000f, 14.000000f);
      Point.LocalRotation = FRotator(180.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Rail-1_Base");
      Point.LocalOffset = FVector(-27.000000f, 0.000000f, 73.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Rail-2_Base");
      Point.LocalOffset = FVector(-267.000000f, 0.000000f, 71.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(299.000000f, -31.000000f, 81.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(299.000000f, 31.000000f, 81.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3_Base");
      Point.LocalOffset = FVector(96.000000f, 0.000000f, -110.000000f);
      Point.LocalRotation = FRotator(-180.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4_Base");
      Point.LocalOffset = FVector(-486.000000f, 0.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5_Base");
      Point.LocalOffset = FVector(-486.000000f, 0.000000f, 14.000000f);
      Point.LocalRotation = FRotator(180.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
