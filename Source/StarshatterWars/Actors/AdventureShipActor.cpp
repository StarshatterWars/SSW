#include "AdventureShipActor.h"
AAdventureShipActor::AAdventureShipActor()
{
    SetActorScale3D(FVector(2.5f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AAdventureShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(130.000000f, 240.000000f, 52.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(130.000000f, -240.000000f, 52.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3");
      Point.LocalOffset = FVector(130.000000f, 130.000000f, -60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4");
      Point.LocalOffset = FVector(130.000000f, -130.000000f, -60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5");
      Point.LocalOffset = FVector(-580.000000f, 90.000000f, 80.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-6");
      Point.LocalOffset = FVector(-580.000000f, -90.000000f, 80.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(130.000000f, 240.000000f, 52.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(130.000000f, -240.000000f, 52.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3_Base");
      Point.LocalOffset = FVector(130.000000f, 130.000000f, -60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4_Base");
      Point.LocalOffset = FVector(130.000000f, -130.000000f, -60.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5_Base");
      Point.LocalOffset = FVector(-580.000000f, 90.000000f, 80.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-6_Base");
      Point.LocalOffset = FVector(-580.000000f, -90.000000f, 80.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
