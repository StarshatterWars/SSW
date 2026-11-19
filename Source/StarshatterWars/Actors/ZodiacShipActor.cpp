#include "ZodiacShipActor.h"
#include "Components/SceneComponent.h"
AZodiacShipActor::AZodiacShipActor()
{

    SetActorScale3D(FVector(0.45f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("Zodiac"));
    bUseAuthoredCockpitMount=true;
    FalconCockpitVisualOffset=FVector::ZeroVector;
    InitializeNativeFighterAttachments(TEXT("FalconPit"), 0.020000f, FVector(145.000000f, 0.000000f, 8.000000f), {
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

void AZodiacShipActor::OnConstruction(const FTransform& Transform)
{
    // Migrate saved Zodiac BP defaults that explicitly serialized the old missing cockpit.
    bUseAuthoredCockpitMount=true;
    if(AuthoredCockpitModel.IsEmpty()){AuthoredCockpitModel=TEXT("FalconPit");AuthoredCockpitScale=.02f;}
    if(FighterCockpitRoot && (FighterCockpitRoot->GetRelativeLocation().IsNearlyZero() || FighterCockpitRoot->GetRelativeLocation().Equals(FVector(8,0,145))))
        FighterCockpitRoot->SetRelativeLocation(FVector(145,0,8));
    Super::OnConstruction(Transform);
}
