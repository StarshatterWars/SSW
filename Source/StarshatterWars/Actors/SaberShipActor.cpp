#include "SaberShipActor.h"
ASaberShipActor::ASaberShipActor()
{
    SetActorScale3D(FVector(0.2f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Saber"));
    InitializeNativeFighterAttachments(TEXT(""), 1.000000f, FVector(8.000000f, 0.000000f, 145.000000f), {
        FFighterLandingGearDefinition(TEXT("LeftSkid"), FVector(0.000000f, -40.000000f, -36.000000f), FVector(0.000000f, -58.000000f, -54.000000f)),
        FFighterLandingGearDefinition(TEXT("RightSkid"), FVector(0.000000f, 40.000000f, -36.000000f), FVector(0.000000f, 58.000000f, -54.000000f)),
    });

}

void ASaberShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(32.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-1");
      Point.LocalOffset = FVector(-10.000000f, -101.000000f, 11.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-2");
      Point.LocalOffset = FVector(-10.000000f, -74.000000f, 1.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-3");
      Point.LocalOffset = FVector(-10.000000f, 74.000000f, 1.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("STA-4");
      Point.LocalOffset = FVector(-10.000000f, 101.000000f, 11.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
