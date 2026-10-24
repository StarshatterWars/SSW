#include "HulkVolnarisShipActor.h"
AHulkVolnarisShipActor::AHulkVolnarisShipActor()
{

    SetActorScale3D(FVector(3.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Hulk-Volnaris"));
}

void AHulkVolnarisShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 1");
      Point.LocalOffset = FVector(460.000000f, -34.000000f, -19.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 2");
      Point.LocalOffset = FVector(460.000000f, 34.000000f, -19.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Torpedo Banks");
      Point.LocalOffset = FVector(400.000000f, 0.000000f, -32.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1");
      Point.LocalOffset = FVector(340.000000f, 0.000000f, 45.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2");
      Point.LocalOffset = FVector(225.000000f, 0.000000f, 55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3");
      Point.LocalOffset = FVector(-255.000000f, 0.000000f, 55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 3.141590f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 1_Base");
      Point.LocalOffset = FVector(340.000000f, 0.000000f, 45.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 2_Base");
      Point.LocalOffset = FVector(225.000000f, 0.000000f, 55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB 3_Base");
      Point.LocalOffset = FVector(-255.000000f, 0.000000f, 55.000000f);
      Point.LocalRotation = FRotator(0.000000f, 3.141590f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
