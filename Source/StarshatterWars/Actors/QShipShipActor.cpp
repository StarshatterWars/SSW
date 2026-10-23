#include "QShipShipActor.h"
AQShipShipActor::AQShipShipActor()
{

    SetActorScale3D(FVector(2.5f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void AQShipShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Port Cannon");
      Point.LocalOffset = FVector(360.000000f, -30.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Starboard Cannon");
      Point.LocalOffset = FVector(360.000000f, 30.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(0.000000f, -58.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(0.000000f, 58.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3");
      Point.LocalOffset = FVector(0.000000f, -58.000000f, -64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4");
      Point.LocalOffset = FVector(0.000000f, 58.000000f, -64.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(0.000000f, -58.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(0.000000f, 58.000000f, 64.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-3_Base");
      Point.LocalOffset = FVector(0.000000f, -58.000000f, -64.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-4_Base");
      Point.LocalOffset = FVector(0.000000f, 58.000000f, -64.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
