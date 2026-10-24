#include "ZZAShipActor.h"
AZZAShipActor::AZZAShipActor()
{
    SetActorScale3D(FVector(0.75f));

    ApplyAuthoredAttachmentDefaults();
    InitializeNativeAuthoredAttachments(TEXT("ZZA"));
    InitializeNativeFighterAttachments(TEXT(""), 1.000000f, FVector(0.000000f, 0.000000f, 0.000000f), {
    });

}

void AZZAShipActor::ApplyAuthoredAttachmentDefaults()
{
    WeaponMountPointDefs.Reset();
    TurretBasePointDefs.Reset();
    { FShipPointDef Point; Point.PointName = TEXT("Cannon");
      Point.LocalOffset = FVector(20.000000f, 0.000000f, 0.000000f);
      Point.LocalRotation = FRotator(0.000000f, 0.000000f, 0.0f);
      WeaponMountPointDefs.Add(Point); }
    SyncShipAttachmentCounts();
}
