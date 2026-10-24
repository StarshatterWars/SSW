#include "TigerShipActor.h"
ATigerShipActor::ATigerShipActor()
{

    SetActorScale3D(FVector(1.8f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Tiger"));
}

void ATigerShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 1");
      Point.LocalOffset = FVector(110.000000f, 74.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 2");
      Point.LocalOffset = FVector(110.000000f, -74.000000f, -12.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Interceptor 1");
      Point.LocalOffset = FVector(300.000000f, -20.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(220.000000f, 0.000000f, 75.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(-420.000000f, 0.000000f, 135.000000f);
      Point.LocalRotation = FRotator(0.000000f, 3.141590f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(220.000000f, 0.000000f, 75.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(-420.000000f, 0.000000f, 135.000000f);
      Point.LocalRotation = FRotator(0.000000f, 3.141590f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
