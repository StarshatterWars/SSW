#include "CutlassShipActor.h"
ACutlassShipActor::ACutlassShipActor()
{
    SetActorScale3D(FVector(0.24f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Cutlass"));
    InitializeNativeFighterAttachments(TEXT(""), 1.000000f, FVector(8.000000f, 0.000000f, 145.000000f), {
        FFighterLandingGearDefinition(TEXT("LeftSkid"), FVector(0.000000f, -20.000000f, -16.000000f), FVector(0.000000f, -48.000000f, -44.000000f)),
        FFighterLandingGearDefinition(TEXT("RightSkid"), FVector(0.000000f, 20.000000f, -16.000000f), FVector(0.000000f, 48.000000f, -44.000000f)),
    });

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
