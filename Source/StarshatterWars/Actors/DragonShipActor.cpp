#include "DragonShipActor.h"
ADragonShipActor::ADragonShipActor()
{

    SetActorScale3D(FVector(3.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Dragon"));
}

void ADragonShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Laser 1");
      Point.LocalOffset = FVector(330.000000f, -30.000000f, -15.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Laser 2");
      Point.LocalOffset = FVector(330.000000f, 30.000000f, -15.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(1150.000000f, 200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(1150.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3");
      Point.LocalOffset = FVector(200.000000f, 200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4");
      Point.LocalOffset = FVector(200.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5");
      Point.LocalOffset = FVector(-1250.000000f, 90.000000f, 155.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-6");
      Point.LocalOffset = FVector(-1250.000000f, -90.000000f, 155.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-7");
      Point.LocalOffset = FVector(1150.000000f, 200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-8");
      Point.LocalOffset = FVector(1150.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-9");
      Point.LocalOffset = FVector(200.000000f, 200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-10");
      Point.LocalOffset = FVector(200.000000f, -200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(1150.000000f, 200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(1150.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3_Base");
      Point.LocalOffset = FVector(200.000000f, 200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4_Base");
      Point.LocalOffset = FVector(200.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-5_Base");
      Point.LocalOffset = FVector(-1250.000000f, 90.000000f, 155.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-6_Base");
      Point.LocalOffset = FVector(-1250.000000f, -90.000000f, 155.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-7_Base");
      Point.LocalOffset = FVector(1150.000000f, 200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-8_Base");
      Point.LocalOffset = FVector(1150.000000f, -200.000000f, 118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-9_Base");
      Point.LocalOffset = FVector(200.000000f, 200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, 90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-10_Base");
      Point.LocalOffset = FVector(200.000000f, -200.000000f, -118.000000f);
      Point.LocalRotation = FRotator(0.000000f, -90.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
