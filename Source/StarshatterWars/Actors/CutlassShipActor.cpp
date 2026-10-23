#include "CutlassShipActor.h"
ACutlassShipActor::ACutlassShipActor()
{
    SetActorScale3D(FVector(0.24f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments();
}

void ACutlassShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(32.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-1");
      Point.LocalOffset = FVector(-25.000000f, -46.000000f, -10.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-2");
      Point.LocalOffset = FVector(-25.000000f, 64.000000f, -10.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
