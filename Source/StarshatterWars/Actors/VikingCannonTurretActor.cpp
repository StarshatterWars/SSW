#include "VikingCannonTurretActor.h"
AVikingCannonTurretActor::AVikingCannonTurretActor()
{
    WeaponType=TEXT("Viking Cannon");
    LegacyGunModel=TEXT("VikingCannonTurret");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=5.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(4.0,0.0,65.0));
    LegacyMuzzles.Add(FVector(-4.0,0.0,65.0));
}
