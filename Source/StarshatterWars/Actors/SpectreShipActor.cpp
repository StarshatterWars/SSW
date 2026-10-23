#include "SpectreShipActor.h"
ASpectreShipActor::ASpectreShipActor()
{

    SetActorScale3D(FVector(2.5f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void ASpectreShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 1");
      Point.LocalOffset = FVector(400.000000f, -36.000000f, -9.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 2");
      Point.LocalOffset = FVector(400.000000f, 36.000000f, -9.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 3");
      Point.LocalOffset = FVector(400.000000f, -14.000000f, -23.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("XRay Laser 4");
      Point.LocalOffset = FVector(400.000000f, 14.000000f, -23.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Starboard Torpedo");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port Torpedo");
      Point.LocalOffset = FVector(0.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port/Fwd Phalanx");
      Point.LocalOffset = FVector(-106.000000f, -211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd/Fwd Phalanx");
      Point.LocalOffset = FVector(-106.000000f, 211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port/Aft Phalanx");
      Point.LocalOffset = FVector(-255.000000f, -211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd/Aft Phalanx");
      Point.LocalOffset = FVector(-255.000000f, 211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port/Fwd Phalanx_Base");
      Point.LocalOffset = FVector(-106.000000f, -211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd/Fwd Phalanx_Base");
      Point.LocalOffset = FVector(-106.000000f, 211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Port/Aft Phalanx_Base");
      Point.LocalOffset = FVector(-255.000000f, -211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Stbd/Aft Phalanx_Base");
      Point.LocalOffset = FVector(-255.000000f, 211.000000f, -50.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
