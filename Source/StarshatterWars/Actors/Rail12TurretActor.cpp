#include "Rail12TurretActor.h"
ARail12TurretActor::ARail12TurretActor()
{
    WeaponType=TEXT("Rail-12");
    LegacyGunModel=TEXT("Rail_12Turret");
    LegacyBaseModel=TEXT("");
    LegacyModelScale=3.0f;
    BaseAxis=0;
    LegacyMuzzles.Add(FVector(4.0,11.0,75.0));
    LegacyMuzzles.Add(FVector(-4.0,11.0,75.0));
}
