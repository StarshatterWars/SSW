#include "MarauderShipActor.h"
AMarauderShipActor::AMarauderShipActor()
{

    SetActorScale3D(FVector(2.5f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Marauder"));
}

void AMarauderShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Port Cannon");
      Point.LocalOffset = FVector(60.000000f, -260.000000f, -32.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Starboard Cannon");
      Point.LocalOffset = FVector(60.000000f, 260.000000f, -32.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1");
      Point.LocalOffset = FVector(20.000000f, -110.000000f, 26.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2");
      Point.LocalOffset = FVector(20.000000f, 110.000000f, 26.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-1_Base");
      Point.LocalOffset = FVector(20.000000f, -110.000000f, 26.000000f);
      Point.LocalRotation = FRotator(0.000000f, 60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("PDB-2_Base");
      Point.LocalOffset = FVector(20.000000f, 110.000000f, 26.000000f);
      Point.LocalRotation = FRotator(0.000000f, -60.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
