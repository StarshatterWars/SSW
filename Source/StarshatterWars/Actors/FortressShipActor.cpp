#include "FortressShipActor.h"
AFortressShipActor::AFortressShipActor()
{
    SetActorScale3D(FVector(11.0f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Fortress"));
}

void AFortressShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A");
      Point.LocalOffset = FVector(320.000000f, 0.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B");
      Point.LocalOffset = FVector(320.000000f, 0.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A");
      Point.LocalOffset = FVector(-160.000000f, 280.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, 120.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B");
      Point.LocalOffset = FVector(-160.000000f, 280.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, 120.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A");
      Point.LocalOffset = FVector(-160.000000f, -280.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, -120.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B");
      Point.LocalOffset = FVector(-160.000000f, -280.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, -120.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1A_Base");
      Point.LocalOffset = FVector(320.000000f, 0.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 1B_Base");
      Point.LocalOffset = FVector(320.000000f, 0.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2A_Base");
      Point.LocalOffset = FVector(-160.000000f, 280.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, 120.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 2B_Base");
      Point.LocalOffset = FVector(-160.000000f, 280.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, 120.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3A_Base");
      Point.LocalOffset = FVector(-160.000000f, -280.000000f, 388.000000f);
      Point.LocalRotation = FRotator(0.000000f, -120.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret 3B_Base");
      Point.LocalOffset = FVector(-160.000000f, -280.000000f, 268.000000f);
      Point.LocalRotation = FRotator(0.000000f, -120.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
