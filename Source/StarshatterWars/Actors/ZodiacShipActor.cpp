#include "ZodiacShipActor.h"
AZodiacShipActor::AZodiacShipActor()
{

    SetActorScale3D(FVector(0.45f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Zodiac"));
    InitializeNativeFighterAttachments(TEXT(""), 1.000000f, FVector(8.000000f, 0.000000f, 145.000000f), {
        FFighterLandingGearDefinition(TEXT("ZGearRight"), FVector(-10.000000f, 44.000000f, -25.000000f), FVector(-10.000000f, 74.000000f, -60.000000f)),
        FFighterLandingGearDefinition(TEXT("ZGearLeft"), FVector(-10.000000f, -44.000000f, -25.000000f), FVector(-10.000000f, -74.000000f, -60.000000f)),
        FFighterLandingGearDefinition(TEXT("ZGearRight"), FVector(-128.000000f, 44.000000f, -25.000000f), FVector(-128.000000f, 74.000000f, -60.000000f)),
        FFighterLandingGearDefinition(TEXT("ZGearLeft"), FVector(-128.000000f, -44.000000f, -25.000000f), FVector(-128.000000f, -74.000000f, -60.000000f)),
    });

}

void AZodiacShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Main Cannon");
      Point.LocalOffset = FVector(110.000000f, 0.000000f, -10.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret");
      Point.LocalOffset = FVector(-189.000000f, 0.000000f, 74.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    { FShipPointDef Point; Point.PointName = TEXT("Turret_Base");
      Point.LocalOffset = FVector(-189.000000f, 0.000000f, 74.000000f);
      Point.LocalRotation = FRotator(0.000000f, 180.000000f, 0.0f);
      TurretBasePointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
